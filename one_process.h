#ifndef ONE_PROCESS_H
#define ONE_PROCESS_H

#include <pthread.h>
#include <stdio.h>
#include "transcodepool.h"
#include "outudppool.h"

#include "pipeline_encode.h"
#include "values.h"

#include <QObject>
#include <QTimer>

typedef struct tagEncoderParameterInfo
{
    int nChannelNum;
    int nCodecLevel;
    int nCodecProfile;
    int nPicStruct;
    int nRateControlMethod;
    int nGopRefDist;
    int nGopPicSize;
    int nNumSlice;
    int nTargetUsage;
    int nGopOptFlag;
    bool bLookAhead;
    int nLADepth;
    QString strCodecId;
    int nFrameRate;
    bool bUseHWLib;
    int nBitRate;
    QString strColorFormat;
    int nAsyncDepth;
} ENCODER_PAR_INFO,*PENCODER_PAR_INFO;

class CEncodingPipeline;
class one_process : public QObject
{
    Q_OBJECT
public:
    one_process();
    ~one_process();
    void Init(int index);
    static void* video_encoder(void *param);
    void run_video_encoder(void);


private:

    transcodepool *g_pCaptureDeviceVec[1];
    void InitEncoderPar();
    ENCODER_PAR_INFO    m_EncoderParInfo;

    mfxStatus InitVideoEncoder( sParams *pParams );
    void InitVideoEncoderParam( sParams *pParams);
    void GetResolution( unsigned short& nWidth, unsigned short& nHeight );
    void GetQuality( unsigned short& nDstWidth, unsigned short& nDstHeight, unsigned short& nICQQuality );
    mfxU32 GetCodecId();
    mfxU32 GetColorFormat();
    void PauseVideoEncoderThread();
    void DestroyVideoEncoder();

    CEncodingPipeline*   m_pVideoEncoder;
    bool                           m_bExitApplication;
    bool                           m_bStopEncoder;
    int                             m_index;
    transcodepool       *   m_TranscodePool[7];
    pthread_mutex_t        m_Mutex;
    pthread_cond_t          m_Cond;
};

#endif // ONE_PROCESS_H
