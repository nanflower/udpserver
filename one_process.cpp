#include "one_process.h"

one_process::one_process( int index)
{
    m_index = index;
    m_bStopEncoder = false;
    m_pVideoEncoder = NULL;
    InitEncoderPar();
    pthread_mutex_init( &m_Mutex, NULL );
    pthread_cond_init( &m_Cond, NULL );

    m_bStopDecoder = false;
    m_pVideoDecoder = NULL;
//    InitDecoderPar();
    pthread_mutex_init( &m_DeMutex, NULL );
    pthread_cond_init( &m_DeCond, NULL );
}

one_process::~one_process()
{
    DestroyVideoEncoder();
    DestroyVideoDecoder();
}

void one_process::Init(void)
{

    pthread_t mpeg2_decode_thread;
    memset( &mpeg2_decode_thread, 0, sizeof( mpeg2_decode_thread ) );

    if( 0 != pthread_create( &mpeg2_decode_thread, NULL, video_decoder, this ))
        printf("%s:%d  Error: Create video encoder thread failed !!!\n", __FILE__, __LINE__ );

    pthread_detach(mpeg2_decode_thread);

    pthread_t h264_encoder_thread;
    memset( &h264_encoder_thread, 0, sizeof( h264_encoder_thread ) );

    if( 0 != pthread_create( &h264_encoder_thread, NULL, video_encoder, this ))
        printf("%s:%d  Error: Create video encoder thread failed !!!\n", __FILE__, __LINE__ );

    pthread_detach(h264_encoder_thread);

//    pthread_t udp_send_thread;
//    memset( &udp_send_thread, 0, sizeof( udp_send_thread ) );

//    if( 0 != pthread_create( &udp_send_thread, NULL, udp_send, this ))
//        printf("%s:%d  Error: Create video encoder thread failed !!!\n", __FILE__, __LINE__ );

//    pthread_detach(udp_send_thread);
}

void* one_process::video_decoder(void *pArg){
    one_process* pTemp = (one_process*)pArg;
    if( pTemp )
        pTemp->run_video_decoder();
    return (void*)NULL;
}

void one_process::run_video_decoder(){
    if( m_bStopDecoder )
        return;
    printf("video decoder work\n");
    bool bInitVideoDecoder = true;

    sInputParams Params;
    mfxStatus sts = MFX_ERR_NONE;
    // Init video encoder
    if( bInitVideoDecoder )
    {
        sts = InitVideoDecoder( &Params );
        if( sts != MFX_ERR_NONE  )
            return;
    }
    printf("Init over \n");
    while( 1 )
    {

        sts = m_pVideoDecoder->RunDecoding();
        if ( MFX_ERR_DEVICE_LOST == sts || MFX_ERR_DEVICE_FAILED == sts )
        {
            bInitVideoDecoder = false;
            sts = m_pVideoDecoder->ResetDecoder(&Params);
            continue;
        }
        else if( ( int )sts == 10000 )
        {
            // Pause video encoder
            PauseVideoDecoderThread();

            // Exit application
            if( m_bExitDeApplication )
                return;

            bInitVideoDecoder = true;
            continue;
        }
        else
            break;
    }

    if( m_pVideoDecoder )
        m_pVideoDecoder->Close();
}

void* one_process::video_encoder(void *pArg){
    one_process* pTemp = (one_process*)pArg;
    if( pTemp )
        pTemp->run_video_encoder();
    return (void*)NULL;
}

void one_process::run_video_encoder(){
    if( m_bStopEncoder )
        return;
    printf("video encoder work\n");
    bool bInitVideoEncoder = true;
    while( 1 )
    {
        sParams Params;
        mfxStatus sts = MFX_ERR_NONE;
        // Init video encoder
        if( bInitVideoEncoder )
        {
            sts = InitVideoEncoder( &Params );
            if( sts != MFX_ERR_NONE  )
                return;
        }

        sts = m_pVideoEncoder->Run();
        if ( MFX_ERR_DEVICE_LOST == sts || MFX_ERR_DEVICE_FAILED == sts )
        {
            bInitVideoEncoder = false;
            sts = m_pVideoEncoder->ResetMFXComponents( &Params );
            continue;
        }
        else if( ( int )sts == 10000 )
        {
            // Pause video encoder
            PauseVideoEncoderThread();

            // Exit application
            if( m_bExitApplication )
                return;

            bInitVideoEncoder = true;
            continue;
        }
        else
            break;
    }

    if( m_pVideoEncoder )
        m_pVideoEncoder->Close();
}

void* one_process::udp_send(void *pArg){
    one_process* pTemp = (one_process*)pArg;
    if( pTemp )
        pTemp->run_udp_send();
    return (void*)NULL;
}

void one_process::run_udp_send(){
    /* 创建UDP套接口 */
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    //server_addr.sin_addr.s_addr = inet_addr("1.8.84.12");
    server_addr.sin_port = htons(10791);

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
         uint8_t *buffer[BUFFER_SIZE];
         bzero(buffer, BUFFER_SIZE);
//         struct timeval tv;
//         fd_set readfds;
//         tv.tv_sec = 3;
//         tv.tv_usec = 10;
//         FD_ZERO(&readfds);
//         FD_SET(server_socket_fd, &readfds);
//         select(server_socket_fd+1,&readfds,NULL, NULL, &tv);

//         if (FD_ISSET(server_socket_fd,&readfds))
//         {
        int len = sendto(server_socket_fd, buffer, BUFFER_SIZE,0,(struct sockaddr*)&client_addr, sizeof(client_addr_length));
        if (len == -1)
        {
            printf("send data error!\n");
        }
//         }
//         else
//         {
//             printf("error is %d\n",errno);
//             printf("timeout!there is no data arrived!\n");
//         }
    }
}

mfxStatus one_process::InitVideoDecoder( sInputParams *pParams )
{
    MSDK_CHECK_POINTER( pParams, MFX_ERR_NULL_PTR );
    if( NULL ==  m_pVideoDecoder )
    {
        m_pVideoDecoder = new CDecodingPipeline();
        m_pVideoDecoder->m_deviceid = m_index;
        MSDK_CHECK_POINTER( m_pVideoDecoder, MFX_ERR_MEMORY_ALLOC );
    }

    MSDK_ZERO_MEMORY( *pParams );
    InitVideoDecoderParam( pParams );
    printf("init decoder\n");
    return m_pVideoDecoder->Init( pParams );
}


void one_process::InitVideoDecoderParam( sInputParams *pParams )
{
    if( NULL == pParams )
        return;
    pParams->decode = g_decodeDevice[0];
    pParams->transcode = g_pCaptureDeviceVec[0];
    pParams->decodeID = m_index;
    pParams->videoType = MFX_CODEC_MPEG2;
    pParams->bUseHWLib = false;
    if(pParams->nAsyncDepth == 0)
        pParams->nAsyncDepth = 4;
//    pParams->width = 720;
//    pParams->height = 576;
//    pParams->fourcc = MFX_FOURCC_NV12;

}

void one_process::PauseVideoDecoderThread()
{
    pthread_mutex_lock( &m_DeMutex );
    while ( m_bStopDecoder )
    {
        DestroyVideoDecoder();
        pthread_cond_wait( &m_DeCond, &m_DeMutex );
    }
    pthread_mutex_unlock( &m_DeMutex );
}

void one_process::DestroyVideoDecoder()
{
    if( m_pVideoDecoder )
    {
        delete m_pVideoDecoder;
        m_pVideoDecoder = NULL;
    }
}

mfxStatus one_process::InitVideoEncoder( sParams *pParams )
{
    MSDK_CHECK_POINTER( pParams, MFX_ERR_NULL_PTR );
    if( NULL ==  m_pVideoEncoder )
    {
        m_pVideoEncoder = new CEncodingPipeline();
        m_pVideoEncoder->m_deviceid = m_index;
        MSDK_CHECK_POINTER( m_pVideoEncoder, MFX_ERR_MEMORY_ALLOC );
    }

    MSDK_ZERO_MEMORY( *pParams );
    InitVideoEncoderParam( pParams );
    return m_pVideoEncoder->Init( pParams );
}

void one_process::InitEncoderPar()
{
    m_EncoderParInfo.bLookAhead = true;
    m_EncoderParInfo.bUseHWLib = false;
    m_EncoderParInfo.nAsyncDepth = 1;
    m_EncoderParInfo.nBitRate = 1000;
    m_EncoderParInfo.nChannelNum = 0;
    m_EncoderParInfo.nCodecLevel = 40;
    m_EncoderParInfo.nCodecProfile = 100;
    m_EncoderParInfo.nFrameRate = 25;
    m_EncoderParInfo.nGopOptFlag = MFX_GOP_STRICT;
    m_EncoderParInfo.nGopPicSize = 24;
    m_EncoderParInfo.nGopRefDist = 6;
//    m_EncoderParInfo.nLADepth = 49;
    m_EncoderParInfo.nNumSlice = 1;
    m_EncoderParInfo.nPicStruct = 0;
//    m_EncoderParInfo.nRateControlMethod = MFX_RATECONTROL_LA_ICQ;
    m_EncoderParInfo.nRateControlMethod = MFX_RATECONTROL_CQP;
    m_EncoderParInfo.nTargetUsage = 4;
    m_EncoderParInfo.strCodecId = QString("AVC");
    m_EncoderParInfo.strColorFormat = QString("");
}

void one_process::InitVideoEncoderParam( sParams *pParams )
{
    if( NULL == pParams )
        return;
    pParams->ndeviceid = m_index;
    pParams->vd = g_pCaptureDeviceVec[0];
    pParams->nCodecLevel = m_EncoderParInfo.nCodecLevel;
    pParams->nCodecProfile = m_EncoderParInfo.nCodecProfile;
    pParams->nPicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    pParams->nRateControlMethod = m_EncoderParInfo.nRateControlMethod;
    pParams->nGopRefDist = m_EncoderParInfo.nGopRefDist;
    pParams->nGopPicSize = m_EncoderParInfo.nGopPicSize;
    pParams->nNumSlice = m_EncoderParInfo.nNumSlice;
    pParams->nTargetUsage = m_EncoderParInfo.nTargetUsage;
    pParams->nGopOptFlag = m_EncoderParInfo.nGopOptFlag;
    pParams->nWidth = 720;
    pParams->nHeight = 576;
    GetQuality( pParams->nDstWidth, pParams->nDstHeight);
//    pParams->nLADepth = m_EncoderParInfo.nLADepth;
    pParams->CodecId = MFX_CODEC_AVC;//GetCodecId();
    pParams->dFrameRate = m_EncoderParInfo.nFrameRate;
    pParams->bUseHWLib = true;//m_EncoderParInfo.bUseHWLib;
    pParams->nBitRate = m_EncoderParInfo.nBitRate;//m_EncoderParInfo.nBitRate;
    pParams->ColorFormat = MFX_FOURCC_YV12;//GetColorFormat();
    pParams->nAsyncDepth = m_EncoderParInfo.nAsyncDepth;
    pParams->MVC_flags = MVC_VIEWOUTPUT;
}

void one_process::GetQuality( unsigned short &nDstWidth, unsigned short &nDstHeight )
{

        nDstWidth = 720;
        nDstHeight = 576;

}

void one_process::PauseVideoEncoderThread()
{
    pthread_mutex_lock( &m_Mutex );
    while ( m_bStopEncoder )
    {
        DestroyVideoEncoder();
        pthread_cond_wait( &m_Cond, &m_Mutex );
    }
    pthread_mutex_unlock( &m_Mutex );
}

void one_process::DestroyVideoEncoder()
{
    if( m_pVideoEncoder )
    {
        delete m_pVideoEncoder;
        m_pVideoEncoder = NULL;
    }
}

