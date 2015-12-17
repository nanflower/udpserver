#include "udpsocket.h"

NewQueue recvqueue;

udpsocket::udpsocket()
{

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

    uint8_t *buf = (uint8_t*)av_mallocz(sizeof(uint8_t)*BUFFER_SIZE);
    av_register_all();
    AVCodec *pVideoCodec, *pAudioCodec;
    AVCodec *pVideoCodec1, *pAudioCodec1;
    AVCodecContext *pVideoCodecCtx = NULL;
    AVCodecContext *pAudioCodecCtx = NULL;
    AVCodecContext *pVideoCodecCtx1 = NULL;
    AVCodecContext *pAudioCodecCtx1 = NULL;
    AVIOContext * pb = NULL;
    AVInputFormat *piFmt = NULL;
    AVFormatContext *pFmt = NULL;

    pb = avio_alloc_context(buf, BUFFER_SIZE, 0, NULL, read_data, NULL, NULL);
    if (!pb) {
        fprintf(stderr, "avio alloc failed!\n");
        return -1;
    }
    printf("before get input\n");
    if (av_probe_input_buffer(pb, &piFmt, "", NULL, 0, 0) < 0) {
        fprintf(stderr, "probe failed!\n");
        printf("loop fail\n");
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
    printf("find stream info\n");
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

    int videoindex = -1,videoindex1 = -1;
    int audioindex = -1,audioindex1 = -1;
    for (int i = 0; i < pFmt->nb_streams; i++) {
        if ( (pFmt->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) &&
                (videoindex < 0 || videoindex1 < 0) ) {
            if(videoindex1 > 0){
                videoindex = i;
            }
            else
                videoindex1 = i;
        }
        if ( (pFmt->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) &&
                (audioindex < 0 || audioindex1 < 0) ) {
            if(audioindex1 > 0){
                audioindex = i;
            }
            else
                audioindex1 = i;
        }
    }
    printf("video = %d,audio = %d,video1 = %d ,audio1 = %d\n",videoindex, audioindex, videoindex1, audioindex1);
    if (videoindex < 0 || audioindex < 0) {
        fprintf(stderr, "videoindex=%d, audioindex=%d\n", videoindex, audioindex);
        return -1;
    }

    AVStream *pVst,*pAst;
    AVStream *pVst1[7],*pAst1[7];
    pVst = pFmt->streams[videoindex];
    pAst = pFmt->streams[audioindex];

    pVst1[0] = pFmt->streams[videoindex1];
    pAst1[0] = pFmt->streams[audioindex1];

    pVideoCodecCtx = pVst->codec;
    pAudioCodecCtx = pAst->codec;

    pVideoCodecCtx1 = pVst1[0]->codec;
    pAudioCodecCtx1 = pAst1[0]->codec;

    //VIDEO 0
    pVideoCodec = avcodec_find_decoder(pVideoCodecCtx->codec_id);
    if (!pVideoCodec) {
        fprintf(stderr, "could not find video decoder!\n");
        return -1;
    }
    if (avcodec_open2(pVideoCodecCtx, pVideoCodec, NULL) < 0) {
        fprintf(stderr, "could not open video codec!\n");
        return -1;
    }
    //VIDEO 1
    pVideoCodec1 = avcodec_find_decoder(pVideoCodecCtx1->codec_id);
    if (!pVideoCodec1) {
        fprintf(stderr, "could not find video decoder!\n");
        return -1;
    }
    if (avcodec_open2(pVideoCodecCtx1, pVideoCodec1, NULL) < 0) {
        fprintf(stderr, "could not open video codec!\n");
        return -1;
    }

    //AUDIO 0
    pAudioCodec = avcodec_find_decoder(pAudioCodecCtx->codec_id);
    if (!pAudioCodec) {
        fprintf(stderr, "could not find audio decoder!\n");
        return -1;
    }
    if (avcodec_open2(pAudioCodecCtx, pAudioCodec, NULL) < 0) {
        fprintf(stderr, "could not open audio codec!\n");
        return -1;
    }
    //AUDIO 1
    pAudioCodec1 = avcodec_find_decoder(pAudioCodecCtx1->codec_id);
    if (!pAudioCodec1) {
        fprintf(stderr, "could not find audio decoder!\n");
        return -1;
    }
    if (avcodec_open2(pAudioCodecCtx1, pAudioCodec1, NULL) < 0) {
        fprintf(stderr, "could not open audio codec!\n");
        return -1;
    }



    int got_picture;
    //uint8_t samples[AVCODEC_MAX_AUDIO_FRAME_SIZE*3/2];
    AVFrame *pframe = av_frame_alloc();
    AVPacket pkt;
    av_init_packet(&pkt);
    printf("start decode\n");
    int picture_num = 0,picture_num1 = 0;
    int audio_num = 0,audio_num1 = 0;
    while(1) {
        if (av_read_frame(pFmt, &pkt) >= 0) {

            if (pkt.stream_index == videoindex) {
                avcodec_decode_video2(pVideoCodecCtx, pframe, &got_picture, &pkt);
                if (got_picture) {
                    printf("1 decode %d video num\n",picture_num++);
        //            fprintf(stdout, "decode one video frame!\r");
                }
             }else if (pkt.stream_index == videoindex1) {
                avcodec_decode_video2(pVideoCodecCtx1, pframe, &got_picture, &pkt);
                if (got_picture) {
                    printf("2 decode %d video num\n",picture_num1++);
        //            fprintf(stdout, "decode one video frame!\r");
                }
            }else if (pkt.stream_index == audioindex) {
                int frame_size = AVCODEC_MAX_AUDIO_FRAME_SIZE*3/2;
//                if (avcodec_decode_audio3(pAudioCodecCtx, (int16_t *)samples, &frame_size, &pkt) >= 0) {
//                    fprintf(stdout, "decode one audio frame!\r");
//                }
                int audio_id = avcodec_decode_audio4(pAudioCodecCtx, pframe, &frame_size, &pkt);
                if (audio_id >= 0) {
                //    fprintf(stdout, "decode one audio frame!\r");
                    printf("1 decode %d audio num\n",audio_num++);
                }
            }else if (pkt.stream_index == audioindex1) {
                int frame_size = AVCODEC_MAX_AUDIO_FRAME_SIZE*3/2;
//                if (avcodec_decode_audio3(pAudioCodecCtx, (int16_t *)samples, &frame_size, &pkt) >= 0) {
//                    fprintf(stdout, "decode one audio frame!\r");
//                }
                int audio_id = avcodec_decode_audio4(pAudioCodecCtx1, pframe, &frame_size, &pkt);
                if (audio_id >= 0) {
                //    fprintf(stdout, "decode one audio frame!\r");
                    printf("2 decode %d audio num\n",audio_num1++);
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
             put_queue(&recvqueue, buffer, len);
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

void udpsocket::put_queue(NewQueue*que, char* buf, int size) {
    uint8_t* dst = que->buf + que->write_ptr;

    pthread_mutex_lock(&que->locker);
    //printf("dst = %d\n",size);
    if ((que->write_ptr + size) > que->bufsize) {
        memcpy(dst, buf, (que->bufsize - que->write_ptr));
        memcpy(que->buf, buf+(que->bufsize - que->write_ptr), size-(que->bufsize - que->write_ptr));
    } else {
        memcpy(dst, buf, size*sizeof(uint8_t));
    }
    que->write_ptr = (que->write_ptr + size) % que->bufsize;
    pthread_cond_signal(&que->cond);
    pthread_mutex_unlock(&que->locker);
}

int udpsocket::get_queue(NewQueue*que, uint8_t* buf, int size) {
    uint8_t* src = que->buf + que->read_ptr;
    int wrap = 0;

    pthread_mutex_lock(&que->locker);

    int pos = que->write_ptr;

    if (pos < que->read_ptr) {
        pos += que->bufsize;
        wrap = 1;
    }

    if ( (que->read_ptr + size) > pos) {
        pthread_mutex_unlock(&que->locker);
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
        memcpy(buf, src, (que->bufsize - que->read_ptr));
        memcpy(buf+(que->bufsize - que->read_ptr), src+(que->bufsize - que->read_ptr), size-(que->bufsize - que->read_ptr));
    } else {
        memcpy(buf, src, sizeof(uint8_t)*size);
    }
    que->read_ptr = (que->read_ptr + size) % que->bufsize;
    pthread_mutex_unlock(&que->locker);

    return 0;
}

int udpsocket::read_data(void *opaque, uint8_t *buf, int buf_size) {
//	UdpParam udphead;
    int size = buf_size;
    int ret;
   // printf("read data %d\n", buf_size);
    do {
        ret = get_queue(&recvqueue, buf, buf_size);
    } while (ret);

  //  printf("read data Ok %d\n", buf_size);
    return size;
}
