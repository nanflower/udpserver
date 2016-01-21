#ifndef DECODEPOOL_H
#define DECODEPOOL_H

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

#define PIN_NUM 7
#include "pthread.h"

class decodepool
{
public:
    decodepool();
    ~decodepool();
    void Init(int index);
    bool getbuffer( uint8_t *pData, int LastLength, int *DataLength, unsigned long *plTimeStamp, int index );
    bool putbuffer( AVPacket *pVideoPacket, int index);

private:
//    pthread_mutex_t lockdecode;
//    uint8_t* decode_buffer;
//    int decode_bufsize;
//    volatile int dewrite_ptr;
//    volatile int deread_ptr;
//    unsigned long DeTimeStamp;
};

#endif // DECODEPOOL_H
