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

    m_tsRecvPool = new tspoolqueue;
    fp_write=fopen("cuc60anniversary_start.h264","wb+"); //输出文件
}

udpsocket::~udpsocket()
{

}

void udpsocket::thread_init(int index)
{

    //init_buffer();
    protindex = index;

    pthread_t udp_recv_thread;
    memset( &udp_recv_thread, 0, sizeof( udp_recv_thread ) );
    if( 0 != pthread_create( &udp_recv_thread, NULL, udp_tsrecv, this ) )
        printf("%s:%d  Error: Create udp receive thread failed !!!\n", __FILE__, __LINE__ );

    pthread_t ts_demux_thread;
    memset( &ts_demux_thread, 0, sizeof( ts_demux_thread ) );

    if( 0 != pthread_create( &ts_demux_thread, NULL, ts_demuxer, this ) )
        printf("%s:%d  Error: Create ts demux thread failed !!!\n", __FILE__, __LINE__ );

    pthread_detach(ts_demux_thread);
    pthread_detach(udp_recv_thread);

    m_transProcess = new one_process(0);
    m_transProcess->Init();
}

void *udpsocket::udp_tsrecv(void *pArg)
{
    udpsocket* pTemp = (udpsocket*)pArg;
    if( pTemp )
        pTemp->udp_ts_recv();
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
    decodepool*      pVideoDecodePool[VIDEO_NUM];

    for(    int i=0; i<1; i++){
        pVideoTransPool[i]->Init();
        pVideoDecodePool[i]->Init(i);
    }

    for( int i=0; i<VIDEO_NUM; i++ ){
        pVideoCodec[i] = NULL;
        pVideoCodecCtx[i] =NULL;
        videoindex[i] = -1;
        pVst[i] = NULL;
        video_num[i] = 0;
        pVideoframe[i] = NULL;
        pVideoframe[i] = av_frame_alloc();
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
    }
    pb = NULL;
    piFmt = NULL;
    pFmt = NULL;
    buffer = (uint8_t*)av_mallocz(sizeof(uint8_t)*BUFFER_SIZE);
    got_picture = 0;
    frame_size = AVCODEC_MAX_AUDIO_FRAME_SIZE*3/2;

    //encoder
//    AVFormatContext *ofmt_ctx = NULL;
//    AVPacket enc_pkt;
//    AVStream *out_stream;
//    AVCodecContext *enc_ctx;
//    AVCodec *encoder;

    AVFormatContext *outAudioFormatCtx[AUDIO_NUM];
    AVPacket audio_pkt;
    AVStream *audio_stream[AUDIO_NUM];
    AVCodecContext *AudioEncodeCtx[AUDIO_NUM];
    AVCodec *AudioEncoder[AUDIO_NUM];

//    fp_v = fopen("OUT.yuv","ab+"); //输出文件
//    fp_a = fopen("audio_out.aac","wb+");

    //FFMPEG
    av_register_all();

    pb = avio_alloc_context(buffer, 4096, 0, NULL, read_data, NULL, NULL);
    printf("thread %d pid %lu tid %lu\n",index,(unsigned long)getpid(),(unsigned long)pthread_self());

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

//    if (videoindex[6] < 0 || audioindex[6] < 0) {
//        fprintf(stderr, "videoindex=%d, audioindex=%d\n", videoindex[6], audioindex[6]);
//        return -1;
//    }

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
    unsigned char* outbuffer = NULL;
    outbuffer = (unsigned char*)av_malloc(1024*1000);
    //video encoder init
//    avformat_alloc_output_context2(&ofmt_ctx, NULL, "h264", NULL);
//    AVIOContext *avio_out = NULL;
//    avio_out = avio_alloc_context(outbuffer, 1024*1000, 0, NULL, NULL, write_buffer,NULL);
//    if(avio_out == NULL){
//        printf("avio_out error\n");
//        return -1;
//    }
//    ofmt_ctx->pb = avio_out;
//    ofmt_ctx->flags = AVFMT_FLAG_CUSTOM_IO;
//    out_stream = avformat_new_stream(ofmt_ctx, NULL);
//    if(!out_stream){
//        av_log(NULL, AV_LOG_ERROR, "Failed allocating output stream\n");
//        return -1;
//    }
//    enc_ctx = out_stream->codec;
//    encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
//    enc_ctx->height = pVideoCodecCtx[0]->height;
//    enc_ctx->width = pVideoCodecCtx[0]->width;
//    enc_ctx->sample_aspect_ratio = pVideoCodecCtx[0]->sample_aspect_ratio;
//    enc_ctx->pix_fmt = encoder->pix_fmts[0];
//    out_stream->time_base = pVst[0]->time_base;
////    out_stream->time_base.num = 1;
////    out_stream->time_base.den = 25;
//    enc_ctx->me_range = 16;
//    enc_ctx->max_qdiff = 4;
//    enc_ctx->qmin = 25;
//    enc_ctx->qmax = 40;
//    enc_ctx->qcompress = 0.6;
//    enc_ctx->refs = 3;
//    enc_ctx->bit_rate = 1000000;
//    int re = avcodec_open2(enc_ctx, encoder, NULL);
//    if (re < 0) {
//        av_log(NULL, AV_LOG_ERROR, "Cannot open video encoder for stream \n");
//        return re;
//    }

//    if(ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
//        enc_ctx->flags |= CODEC_FLAG_GLOBAL_HEADER;
//    re = avformat_write_header(ofmt_ctx, NULL);
//    if(re < 0){
//        av_log(NULL, AV_LOG_ERROR, "Error occured when opening output file\n");
//        return re;
//    }

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
    for(int i=0; i<AUDIO_NUM; i++){
        outAudioFormatCtx[i] = avformat_alloc_context();
        outAudioFormatCtx[i]->oformat = av_guess_format(NULL, out_audio_file, NULL);
        AVIOContext *avio_audio_out = NULL;
        avio_audio_out = avio_alloc_context(outbuffer, 1024*1000, 0, NULL, NULL, write_buffer,NULL);
        if(avio_audio_out == NULL){
            printf("avio_out error\n");
            return -1;
        }
        outAudioFormatCtx[i]->pb = avio_audio_out;
        //Method 2.
        //avformat_alloc_output_context2(&pFormatCtx, NULL, NULL, out_file);
        //fmt = pFormatCtx->oformat;

        //Open output URL
        if (avio_open(&outAudioFormatCtx[i]->pb,out_audio_file, AVIO_FLAG_READ_WRITE) < 0){
            printf("Failed to open output file!\n");
            return -1;
        }

        //Show some information
        av_dump_format(outAudioFormatCtx[i], 0, out_audio_file, 1);

        AudioEncoder[i] = avcodec_find_encoder(AV_CODEC_ID_AAC);
        if (!AudioEncoder[i]){
            printf("Can not find encoder!\n");
            return -1;
        }
        audio_stream[i] = avformat_new_stream(outAudioFormatCtx[i], AudioEncoder[i]);
        if (audio_stream[i]==NULL){
            return -1;
        }
        AudioEncodeCtx[i] = audio_stream[i]->codec;
        AudioEncodeCtx[i]->codec_id =  outAudioFormatCtx[i]->oformat->audio_codec;
        AudioEncodeCtx[i]->codec_type = AVMEDIA_TYPE_AUDIO;
        AudioEncodeCtx[i]->sample_fmt = AV_SAMPLE_FMT_S16;
        AudioEncodeCtx[i]->sample_rate= 48000;//44100
        AudioEncodeCtx[i]->channel_layout=AV_CH_LAYOUT_STEREO;
        AudioEncodeCtx[i]->channels = av_get_channel_layout_nb_channels(AudioEncodeCtx[i]->channel_layout);
        AudioEncodeCtx[i]->bit_rate = 64000;//64000
        /** Allow the use of the experimental AAC encoder */
        AudioEncodeCtx[i]->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

        /** Set the sample rate for the container. */
        audio_stream[i]->time_base.den = pAudioCodecCtx[i]->sample_rate;
        audio_stream[i]->time_base.num = 1;

        if (avcodec_open2(AudioEncodeCtx[i], AudioEncoder[i],NULL) < 0){
            printf("Failed to open encoder!\n");
            return -1;
        }

        av_samples_get_buffer_size(NULL, AudioEncodeCtx[i]->channels,AudioEncodeCtx[0]->frame_size,AudioEncodeCtx[0]->sample_fmt, 1);
    }

    //uint8_t samples[AVCODEC_MAX_AUDIO_FRAME_SIZE*3/2];
    av_init_packet(&pkt);
    av_init_packet(&audio_pkt);
//    av_init_packet(&enc_pkt);
    AVAudioFifo *af[AUDIO_NUM];
    SwrContext *resample_context[AUDIO_NUM];
    long long pts = 0;
    /** Initialize the resampler to be able to convert audio sample formats. */
//    if (init_resampler(pAudioCodecCtx[0], AudioEncodeCtx[0],
//                       &resample_context))
    for(int i=0; i<1; i++){
        printf(" samplerate input = %d , samplerate output = %d\n",pAudioCodecCtx[i]->sample_rate, AudioEncodeCtx[i]->sample_rate);
        resample_context[i] = swr_alloc_set_opts(NULL, av_get_default_channel_layout(AudioEncodeCtx[i]->channels),
                                                          AudioEncodeCtx[i]->sample_fmt,
                                                          AudioEncodeCtx[i]->sample_rate,
                                                          av_get_default_channel_layout(pAudioCodecCtx[i]->channels),
                                                          pAudioCodecCtx[i]->sample_fmt,
                                                          pAudioCodecCtx[i]->sample_rate,
                                                          0, NULL);
        swr_init(resample_context[i]);
        af[i] = av_audio_fifo_alloc(AudioEncodeCtx[i]->sample_fmt, AudioEncodeCtx[i]->channels, 1);
        if(af[i] == NULL)
        {
            printf("error af \n");
            return -1;
        }
    }
    printf("swr over\n");


    while(1) {
        if (av_read_frame(pFmt, &pkt) >= 0) {
            for( int i=0; i<1; i++ ){
                if (pkt.stream_index == videoindex[i]) {
//                    fwrite(pkt.data,pkt.size, 1, fp_v);
                    pVideoDecodePool[i]->putbuffer(&pkt, i);
//                    av_frame_free(&pframe);
//                    avcodec_decode_video2(pVideoCodecCtx[i], pVideoframe[i], &got_picture, &pkt);
//                    if (got_picture) {
//                        if(i == 0){
//                            pVideoframe[i]->pts = av_frame_get_best_effort_timestamp(pVideoframe[i]);
//                            pVideoframe[i]->pict_type = AV_PICTURE_TYPE_NONE;

////                            printf("pkt .size = %d , frame.size = %d, width = %d, height = %d\n", pkt.size, pVideoframe[i]->linesize[0], pVideoframe[i]->width, pVideoframe[i]->height);
////                            printf("pVideoframe->Y = %d, pVideoframe->WIDTH = %d\n", pVideoframe[i]->linesize[0], pVideoframe[i]->width);
////                            for(int j=0; j<pVideoframe[i]->height; j++)
////                                fwrite( pVideoframe[i]->data[0]  +  pVideoframe[i]->linesize[0]*j, pVideoframe[i]->width, 1, fp_v);
////                            for(int k=0; k<pVideoframe[i]->height/2; k++)
////                                fwrite( pVideoframe[i]->data[1] + pVideoframe[i]->linesize[1]*k, pVideoframe[i]->width/2, 1, fp_v);
////                            for(int l=0; l<pVideoframe[i]->height/2; l++)
////                                fwrite( pVideoframe[i]->data[2] + pVideoframe[i]->linesize[2]*l, pVideoframe[i]->width/2, 1, fp_v);
////                            m_tsRecvPool->write_buffer(pkt.data, pkt.size);

////                            printf("videoframesize0 = %d, size1 = %d, size2 = %d, size3 = %d, size4 = %d,format = %d\n",pVideoframe[i]->linesize[0],
////                                    pVideoframe[i]->linesize[1],pVideoframe[i]->linesize[2],pVideoframe[i]->linesize[3],pVideoframe[i]->linesize[4],pVideoframe[i]->format);
//                            pVideoTransPool[i]->PutFrame( pVideoframe[i] ,i);

//                            /*  ffmpeg encoder */
////                            enc_pkt.data = NULL;
////                            enc_pkt.size = 0;
////                            av_init_packet(&enc_pkt);
////                            re = avcodec_encode_video2(ofmt_ctx->streams[videoindex[i]]->codec, &enc_pkt,
////                                    pVideoframe[i], &got_picture);
////                            printf("enc_got_frame =%d, re = %d \n",enc_got_frame, re);
////                            printf("Encode 1 Packet\tsize:%d\tpts:%lld\n",enc_pkt.size,enc_pkt.pts);
//                            /* prepare packet for muxing */
////                            fwrite(enc_pkt.data,enc_pkt.size, 1, fp_v);
//                        }
////                        printf("index = %d video %d decode %d num\n", index, i, video_num[i]++);
//                        break;
//                    }

                 }else if (pkt.stream_index == audioindex[i]) {
                    if (avcodec_decode_audio4(pAudioCodecCtx[i], pAudioframe[i], &frame_size, &pkt) >= 0) {
                        if (i == 0){

                            uint8_t *converted_input_samples = NULL;
                            converted_input_samples = (uint8_t *)calloc(AudioEncodeCtx[i]->channels, sizeof(*converted_input_samples));
                            av_samples_alloc(&converted_input_samples, NULL, AudioEncodeCtx[i]->channels, pAudioframe[i]->nb_samples, AudioEncodeCtx[i]->sample_fmt, 0);
                                        int error = 0;
                            if((error = swr_convert(resample_context[i], &converted_input_samples, pAudioframe[i]->nb_samples,
                                                   (const uint8_t**)pAudioframe[i]->extended_data, pAudioframe[i]->nb_samples))<0){
                                printf("error  : %d\n",error);
                            }
                            av_audio_fifo_write(af[i], (void **)&converted_input_samples, pAudioframe[i]->nb_samples);

                            int got_frame=0;
                            //Encode
//                            av_init_packet(&audio_pkt);
//                            audio_pkt.data = NULL;
//                            audio_pkt.size = 0;
//                            avcodec_encode_audio2(AudioEncodeCtx[0], &audio_pkt, pOutAudioframe[i], &got_frame);
//                            printf("Encode 1 Packet\tsize:%d\tpts:%lld\n", audio_pkt.size, audio_pkt.pts);
                            while(av_audio_fifo_size(af[i]) >= AudioEncodeCtx[i]->frame_size){
                                int frame_size = FFMIN(av_audio_fifo_size(af[i]),AudioEncodeCtx[i]->frame_size);
                                pOutAudioframe[i]->nb_samples =  frame_size;
                                pOutAudioframe[i]->channel_layout = AudioEncodeCtx[i]->channel_layout;
                                pOutAudioframe[i]->sample_rate = AudioEncodeCtx[i]->sample_rate;
                                pOutAudioframe[i]->format = AudioEncodeCtx[i]->sample_fmt;

                                av_frame_get_buffer(pOutAudioframe[i], 0);
                                av_audio_fifo_read(af[i], (void **)&pOutAudioframe[i]->data, frame_size);

                                pOutAudioframe[i]->pts=pts;
                                pts += pOutAudioframe[i]->nb_samples;

                                audio_pkt.data = NULL;
                                audio_pkt.size = 0;
                                av_init_packet(&audio_pkt);
                                avcodec_encode_audio2(AudioEncodeCtx[i], &audio_pkt, pOutAudioframe[i], &got_frame);
                                printf("Encode 1 Packet\tsize:%d\tpts:%lld\n", audio_pkt.size, audio_pkt.pts);
//                                fwrite(audio_pkt.data,audio_pkt.size, 1, fp_a);
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
//            av_free_packet(&enc_pkt);
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
//    printf("thread 1 pid %lu tid %lu\n",(unsigned long)getpid(),(unsigned long)pthread_self());
    /* 创建UDP套接口 */
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    //server_addr.sin_addr.s_addr = inet_addr("1.8.84.12");
    int port = 50101 + protindex;
    server_addr.sin_port = htons(port);

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
