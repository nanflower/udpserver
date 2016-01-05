#include "one_process.h"

one_process::one_process()
{

}

one_process::~one_process()
{

}

void one_process::Init(int index)
{
    m_index = index;
    pthread_t aac_encoder_thread;
    if( 0 != pthread_create( &aac_encoder_thread, NULL, audio_encoder, this ))
        printf("%s:%d  Error: Create video encoder thread failed !!!\n", __FILE__, __LINE__ );

    pthread_detach(aac_encoder_thread);

    pthread_t h264_encoder_thread;
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

}

void* one_process::audio_encoder(void *pArg){
    one_process* pTemp = (one_process*)pArg;
    if( pTemp )
        pTemp->run_audio_encoder();
    return (void*)NULL;
}

void one_process::run_audio_encoder(){
    if( m_bStopEncoder )
        return;

    int nCurAudioIndex = m_index+7;
    if( NULL == m_TranscodePool[nCurAudioIndex] )
        return;

    while( 1 )
    {
        // Pause audio encoder
        pthread_mutex_lock( &m_Mutex );
        while ( m_bStopEncoder )
            pthread_cond_wait( &m_Cond, &m_Mutex );
        pthread_mutex_unlock( &m_Mutex );

        // Exit application
        if( m_bExitApplication )
            return;

        // Get one audio frame
        unsigned char * pV4l2Buffer = NULL;
        unsigned long lBufferLen = 0;
        unsigned long lTimeStamp = 0;
        if( !m_TranscodePool[nCurAudioIndex]->GetFrame( (void **)&pV4l2Buffer,&lBufferLen, &lTimeStamp ))
            continue;

        // Start faac encoder
        if(  NULL != pV4l2Buffer && lBufferLen > 0 )
            m_AudioEncoder.StartFaacEnc( pV4l2Buffer, lBufferLen, lTimeStamp );
    }
}
