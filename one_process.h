#ifndef ONE_PROCESS_H
#define ONE_PROCESS_H

#include <pthread.h>
#include <stdio.h>
#include "audio_encode.h"
#include "transcodepool.h"

class one_process
{
public:
    one_process();
    ~one_process();
    void Init(int index);
    static void* video_encoder(void *param);
    void run_video_encoder(void);
    static void* audio_encoder(void *param);
    void run_audio_encoder(void);

private:
    bool                        m_bExitApplication;
    bool                        m_bStopEncoder;
    int                          m_index;
    transcodepool       *m_TranscodePool[7];
    audio_encode         m_AudioEncoder;
    pthread_mutex_t        m_Mutex;
    pthread_cond_t          m_Cond;
};

#endif // ONE_PROCESS_H
