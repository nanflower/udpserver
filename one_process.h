#ifndef ONE_PROCESS_H
#define ONE_PROCESS_H

#include <pthread.h>
#include <stdio.h>
#include "transcodepool.h"
#include "outudppool.h"
#include "decodepool.h"

#include "pipeline_decode.h"
#include "pipeline_encode.h"
#include "values.h"

#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<netdb.h>
#include<stdarg.h>
#include<string.h>
#include<errno.h>

#include <QObject>
#include <QTimer>

#define BUFFER_SIZE 4096
#define VIDEO_TRANSNUM 7

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

typedef struct tagDecoderParameterInfo
{
    mfxU32 videoType;
    eWorkMode mode;
    bool    bUseHWLib; // true if application wants to use HW mfx library
    bool    bIsMVC; // true if Multi-View Codec is in use
    bool    bLowLat; // low latency mode
    bool    bCalLat; // latency calculation
    mfxU32  nMaxFPS; //rendering limited by certain fps
    mfxU32  nWallCell;
    mfxU32  nWallW; //number of windows located in each row
    mfxU32  nWallH; //number of windows located in each column
    mfxU32  nWallMonitor; //monitor id, 0,1,.. etc
    bool    bWallNoTitle; //whether to show title for each window with fps value
    mfxU32  nWallTimeout; //timeout for -wall option
    mfxU32  numViews; // number of views for Multi-View Codec
    mfxU32  nRotation; // rotation for Motion JPEG Codec
    mfxU16  nAsyncDepth; // asyncronous queue
    mfxU16  gpuCopy; // GPU Copy mode (three-state option)
    mfxU16  nThreadsNum;
    mfxI32  SchedulingType;
    mfxI32  Priority;

    mfxU16  width;
    mfxU16  height;
    mfxU32  fourcc;
    mfxU32  nFrames;
} DECODER_PAR_INFO,*PDECODER_PAR_INFO;

class CEncodingPipeline;
class CDecodingPipeline;

class one_process : public QObject
{
    Q_OBJECT
public:
    one_process(int index);
    ~one_process();
    void Init(void);
    static void* video_decoder(void *param);
    void run_video_decoder(void);
    static void* video_encoder(void *param);
    void run_video_encoder(void);
    static void* udp_send(void *param);
    void run_udp_send(void);


private:

    transcodepool *g_pCaptureDeviceVec[1];
    decodepool *g_decodeDevice[1];
    void InitEncoderPar();
    DECODER_PAR_INFO    m_DecoderParInfo;
    ENCODER_PAR_INFO    m_EncoderParInfo;

    mfxStatus InitVideoEncoder( sParams *pParams );
    void InitVideoEncoderParam( sParams *pParams);
//    void GetResolution( unsigned short& nWidth, unsigned short& nHeight );
    void GetQuality( unsigned short& nDstWidth, unsigned short& nDstHeight );
    void PauseVideoEncoderThread();
    void DestroyVideoEncoder();

    mfxStatus InitVideoDecoder( sInputParams *pParams );
    void InitVideoDecoderParam( sInputParams *pParams);
//    void GetResolution( unsigned short& nWidth, unsigned short& nHeight );
    void PauseVideoDecoderThread();
    void DestroyVideoDecoder();

    CDecodingPipeline*   m_pVideoDecoder;
    bool                           m_bExitDeApplication;
    bool                           m_bStopDecoder;
    decodepool*   m_DecodePool[7];
    pthread_mutex_t        m_DeMutex;
    pthread_cond_t          m_DeCond;

    CEncodingPipeline*   m_pVideoEncoder;
    bool                           m_bExitApplication;
    bool                           m_bStopEncoder;
    int                             m_index;
    transcodepool*   m_TranscodePool[7];
    pthread_mutex_t        m_Mutex;
    pthread_cond_t          m_Cond;
};

#endif // ONE_PROCESS_H
