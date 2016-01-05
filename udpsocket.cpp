#include "udpsocket.h"

FILE *fp_write;
FILE *fp_v;
FILE *fp_a;

int write_buffer(void *opaque, uint8_t *buf, int buf_size){
    if(!feof(fp_write)){
        int true_size=fwrite(buf,buf_size,1,fp_write);
        return true_size;
    }else{
        return -1;
    }
}

udpsocket::udpsocket()
{

    printf("into new tspool\n");
    m_tsRecvPool = new tspoolqueue;
    printf("out tspool\n");
    fp_write=fopen("cuc60anniversary_start.h264","wb+"); //输出文件
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
            printf("%s:%d  Error: Create ts demux thread failed !!!\n", __FILE__, __LINE__ );
    }
    else if(index == 1){
        if( 0 != pthread_create( &ts_demux_thread, NULL, ts_demuxer1, this ) )
            printf("%s:%d  Error: Create ts demux thread failed !!!\n", __FILE__, __LINE__ );
    }

    pthread_t h264_encoder_thread;
    if(index == 0){
        if( 0 != pthread_create( &h264_encoder_thread, NULL, video_encoder, this ))
            printf("%s:%d  Error: Create video encoder thread failed !!!\n", __FILE__, __LINE__ );
    }

    pthread_detach(h264_encoder_thread);
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

void *udpsocket::video_encoder(void *pArg)
{
    udpsocket* pTemp = (udpsocket*) pArg;
    if( pTemp )
        pTemp->run_video_encoder();
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
    AVFrame *pVideoframe[VIDEO_NUM];
    AVFrame *pAudioframe[AUDIO_NUM];
    AVFrame *pOutAudioframe[AUDIO_NUM];
    AVFrame *pOutAudioframelast[AUDIO_NUM];
    AVPacket pkt;
    int got_picture;
    int video_num[VIDEO_NUM];
    int audio_num[AUDIO_NUM];
    int frame_size;

    //transcodepool
    transcodepool*  pVideoTransPool[VIDEO_NUM];
    transcodepool*  pAudioTransPool[AUDIO_NUM];

    for( int i=0; i<VIDEO_NUM; i++ ){
        pVideoCodec[i] = NULL;
        pVideoCodecCtx[i] =NULL;
        videoindex[i] = -1;
        pVst[i] = NULL;
        video_num[i] = 0;
        pVideoframe[i] = NULL;
        pVideoframe[i] = av_frame_alloc();
        pVideoTransPool[i] = NULL;
    }
    for( int i=0; i<AUDIO_NUM; i++ ){
        pAudioCodec[i] = NULL;
        pAudioCodecCtx[i] = NULL;
        audioindex[i] = -1;
        pAst[i] = NULL;
        audio_num[i] = 0;
        pOutAudioframe[i] = NULL;
        pOutAudioframe[i] = av_frame_alloc();
        pOutAudioframelast[i] = NULL;
        pOutAudioframelast[i] = av_frame_alloc();
        pAudioframe[i] = NULL;
        pAudioframe[i] = av_frame_alloc();
        pAudioTransPool[i] = NULL;
    }
    pb = NULL;
    piFmt = NULL;
    pFmt = NULL;
    buffer = (uint8_t*)av_mallocz(sizeof(uint8_t)*BUFFER_SIZE);
    got_picture = 0;
    frame_size = AVCODEC_MAX_AUDIO_FRAME_SIZE*3/2;

    //encoder
    AVFormatContext *ofmt_ctx = NULL;
    AVPacket enc_pkt;
    int enc_got_frame = 0;
    AVStream *out_stream;
    AVCodecContext *enc_ctx;
    AVCodec *encoder;

    AVFormatContext *outAudioFormatCtx[AUDIO_NUM];
    AVPacket audio_pkt;
    int audio_got_frame = 0;
    AVStream *audio_stream[AUDIO_NUM];
    AVCodecContext *AudioEncodeCtx[AUDIO_NUM];
    AVCodec *AudioEncoder[AUDIO_NUM];

    fp_v = fopen("OUT.h264","wb+"); //输出文件
    fp_a = fopen("audio_out.aac","wb+");

    //FFMPEG
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

    for( int i=0; i<AUDIO_NUM; i++ ){
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

    //video encoder init
    avformat_alloc_output_context2(&ofmt_ctx, NULL, "h264", NULL);
    unsigned char* outbuffer = NULL;
    outbuffer = (unsigned char*)av_malloc(1024*1000);
    AVIOContext *avio_out = NULL;
    avio_out = avio_alloc_context(outbuffer, 1024*1000, 0, NULL, NULL, write_buffer,NULL);
    if(avio_out == NULL){
        printf("avio_out error\n");
        return -1;
    }
    ofmt_ctx->pb = avio_out;
    ofmt_ctx->flags = AVFMT_FLAG_CUSTOM_IO;
    out_stream = avformat_new_stream(ofmt_ctx, NULL);
    if(!out_stream){
        av_log(NULL, AV_LOG_ERROR, "Failed allocating output stream\n");
        return -1;
    }
    enc_ctx = out_stream->codec;
    encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
    enc_ctx->height = pVideoCodecCtx[0]->height;
    enc_ctx->width = pVideoCodecCtx[0]->width;
    enc_ctx->sample_aspect_ratio = pVideoCodecCtx[0]->sample_aspect_ratio;
    enc_ctx->pix_fmt = encoder->pix_fmts[0];
    out_stream->time_base = pVst[0]->time_base;
//    out_stream->time_base.num = 1;
//    out_stream->time_base.den = 25;
    enc_ctx->me_range = 16;
    enc_ctx->max_qdiff = 4;
    enc_ctx->qmin = 25;
    enc_ctx->qmax = 40;
    enc_ctx->qcompress = 0.6;
    enc_ctx->refs = 3;
    enc_ctx->bit_rate = 1000000;
    int re = avcodec_open2(enc_ctx, encoder, NULL);
    if (re < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open video encoder for stream \n");
        return re;
    }

    if(ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        enc_ctx->flags |= CODEC_FLAG_GLOBAL_HEADER;
    re = avformat_write_header(ofmt_ctx, NULL);
    if(re < 0){
        av_log(NULL, AV_LOG_ERROR, "Error occured when opening output file\n");
        return re;
    }

    //audio encoder
    for( int i=0; i<AUDIO_NUM; i++){
        outAudioFormatCtx[i] = NULL;
//        audio_pkt = NULL;
        audio_stream[i] = NULL;
        AudioEncodeCtx[i] = NULL;
        AudioEncoder[i] = NULL;
    }
    const char* out_audio_file = "transcodeaudio.aac";          //Output URL

    //Method 1.
    outAudioFormatCtx[0] = avformat_alloc_context();
    outAudioFormatCtx[0]->oformat = av_guess_format(NULL, out_audio_file, NULL);

    //Method 2.
    //avformat_alloc_output_context2(&pFormatCtx, NULL, NULL, out_file);
    //fmt = pFormatCtx->oformat;

    //Open output URL
    if (avio_open(&outAudioFormatCtx[0]->pb,out_audio_file, AVIO_FLAG_READ_WRITE) < 0){
        printf("Failed to open output file!\n");
        return -1;
    }

    audio_stream[0] = avformat_new_stream(outAudioFormatCtx[0], 0);
    if (audio_stream[0]==NULL){
        return -1;
    }
    AudioEncodeCtx[0] = audio_stream[0]->codec;
    AudioEncodeCtx[0]->codec_id =  outAudioFormatCtx[0]->oformat->audio_codec;
    AudioEncodeCtx[0]->codec_type = AVMEDIA_TYPE_AUDIO;
    AudioEncodeCtx[0]->sample_fmt = AV_SAMPLE_FMT_S16;
    AudioEncodeCtx[0]->sample_rate= 44100;
    AudioEncodeCtx[0]->channel_layout=AV_CH_LAYOUT_STEREO;
    AudioEncodeCtx[0]->channels = av_get_channel_layout_nb_channels(AudioEncodeCtx[0]->channel_layout);
    AudioEncodeCtx[0]->bit_rate = 64000;

    //Show some information
    av_dump_format(outAudioFormatCtx[0], 0, out_audio_file, 1);

    AudioEncoder[0] = avcodec_find_encoder(AudioEncodeCtx[0]->codec_id);
    if (!AudioEncoder[0]){
        printf("Can not find encoder!\n");
        return -1;
    }
    if (avcodec_open2(AudioEncodeCtx[0], AudioEncoder[0],NULL) < 0){
        printf("Failed to open encoder!\n");
        return -1;
    }
//    pFrame = av_frame_alloc();
//    pFrame->nb_samples= pCodecCtx->frame_size;
//    pFrame->format= pCodecCtx->sample_fmt;

//    int size = av_samples_get_buffer_size(NULL, AudioEncodeCtx[0]->channels,AudioEncodeCtx[0]->frame_size,AudioEncodeCtx[0]->sample_fmt, 1);
//    uint8_t *frame_buf = (uint8_t *)av_malloc(size);
//    avcodec_fill_audio_frame(pFrame, pCodecCtx->channels, pCodecCtx->sample_fmt,(const uint8_t*)frame_buf, size, 1);

    //uint8_t samples[AVCODEC_MAX_AUDIO_FRAME_SIZE*3/2];
    av_init_packet(&pkt);
    av_init_packet(&audio_pkt);
    av_init_packet(&enc_pkt);
    AVSampleFormat SAMPLE_FMT = AV_SAMPLE_FMT_S16;
    int CHANNEL_LAYOUT = AV_CH_LAYOUT_STEREO;
    AVAudioFifo *af = NULL;
    af = av_audio_fifo_alloc(SAMPLE_FMT, av_get_channel_layout_nb_channels(CHANNEL_LAYOUT), 1);
    if(af == NULL)
    {
        printf("error af \n");
        return -1;
    }
    int nb_last = 0;
    printf("start decode\n");
    while(1) {
        if (av_read_frame(pFmt, &pkt) >= 0) {
            for( int i=0; i<1; i++ ){
                if (pkt.stream_index == videoindex[i]) {
//                    av_frame_free(&pframe);
                    avcodec_decode_video2(pVideoCodecCtx[i], pVideoframe[i], &got_picture, &pkt);
                    if (got_picture) {
                        if(videoindex[i] == 0){
//                            m_tsRecvPool->write_buffer(pkt.data, pkt.size);
                            pVideoframe[i]->pts = av_frame_get_best_effort_timestamp(pVideoframe[i]);
                            pVideoframe[i]->pict_type = AV_PICTURE_TYPE_NONE;
//                            pVideoTransPool[i]->PutFrame( pVideoframe[i]->data, pVideoframe[i]->size, pVideoframe[i]->pts );

                            /*  ffmpeg encoder */
//                            enc_pkt.data = NULL;
//                            enc_pkt.size = 0;
//                            av_init_packet(&enc_pkt);
//                            re = avcodec_encode_video2(ofmt_ctx->streams[videoindex[i]]->codec, &enc_pkt,
//                                    pVideoframe[i], &enc_got_frame);
//                            printf("enc_got_frame =%d, re = %d \n",enc_got_frame, re);
//                            printf("Encode 1 Packet\tsize:%d\tpts:%lld\n",enc_pkt.size,enc_pkt.pts);
                            /* prepare packet for muxing */
//                            fwrite(enc_pkt.data,enc_pkt.size, 1, fp_v);
                        }
                        printf("index = %d video %d decode %d num\n", index, i, video_num[i]++);
                        break;
                    }

                 }else if (pkt.stream_index == audioindex[i]) {
                    if (avcodec_decode_audio4(pAudioCodecCtx[i], pAudioframe[i], &frame_size, &pkt) >= 0) {
                        if (i == 0){
//                            pAudioframe[i]->data[0] = frame_buf;

                            printf("framesize = %d, audioframesize = %d\n", pAudioframe[i]->nb_samples, AudioEncodeCtx[0]->frame_size);
                            audio_pkt.data = NULL;
                            audio_pkt.size = 0;
                            av_init_packet(&audio_pkt);

                            pOutAudioframe[i]->nb_samples =  AudioEncodeCtx[0]->frame_size;
                            pOutAudioframe[i]->channel_layout = CHANNEL_LAYOUT;
                            pOutAudioframe[i]->sample_rate = pAudioframe[i]->sample_rate;
                            pOutAudioframe[i]->format = SAMPLE_FMT;
                            av_frame_get_buffer(pOutAudioframe[i], 1);

                            pOutAudioframe[i]->pts=av_frame_get_best_effort_timestamp(pAudioframe[i]);
                            pOutAudioframe[i]->pict_type = AV_PICTURE_TYPE_NONE;
                            int got_frame=0;
                            av_audio_fifo_write(af, (void **)&pAudioframe[i]->data, pAudioframe[i]->nb_samples);
                            av_audio_fifo_read(af, (void **)&pOutAudioframe[i]->data, AudioEncodeCtx[0]->frame_size);
                            nb_last += pAudioframe[i]->nb_samples - AudioEncodeCtx[0]->frame_size;
                            //Encode
                            av_init_packet(&audio_pkt);
                            audio_pkt.data = NULL;
                            audio_pkt.size = 0;
                            avcodec_encode_audio2(AudioEncodeCtx[0], &audio_pkt, pOutAudioframe[i], &got_frame);
                            printf("Encode 1 Packet\tsize:%d\tpts:%lld\n", audio_pkt.size, audio_pkt.pts);
                            fwrite(audio_pkt.data,audio_pkt.size, 1, fp_a);
                            if(nb_last >= AudioEncodeCtx[0]->frame_size){
                                printf(" nb_last = %d \n",nb_last);
                                pOutAudioframelast[i]->nb_samples =  AudioEncodeCtx[0]->frame_size;
                                pOutAudioframelast[i]->channel_layout = CHANNEL_LAYOUT;
                                pOutAudioframelast[i]->sample_rate = pAudioframe[i]->sample_rate;
                                pOutAudioframelast[i]->format = SAMPLE_FMT;
                                av_frame_get_buffer(pOutAudioframelast[i], 1);

//                                pOutAudioframelast[i]->pts=av_frame_get_best_effort_timestamp(pAudioframe[i]);
                                pOutAudioframelast[i]->pict_type = AV_PICTURE_TYPE_NONE;

                                av_audio_fifo_read(af, (void **)&pOutAudioframelast[i]->data, AudioEncodeCtx[0]->frame_size);
                                nb_last = nb_last - AudioEncodeCtx[0]->frame_size;
                                av_init_packet(&audio_pkt);
                                audio_pkt.data = NULL;
                                audio_pkt.size = 0;
                                avcodec_encode_audio2(AudioEncodeCtx[0], &audio_pkt, pOutAudioframelast[i], &got_frame);
                                printf("Encode 1 Packet\tsize:%d\tpts:%lld\n", audio_pkt.size, audio_pkt.pts);
                                fwrite(audio_pkt.data,audio_pkt.size, 1, fp_a);
                            }
                        }
//                        if(i == 0){
//                            fwrite(pkt.data,pkt.size, 1, fp_a);
//                        }
//                        printf("index = %d audio %d decode %d num\n", index, i, audio_num[i]++);
                        break;
                    }
                }
            }
            av_free_packet(&pkt);
            av_free_packet(&enc_pkt);
        }
    }

    av_free(buffer);
    for(int i=0; i<VIDEO_NUM; i++)
        av_free(pVideoframe[i]);

    for(int i=0; i<AUDIO_NUM; i++)
        av_free(pAudioframe[i]);

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

void udpsocket::run_video_encoder(){
//    if( m_bStopEncoder )
//        return;

//    bool bInitVideoEncoder = true;
//    while( 1 )
//    {
//        sInputParams Params;
//        mfxStatus sts = MFX_ERR_NONE;
//        // Init video encoder
//        if( bInitVideoEncoder )
//        {
//            sts = InitVideoEncoder( &Params );
//            if( sts != MFX_ERR_NONE  )
//                return;
//        }

//        sts = m_pVideoEncoder->Run();
//        if ( MFX_ERR_DEVICE_LOST == sts || MFX_ERR_DEVICE_FAILED == sts )
//        {
//            bInitVideoEncoder = false;
//            sts = m_pVideoEncoder->ResetMFXComponents( &Params );
//            continue;
//        }
//        else if( ( int )sts == 10000 )
//        {
//            // Pause video encoder
//            PauseVideoEncoderThread();

//            // Exit application
//            if( m_bExitApplication )
//                return;

//            bInitVideoEncoder = true;
//            continue;
//        }
//        else
//            break;
//    }

//    if( m_pVideoEncoder )
//        m_pVideoEncoder->Close();
}

