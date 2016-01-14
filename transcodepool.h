#ifndef TRANSCODEPOOL_H
#define TRANSCODEPOOL_H

#ifdef _WIN32
//Windows
extern "C"
{
#include "libavformat/avformat.h"
};
#else
//Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#ifdef __cplusplus
};
#endif
#endif

#include "pthread.h"
#define PIN_NUM 7

typedef unsigned char BYTE;

//typedef struct tagSample
//{
//    unsigned long   lSampleLength;
//    unsigned long   lTimeStamp;
//    long   lDecodeTimeStamp;
//    BYTE  abySample[1];
//}SAMPLE, *PSAMPLE;

class transcodepool
{
public:
    transcodepool();
    ~transcodepool();
    void Init();
    bool GetFrame( uint8_t *YFrameBuf,  int DataLength, unsigned long * plTimeStamp, int i);
    bool PutFrame( AVFrame *pVideoframe , int index);
private:
//    pthread_mutex_t lockerx;
////    pthread_cond_t ycond[PIN_NUM];
//    uint8_t* yQueue_buf[PIN_NUM];
//    int ybufsize[PIN_NUM];
//    volatile int ywrite_ptr[PIN_NUM];
//    volatile int yread_ptr[PIN_NUM];
//    unsigned long long TimeStamp;
};

#endif // TRANSCODEPOOL_H
