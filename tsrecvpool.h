#ifndef TSRECVPOOL_H
#define TSRECVPOOL_H

#define __STDC_CONSTANT_MACROS

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

#include <pthread.h>

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

class tsrecvpool
{
public:
    tsrecvpool();
    ~tsrecvpool();
    bool WriteTsPacket(unsigned char* pData, int len);
    bool GetTsPacket(unsigned char* pData, int size);
    int GetSampleCount();

    volatile int m_nSampleCount;
    unsigned char* m_pabyBuffer;
    volatile unsigned long m_lHead;
    volatile unsigned long m_lRear;
    unsigned long m_MAXCNT;
    unsigned long m_lBufferSize;
    pthread_mutex_t m_mutex;

};

#endif // TSRECVPOOL_H
