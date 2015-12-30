#ifndef TSPOOLQUEUE_H
#define TSPOOLQUEUE_H

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

typedef unsigned char uint8_t;

class tspoolqueue
{
public:
    tspoolqueue();
    ~tspoolqueue();
    void init(void);
    void init_buffer(void);
    void init_queue( int size);
    void free_queue();
    void put_queue(unsigned char* buf, int size);
    int get_queue(uint8_t* buf, int size);

    void put_queue1(unsigned char* buf, int size);
    int get_queue1(uint8_t* buf, int size);

    //decode
    void write_buffer(uint8_t* buf, int size);
    int get_buffer(uint8_t* buf,int size);

//    pthread_mutex_t locker;
//    pthread_cond_t cond;
//    uint8_t* q_buf;
//    int bufsize;
//    int write_ptr;
//    int read_ptr;

};

#endif // TSRECVPOOL_H
