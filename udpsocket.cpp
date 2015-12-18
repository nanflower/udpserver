#include "udpsocket.h"

NewQueue recvqueue;

udpsocket::udpsocket()
{
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
    buf = (uint8_t*)av_mallocz(sizeof(uint8_t)*BUFFER_SIZE);
    pframe = av_frame_alloc();
    got_picture = 0;
    frame_size = AVCODEC_MAX_AUDIO_FRAME_SIZE*3/2;
}

udpsocket::~udpsocket()
{

}

void udpsocket::thread_init(int index)
{
    init_queue(&recvqueue, 1024*1024*500);

    pthread_t udp_recv_thread;
    if(index==0){
        if( 0 != pthread_create( &udp_recv_thread, NULL, udp_ts_recv1, this ) )
            printf("%s:%d  Error: Create udp receive thread failed !!!\n", __FILE__, __LINE__ );
    }
    else if(index==1){
        if( 0 != pthread_create( &udp_recv_thread, NULL, udp_ts_recv2, this ) )
            printf("%s:%d  Error: Create udp receive thread failed !!!\n", __FILE__, __LINE__ );
    }
    else if(index==2){
        if( 0 != pthread_create( &udp_recv_thread, NULL, udp_ts_recv3, this ) )
            printf("%s:%d  Error: Create udp receive thread failed !!!\n", __FILE__, __LINE__ );
    }

    pthread_detach(udp_recv_thread);
}

int udpsocket::ts_demux()
{

    av_register_all();

    pb = avio_alloc_context(buf, BUFFER_SIZE, 0, NULL, read_data, NULL, NULL);
    if (!pb) {
        fprintf(stderr, "avio alloc failed!\n");
        return -1;
    }
    if (av_probe_input_buffer(pb, &piFmt, "", NULL, 0, 0) < 0) {
        fprintf(stderr, "probe failed!\n");
//			return -1;
    } else {
        fprintf(stdout, "probe success!\n");
        fprintf(stdout, "format: %s[%s]\n", piFmt->name, piFmt->long_name);
    }
    pFmt = avformat_alloc_context();
    pFmt->pb = pb;
    if (avformat_open_input(&pFmt, "", piFmt, NULL) < 0) {
        fprintf(stderr, "avformat open failed.\n");
        return -1;
    } else {
        fprintf(stdout, "open stream success!\n");
    }
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
    printf("video = %d,audio = %d,video1 = %d ,audio1 = %d\n",videoindex[0], audioindex[0], videoindex[1], audioindex[1]);
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
                        printf("video %d decode %d num\n", i, video_num[i]++);
                        break;
                    }
                 }else if (pkt.stream_index == audioindex[i]) {
    //                if (avcodec_decode_audio3(pAudioCodecCtx, (int16_t *)samples, &frame_size, &pkt) >= 0) {
    //                    fprintf(stdout, "decode one audio frame!\r");
    //                }
                    if (avcodec_decode_audio4(pAudioCodecCtx[i], pframe, &frame_size, &pkt) >= 0) {
                    //    fprintf(stdout, "decode one audio frame!\r");
                        printf("audio %d decode %d num\n", i, audio_num[i]++);
                        break;
                    }
                }
            }
            av_free_packet(&pkt);
        }
    }

    av_free(buf);
    av_free(pframe);
    free_queue(&recvqueue);
    return 0;

}

void *udpsocket::udp_ts_recv1(void * pArg)
{
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
    printf("before server\n");
    /* 数据传输 */
    while(1)
    {
         /* 定义一个地址，用于捕获客户端地址 */
         struct sockaddr_in client_addr;
         socklen_t client_addr_length = sizeof(client_addr);
         /* 接收数据 */
         char buffer[BUFFER_SIZE];
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
             put_queue( buffer, len);
       //      else
       //          printf("socket 1 work\n");
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

void *udpsocket::udp_ts_recv2(void * pArg)
{
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
    while(1)
    {
         /* 定义一个地址，用于捕获客户端地址 */
         struct sockaddr_in client_addr;
         socklen_t client_addr_length = sizeof(client_addr);
         /* 接收数据 */
         char buffer[BUFFER_SIZE];
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
             if (recvfrom(server_socket_fd, buffer, BUFFER_SIZE,0,(struct sockaddr*)&client_addr, &client_addr_length) == -1)
             {
                 printf("received data error!\n");
             }
      //       else
      //           printf("socket 2 work\n");
         }
         else
         {
             printf("error is %d\n",errno);
             printf("timeout!there is no data arrived!\n");
         }

         /* 从buffer中拷贝出file_name */
//         char file_name[FILE_NAME_MAX_SIZE+1];
//         bzero(file_name,FILE_NAME_MAX_SIZE+1);
//         strncpy(file_name, buffer, strlen(buffer)>FILE_NAME_MAX_SIZE?FILE_NAME_MAX_SIZE:strlen(buffer));
         //printf("%s\n", file_name);
    }
}

void *udpsocket::udp_ts_recv3(void * pArg)
{
    /* 创建UDP套接口 */
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    //server_addr.sin_addr.s_addr = inet_addr("1.8.84.12");
    server_addr.sin_port = htons(50103);

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
         char buffer[BUFFER_SIZE];
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
             if (recvfrom(server_socket_fd, buffer, BUFFER_SIZE,0,(struct sockaddr*)&client_addr, &client_addr_length) == -1)
             {
                 printf("received data error!\n");
             }
         //    else
       //          printf("socket 3 work\n");
         }
         else
         {
             printf("error is %d\n",errno);
             printf("timeout!there is no data arrived!\n");
         }

         /* 从buffer中拷贝出file_name */
//         char file_name[FILE_NAME_MAX_SIZE+1];
//         bzero(file_name,FILE_NAME_MAX_SIZE+1);
//         strncpy(file_name, buffer, strlen(buffer)>FILE_NAME_MAX_SIZE?FILE_NAME_MAX_SIZE:strlen(buffer));
         //printf("%s\n", file_name);
    }
}

void udpsocket::init_queue(NewQueue *que, int size) {
    pthread_mutex_init(&que->locker, NULL);
    pthread_cond_init(&que->cond, NULL);
    que->buf = (uint8_t*)av_mallocz(sizeof(uint8_t)*size);
    que->read_ptr = que->write_ptr = 0;
    que->bufsize = size;
    printf("buffer size = %d\n",que->bufsize);
//	fprintf(stdout, "que->bufsize=%d\n", size);
}

void udpsocket::free_queue(NewQueue* que) {
    pthread_mutex_destroy(&que->locker);
    pthread_cond_destroy(&que->cond);
    av_free(que->buf);
}

void udpsocket::put_queue( char* buf, int size) {
    uint8_t* dst = recvqueue.buf + recvqueue.write_ptr;

    pthread_mutex_lock(&recvqueue.locker);
    //printf("dst = %d\n",size);
    if ((recvqueue.write_ptr + size) > recvqueue.bufsize) {
        memcpy(dst, buf, (recvqueue.bufsize - recvqueue.write_ptr));
        memcpy(recvqueue.buf, buf+(recvqueue.bufsize - recvqueue.write_ptr), size-(recvqueue.bufsize - recvqueue.write_ptr));
    } else {
        memcpy(dst, buf, size*sizeof(uint8_t));
    }
    recvqueue.write_ptr = (recvqueue.write_ptr + size) % recvqueue.bufsize;
    pthread_cond_signal(&recvqueue.cond);
    pthread_mutex_unlock(&recvqueue.locker);
}

int udpsocket::get_queue(uint8_t* buf, int size) {
    uint8_t* src = recvqueue.buf + recvqueue.read_ptr;
    int wrap = 0;

    pthread_mutex_lock(&recvqueue.locker);

    int pos = recvqueue.write_ptr;

    if (pos < recvqueue.read_ptr) {
        pos += recvqueue.bufsize;
        wrap = 1;
    }

    if ( (recvqueue.read_ptr + size) > pos) {
        pthread_mutex_unlock(&recvqueue.locker);
        return 1;
//		struct timespec timeout;
//		timeout.tv_sec=time(0)+1;
//		timeout.tv_nsec=0;
//		pthread_cond_timedwait(&que->cond, &que->locker, &timeout);
//		if ( (que->read_ptr + size) > pos ) {
//			pthread_mutex_unlock(&que->locker);
//			return 1;
//		}
    }

    if (wrap) {
        fprintf(stdout, "wrap...\n");
        memcpy(buf, src, (recvqueue.bufsize - recvqueue.read_ptr));
        memcpy(buf+(recvqueue.bufsize - recvqueue.read_ptr), src+(recvqueue.bufsize - recvqueue.read_ptr), size-(recvqueue.bufsize - recvqueue.read_ptr));
    } else {
        memcpy(buf, src, sizeof(uint8_t)*size);
    }
    recvqueue.read_ptr = (recvqueue.read_ptr + size) % recvqueue.bufsize;
    pthread_mutex_unlock(&recvqueue.locker);

    return 0;
}

int udpsocket::read_data(void *opaque, uint8_t *buf, int buf_size) {
//	UdpParam udphead;
    int size = buf_size;
    int ret;
   // printf("read data %d\n", buf_size);
    do {
        ret = get_queue( buf, buf_size);
    } while (ret);

  //  printf("read data Ok %d\n", buf_size);
    return size;
}
