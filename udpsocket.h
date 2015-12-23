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

#include "tsrecvpool.h"
#include "tspoolqueue.h"

#define VIDEO_NUM 7
#define AUDIO_NUM 7
#define VIDEOBUF_SIZE 10000*188
#define BUF_SIZE 4096*2000 //read buffer
#define BUFFER_SIZE 4096      //recvfrom buffer
#define FILE_NAME_MAX_SIZE 512
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000

typedef struct _new_queue {
    pthread_mutex_t locker;
    pthread_cond_t cond;
    uint8_t* q_buf;
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
    static void* udp_tsrecv(void *param);
    static void* udp_tsrecv1(void *param);
    void udp_ts_recv(void);
    void udp_ts_recv1(void);
    static int read_data(void *opaque, uint8_t *buf, int buf_size);
    static int read_data1(void *opaque, uint8_t *buf, int buf_size);
    int ts_demux(int index);
    int thread_test();
    static void* ts_demuxer( void *param );
    static void* ts_demuxer1(void *param);
    tspoolqueue* m_tsRecvPool;
private:
//    AVCodec *pVideoCodec[VIDEO_NUM];
//    AVCodec *pAudioCodec[AUDIO_NUM];
//    AVCodecContext *pVideoCodecCtx[VIDEO_NUM];
//    AVCodecContext *pAudioCodecCtx[AUDIO_NUM];
//    AVIOContext * pb;
//    AVInputFormat *piFmt;
//    AVFormatContext *pFmt;
//    uint8_t *buffer;
//    int videoindex[VIDEO_NUM];
//    int audioindex[AUDIO_NUM];
//    AVStream *pVst[VIDEO_NUM];
//    AVStream *pAst[AUDIO_NUM];
//    AVFrame *pframe;
//    AVPacket pkt;
//    int got_picture;
//    int video_num[VIDEO_NUM];
//    int audio_num[AUDIO_NUM];
//    int frame_size;


};

#endif // UDPSOCKET_H
