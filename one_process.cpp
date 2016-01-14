#include "one_process.h"

one_process::one_process()
{
    m_index = 0;
    m_bStopEncoder = false;
    m_pVideoEncoder = NULL;
    printf(" processxx \n");
    InitEncoderPar();
    pthread_mutex_init( &m_Mutex, NULL );
    pthread_cond_init( &m_Cond, NULL );
}

one_process::~one_process()
{
    DestroyVideoEncoder();
}

void one_process::Init(int index)
{
    m_index = index;

    pthread_t h264_encoder_thread;
    memset( &h264_encoder_thread, 0, sizeof( h264_encoder_thread ) );

    if( 0 != pthread_create( &h264_encoder_thread, NULL, video_encoder, this ))
        printf("%s:%d  Error: Create video encoder thread failed !!!\n", __FILE__, __LINE__ );

    pthread_detach(h264_encoder_thread);
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
    m_EncoderParInfo.nLADepth = 49;
    m_EncoderParInfo.nNumSlice = 1;
    m_EncoderParInfo.nPicStruct = 0;
    m_EncoderParInfo.nRateControlMethod = MFX_RATECONTROL_LA_ICQ;
    m_EncoderParInfo.nTargetUsage = 4;
    m_EncoderParInfo.strCodecId = QString("AVC");
    m_EncoderParInfo.strColorFormat = QString("");
}

void one_process::InitVideoEncoderParam( sParams *pParams )
{
    if( NULL == pParams )
        return;
    pParams->ndeviceid = m_index;
    pParams->vd = g_pCaptureDeviceVec[m_index];
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
    GetQuality( pParams->nDstWidth, pParams->nDstHeight, pParams->nICQQuality );
    pParams->nLADepth = m_EncoderParInfo.nLADepth;
    pParams->CodecId = MFX_CODEC_AVC;//GetCodecId();
    pParams->dFrameRate = m_EncoderParInfo.nFrameRate;
    pParams->bUseHWLib = true;//m_EncoderParInfo.bUseHWLib;
    pParams->nBitRate = m_EncoderParInfo.nBitRate;//m_EncoderParInfo.nBitRate;
    pParams->ColorFormat = MFX_FOURCC_YV12;//GetColorFormat();
    pParams->nAsyncDepth = m_EncoderParInfo.nAsyncDepth;
    pParams->MVC_flags = MVC_VIEWOUTPUT;
}

void one_process::GetQuality( unsigned short &nDstWidth, unsigned short &nDstHeight, unsigned short &nICQQuality )
{

 //   case EXCELLENT:
//        nDstWidth = 1280;
//        nDstHeight = 720;
//        nICQQuality = 35;
   //     break;
//    case GOOD:
//        nDstWidth = 1120;
//        nDstHeight = 630;
//        nICQQuality = 36;
//        break;
//    case FAIR:
        nDstWidth = 720;
        nDstHeight = 576;
        nICQQuality = 36;
//        break;
//    case POOR:
//        nDstWidth = 800;
//        nDstHeight = 468;
//        nICQQuality = 37;
//        break;
//    case MOBILE:
//        nDstWidth = 480;
//        nDstHeight = 270;
//        nICQQuality = 37;
//        break;
//    default:
//        break;

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

