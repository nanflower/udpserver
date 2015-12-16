#ifndef UDPSOCKET_H
#define UDPSOCKET_H

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

#define BUFFER_SIZE 4096*2000
#define FILE_NAME_MAX_SIZE 512
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000

typedef struct _new_queue {
    pthread_mutex_t locker;
    pthread_cond_t cond;
    uint8_t* buf;
    int bufsize;
    int write_ptr;
    int read_ptr;
} NewQueue;

class udpsocket
{
public:
    udpsocket();
    ~udpsocket();
    void thread_init(int index);
    static void* udp_ts_recv1(void *param);
    static void* udp_ts_recv2(void *param);
    static void* udp_ts_recv3(void *param);
    static void init_queue(NewQueue *que, int size);
    static void free_queue(NewQueue* que);
    static void put_queue(NewQueue*que, char* buf, int size);
    static int get_queue(NewQueue*que, uint8_t* buf, int size);
    static int read_data(void *opaque, uint8_t *buf, int buf_size);
    int ts_demux();
};

#endif // UDPSOCKET_H
