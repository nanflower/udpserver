#include "udpsocket.h"

udpsocket::udpsocket()
{

    printf("into new tspool\n");
    m_tsRecvPool = new tspoolqueue;
    printf("out tspool\n");
}

udpsocket::~udpsocket()
{

}

void udpsocket::thread_init(int index)
{

    //init_buffer();

    pthread_t udp_recv_thread;
    memset( &udp_recv_thread, 0, sizeof( udp_recv_thread ) );
    if(index == 0){
        if( 0 != pthread_create( &udp_recv_thread, NULL, udp_tsrecv, this ) )
            printf("%s:%d  Error: Create udp receive thread failed !!!\n", __FILE__, __LINE__ );
    }
    else if(index == 1){
        if( 0 != pthread_create( &udp_recv_thread, NULL, udp_tsrecv1, this ) )
            printf("%s:%d  Error: Create udp receive thread failed !!!\n", __FILE__, __LINE__ );
    }

    pthread_t ts_demux_thread;
    memset( &ts_demux_thread, 0, sizeof( ts_demux_thread ) );

    if(index == 0){
        if( 0 != pthread_create( &ts_demux_thread, NULL, ts_demuxer, this ) )
            printf("%s:%d  Error: Create udp receive thread failed !!!\n", __FILE__, __LINE__ );
    }
    else if(index == 1){
        if( 0 != pthread_create( &ts_demux_thread, NULL, ts_demuxer1, this ) )
            printf("%s:%d  Error: Create udp receive thread failed !!!\n", __FILE__, __LINE__ );
    }

    pthread_detach(ts_demux_thread);
    pthread_detach(udp_recv_thread);
}

void *udpsocket::udp_tsrecv(void *pArg)
{
    udpsocket* pTemp = (udpsocket*)pArg;
    if( pTemp )
        pTemp->udp_ts_recv();
    return (void*)NULL;
}

void *udpsocket::udp_tsrecv1(void *pArg)
{
    udpsocket* pTemp = (udpsocket*)pArg;
    if( pTemp )
        pTemp->udp_ts_recv1();
    return (void*)NULL;
}

void *udpsocket::ts_demuxer(void *pArg)
{
    udpsocket* pTemp = (udpsocket*) pArg;
    if( pTemp )
        pTemp->ts_demux(1);
    //    pTemp->thread_test();
    return (void*)NULL;
}

void *udpsocket::ts_demuxer1(void *pArg)
{
    udpsocket* pTemp = (udpsocket*) pArg;
    if( pTemp )
        pTemp->ts_demux(2);
    //    pTemp->thread_test();
    return (void*)NULL;
}

int udpsocket::thread_test()
{
    while(1)
    {
        printf("thread work\n");
    }
    return 1;
}

int udpsocket::ts_demux(int index)
{
    AVCodec *pVideoCodec[VIDEO_NUM];
    AVCodec *pAudioCodec[AUDIO_NUM];
    AVCodecContext *pVideoCodecCtx[VIDEO_NUM];
    AVCodecContext *pAudioCodecCtx[AUDIO_NUM];
    AVIOContext * pb;
    AVInputFormat *piFmt;
    AVFormatContext *pFmt;
    uint8_t *buffer;
    int videoindex[VIDEO_NUM];
    int audioindex[AUDIO_NUM];
    AVStream *pVst[VIDEO_NUM];
    AVStream *pAst[AUDIO_NUM];
    AVFrame *pframe;
    AVPacket pkt;
    int got_picture;
    int video_num[VIDEO_NUM];
    int audio_num[AUDIO_NUM];
    int frame_size;

    for( int i=0; i<VIDEO_NUM; i++ ){
        pVideoCodec[i] = NULL;
        pVideoCodecCtx[i] =NULL;
        videoindex[i] = -1;
        pVst[i] = NULL;
        video_num[i] = 0;
    }
    for( int i=0; i<AUDIO_NUM; i++ ){
        pAudioCodec[i] = NULL;
        pAudioCodecCtx[i] = NULL;
        audioindex[i] = -1;
        pAst[i] = NULL;
        audio_num[i] = 0;
    }
    pb = NULL;
    piFmt = NULL;
    pFmt = NULL;
    pframe = NULL;
    buffer = (uint8_t*)av_mallocz(sizeof(uint8_t)*BUFFER_SIZE);
    pframe = av_frame_alloc();
    got_picture = 0;
    frame_size = AVCODEC_MAX_AUDIO_FRAME_SIZE*3/2;


    av_register_all();
    if(index == 1){
        pb = avio_alloc_context(buffer, 4096, 0, NULL, read_data, NULL, NULL);
        printf("thread %d pid %lu tid %lu\n",index,(unsigned long)getpid(),(unsigned long)pthread_self());
    }
    else if(index == 2){
        pb = avio_alloc_context(buffer, 4096, 0, NULL, read_data1, NULL, NULL);
        printf("thread %d pid %lu tid %lu\n",index,(unsigned long)getpid(),(unsigned long)pthread_self());
    }
    if (!pb) {
        fprintf(stderr, "avio alloc failed!\n");
        return -1;
    }
    printf("demux work\n");
    int x = av_probe_input_buffer(pb, &piFmt, "", NULL, 0, 0);
    if (x < 0) {
        printf("probe error: %d",x);
       // fprintf(stderr, "probe failed!\n");
    } else {
        fprintf(stdout, "probe success!\n");
        fprintf(stdout, "format: %s[%s]\n", piFmt->name, piFmt->long_name);
    }
    pFmt = avformat_alloc_context();
    pFmt->pb = pb;
    printf("demux work1\n");
    if (avformat_open_input(&pFmt, "", piFmt, NULL) < 0) {
        fprintf(stderr, "avformat open failed.\n");
        return -1;
    } else {
        fprintf(stdout, "open stream success!\n");
    }printf("demux work2\n");
    //pFmt->probesize = 4096 * 2000;
    //pFmt->max_analyze_duration = 5 * AV_TIME_BASE;
    //pFmt->probesize = 2048;
   // pFmt->max_analyze_duration = 1000;
    pFmt->probesize = 2048 * 1000 ;
    pFmt->max_analyze_duration = 2048 * 1000;
    if (avformat_find_stream_info(pFmt,0) < 0) {
        fprintf(stderr, "could not fine stream.\n");
        return -1;
    }
    printf("dump format\n");
    av_dump_format(pFmt, 0, "", 0);

    int videox = 0,audiox = 0;
    for (int i = 0; i < pFmt->nb_streams; i++) {
        if(videox == 7 && audiox == 7)
            break;
        if ( pFmt->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO && videox < 7 ) {
            videoindex[ videox++ ] = i;
        }
        if ( pFmt->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO && audiox < 7 ) {
            audioindex[ audiox++ ] = i;
        }
    }

    for(int i=0; i<VIDEO_NUM; i++)
        printf("videoindex %d = %d, audioindex %d = %d\n",i , videoindex[i], i ,audioindex[i]);

    if (videoindex[6] < 0 || audioindex[6] < 0) {
        fprintf(stderr, "videoindex=%d, audioindex=%d\n", videoindex[6], audioindex[6]);
        return -1;
    }

    for( int i=0; i<VIDEO_NUM; i++ ){
        pVst[i] = pFmt->streams[videoindex[i]];
        pVideoCodecCtx[i] = pVst[i]->codec;
        pVideoCodec[i] = avcodec_find_decoder(pVideoCodecCtx[i]->codec_id);
        if (!pVideoCodec[i]) {
            fprintf(stderr, "could not find video decoder!\n");
            return -1;
        }
        if (avcodec_open2(pVideoCodecCtx[i], pVideoCodec[i], NULL) < 0) {
            fprintf(stderr, "could not open video codec!\n");
            return -1;
        }
    }
    for( int i=0; i<VIDEO_NUM; i++ ){
        pAst[i] = pFmt->streams[audioindex[i]];
        pAudioCodecCtx[i] = pAst[i]->codec;
        pAudioCodec[i] = avcodec_find_decoder(pAudioCodecCtx[i]->codec_id);
        if (!pAudioCodec[i]) {
            fprintf(stderr, "could not find audio decoder!\n");
            return -1;
        }
        if (avcodec_open2(pAudioCodecCtx[i], pAudioCodec[i], NULL) < 0) {
            fprintf(stderr, "could not open audio codec!\n");
            return -1;
        }
    }

    //uint8_t samples[AVCODEC_MAX_AUDIO_FRAME_SIZE*3/2];
    av_init_packet(&pkt);
    printf("start decode\n");
    while(1) {
        if (av_read_frame(pFmt, &pkt) >= 0) {

            for( int i=0; i<VIDEO_NUM; i++ ){
                if (pkt.stream_index == videoindex[i]) {
                    avcodec_decode_video2(pVideoCodecCtx[i], pframe, &got_picture, &pkt);
                    if (got_picture) {
                        printf("index = %d video %d decode %d num\n", index, i, video_num[i]++);
                        break;
                    }
                 }else if (pkt.stream_index == audioindex[i]) {
    //                if (avcodec_decode_audio3(pAudioCodecCtx, (int16_t *)samples, &frame_size, &pkt) >= 0) {
    //                    fprintf(stdout, "decode one audio frame!\r");
    //                }
                    if (avcodec_decode_audio4(pAudioCodecCtx[i], pframe, &frame_size, &pkt) >= 0) {
                    //    fprintf(stdout, "decode one audio frame!\r");
                        printf("index = %d audio %d decode %d num\n", index, i, audio_num[i]++);
                        break;
                    }
                }
            }
            av_free_packet(&pkt);
        }
    }

    av_free(buffer);
    av_free(pframe);
    return 0;

}

void udpsocket::udp_ts_recv(void)
{
    printf("thread 1 pid %lu tid %lu\n",(unsigned long)getpid(),(unsigned long)pthread_self());
    /* 创建UDP套接口 */
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    //server_addr.sin_addr.s_addr = inet_addr("1.8.84.12");
        server_addr.sin_port = htons(50101);

    /* 创建socket */
    int server_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(server_socket_fd == -1)
    {
         perror("Create Socket Failed:");
         exit(1);
    }

    memset(server_addr.sin_zero,0,8);
     int re_flag=1;
     int re_len=sizeof(int);
     setsockopt(server_socket_fd,SOL_SOCKET,SO_REUSEADDR,&re_flag,re_len);

    /* 绑定套接口 */
    if(-1 == (bind(server_socket_fd,(struct sockaddr*)&server_addr,sizeof(server_addr))))
    {
         perror("Server Bind Failed:");
         exit(1);
    }
    /* 数据传输 */

    while(1)
    {
         /* 定义一个地址，用于捕获客户端地址 */
         struct sockaddr_in client_addr;
         socklen_t client_addr_length = sizeof(client_addr);
         /* 接收数据 */
         uint8_t buffer[BUFFER_SIZE];
         bzero(buffer, BUFFER_SIZE);
         struct timeval tv;
         fd_set readfds;
         tv.tv_sec = 3;
         tv.tv_usec = 10;
         FD_ZERO(&readfds);
         FD_SET(server_socket_fd, &readfds);
         select(server_socket_fd+1,&readfds,NULL, NULL, &tv);
         if (FD_ISSET(server_socket_fd,&readfds))
         {
             int len = recvfrom(server_socket_fd, buffer, BUFFER_SIZE,0,(struct sockaddr*)&client_addr, &client_addr_length);
             if (len == -1)
             {
                 printf("received data error!\n");
             }
           // printf("receive length = %d\n",len);
             m_tsRecvPool->put_queue( buffer, len);
       //      else
      //       printf("socket %d work\n", multiindex);
         }
         else
         {
             printf("error is %d\n",errno);
             printf("timeout!there is no data arrived!\n");
         }
     /* 从buffer中拷贝出file_name */
//     char file_name[FILE_NAME_MAX_SIZE+1];
//     bzero(file_name,FILE_NAME_MAX_SIZE+1);
//     strncpy(file_name, buffer, strlen(buffer)>FILE_NAME_MAX_SIZE?FILE_NAME_MAX_SIZE:strlen(buffer));
     //printf("%s\n", file_name);
    }
}

void udpsocket::udp_ts_recv1(void)
{
  //  printf("thread 2 pid %lu tid %lu\n",(unsigned long)getpid(),(unsigned long)pthread_self());
    /* 创建UDP套接口 */
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    //server_addr.sin_addr.s_addr = inet_addr("1.8.84.12");
        server_addr.sin_port = htons(50102);

    /* 创建socket */
    int server_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(server_socket_fd == -1)
    {
         perror("Create Socket Failed:");
         exit(1);
    }

    memset(server_addr.sin_zero,0,8);
     int re_flag=1;
     int re_len=sizeof(int);
     setsockopt(server_socket_fd,SOL_SOCKET,SO_REUSEADDR,&re_flag,re_len);

    /* 绑定套接口 */
    if(-1 == (bind(server_socket_fd,(struct sockaddr*)&server_addr,sizeof(server_addr))))
    {
         perror("Server Bind Failed:");
         exit(1);
    }
    /* 数据传输 */

    /* 定义一个地址，用于捕获客户端地址 */
    struct sockaddr_in client_addr;
    socklen_t client_addr_length = sizeof(client_addr);
    /* 接收数据 */
    uint8_t buffer[BUFFER_SIZE];
    bzero(buffer, BUFFER_SIZE);
//    struct timeval tv;
//    fd_set readfds;
//    tv.tv_sec = 3;
//    tv.tv_usec = 10;
//    FD_ZERO(&readfds);
//    FD_SET(server_socket_fd, &readfds);

    while(1)
    {
//         select(server_socket_fd+1,&readfds,NULL, NULL, &tv);
//         if (FD_ISSET(server_socket_fd,&readfds))
//         {
             int len = recvfrom(server_socket_fd, buffer, BUFFER_SIZE,0,(struct sockaddr*)&client_addr, &client_addr_length);
             if (len == -1)
             {
                 printf("received data error!\n");
             }
           // printf("receive length = %d\n",len);
             m_tsRecvPool->put_queue1( buffer, len);
       //      else
      //       printf("socket %d work\n", multiindex);
//         }
//         else
//         {
//             printf("error is %d\n",errno);
//             printf("timeout!there is no data arrived!\n");
//         }
     /* 从buffer中拷贝出file_name */
//     char file_name[FILE_NAME_MAX_SIZE+1];
//     bzero(file_name,FILE_NAME_MAX_SIZE+1);
//     strncpy(file_name, buffer, strlen(buffer)>FILE_NAME_MAX_SIZE?FILE_NAME_MAX_SIZE:strlen(buffer));
     //printf("%s\n", file_name);
    }
}


int udpsocket::read_data(void *opaque, uint8_t *buf, int buf_size) {
//	UdpParam udphead;
 //   tspoolqueue* pTemp = new tspoolqueue;
    tspoolqueue* pTemp = (tspoolqueue*)opaque;


    int size = buf_size;
    bool ret;
   // printf("read data %d\n", buf_size);
    do {
        ret = pTemp->get_queue( buf, size);
     //   ret = m_tsRecvPool->GetTsPacket(buf);
    } while (ret);

  //  printf("read data Ok %d\n", buf_size);
    return size;
}

int udpsocket::read_data1(void *opaque, uint8_t *buf, int buf_size) {
//	UdpParam udphead;
 //   tspoolqueue* pTemp = new tspoolqueue;
    tspoolqueue* pTemp = (tspoolqueue*)opaque;


    int size = buf_size;
    bool ret;
   // printf("read data %d\n", buf_size);
    do {
        ret = pTemp->get_queue1( buf, size);
     //   ret = m_tsRecvPool->GetTsPacket(buf);
    } while (ret);

  //  printf("read data Ok %d\n", buf_size);
    return size;
}

