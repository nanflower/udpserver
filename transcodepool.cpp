#include "transcodepool.h"

pthread_mutex_t lockerx;
pthread_mutex_t ylocker[PIN_NUM];
pthread_mutex_t uvlocker[PIN_NUM];
pthread_cond_t ycond[PIN_NUM];
pthread_cond_t uvcond[PIN_NUM];
uint8_t* yQueue_buf[PIN_NUM];
uint8_t* uvQueue_buf[PIN_NUM];
int ybufsize[PIN_NUM];
int uvbufsize[PIN_NUM];
volatile int ywrite_ptr[PIN_NUM];
volatile int yread_ptr[PIN_NUM];
volatile int uvwrite_ptr[PIN_NUM];
volatile int uvread_ptr[PIN_NUM];
unsigned long TimeStamp;


transcodepool::transcodepool()
{

}

transcodepool::~transcodepool()
{
    for(int i=0; i<PIN_NUM; i++){
        pthread_mutex_destroy(&ylocker[i]);
        av_free(yQueue_buf[i]);
        pthread_mutex_destroy(&uvlocker[i]);
        av_free(uvQueue_buf[i]);
    }
    pthread_mutex_destroy(&lockerx);
}

void transcodepool::Init()
{
    for(int i=0; i<PIN_NUM; i++){
        pthread_mutex_init(&ylocker[i], NULL);
        yQueue_buf[i] = (uint8_t*)av_mallocz(sizeof(uint8_t)*720*576*100);
        yread_ptr[i] = 0;
        ywrite_ptr[i] = 0;
        ybufsize[i] = 720*576*100;
    }

    pthread_mutex_init(&lockerx, NULL);

    for(int i=0; i<PIN_NUM; i++){
        pthread_mutex_init(&uvlocker[i], NULL);
        uvQueue_buf[i] = (uint8_t*)av_mallocz(sizeof(uint8_t)*720*576*50);
        uvread_ptr[i] = 0;
        uvwrite_ptr[i] = 0;
        uvbufsize[i] = 720*576*50;
    }
    TimeStamp = 0;
}

bool transcodepool::GetFrame( void **YFrameBuf, void **UVFrameBuf, int *DataLength, int *UDataLength, unsigned long * plTimeStamp, int i )
{
    printf("encoder get frame\n");

    pthread_mutex_lock(&lockerx);

    if(ywrite_ptr[i] == yread_ptr[i]){
        return false;
    }
    else if(ywrite_ptr[i] < yread_ptr[i]){
        *DataLength = ywrite_ptr[i] + ybufsize[i] - yread_ptr[i];
        memcpy(YFrameBuf, yQueue_buf[i] + yread_ptr[i], ybufsize[i] - yread_ptr[i]);
        memcpy(YFrameBuf + ybufsize[i] - yread_ptr[i], yQueue_buf[i], *DataLength - ybufsize[i] + yread_ptr[i]);
    }
    else {
        *DataLength = ywrite_ptr[i] - yread_ptr[i];
        memcpy(YFrameBuf, yQueue_buf[i] + yread_ptr[i], *DataLength);
    }
    yread_ptr[i] = ywrite_ptr[i];

    if(uvwrite_ptr[i] == uvread_ptr[i]){
        return false;
    }
    else if(uvwrite_ptr[i] < uvread_ptr[i]){
        *UDataLength = uvwrite_ptr[i] + uvbufsize[i] - uvread_ptr[i];
        memcpy(UVFrameBuf, uvQueue_buf[i] + uvread_ptr[i], uvbufsize[i] - uvread_ptr[i]);
        memcpy(UVFrameBuf + uvbufsize[i] - uvread_ptr[i], uvQueue_buf[i], *UDataLength - uvbufsize[i] + uvread_ptr[i]);
    }
    else {
        *UDataLength = uvwrite_ptr[i] - uvread_ptr[i];
        memcpy(UVFrameBuf, uvQueue_buf[i] + uvread_ptr[i], *UDataLength);
    }
    uvread_ptr[i] = uvwrite_ptr[i];

    *plTimeStamp = TimeStamp;

    pthread_mutex_unlock(&lockerx);
    return true;
}

bool transcodepool::PutFrame( AVFrame *pVideoframe, int i )
{

    TimeStamp = pVideoframe->pts;

    pthread_mutex_lock(&lockerx);
    printf("thread 1write = %d, read = %d, size = %d, bufsize = %d\n",ywrite_ptr[i], yread_ptr[i], pVideoframe->linesize[0],ybufsize[i]);
    if(ywrite_ptr[i] + pVideoframe->linesize[0] < ybufsize[i]){
        memcpy(yQueue_buf[i] + ywrite_ptr[i], pVideoframe->data[0], pVideoframe->linesize[0]);
        ywrite_ptr[i] += pVideoframe->linesize[0];
    }
    else{
        int ylastbufsize = ybufsize[i] - ywrite_ptr[i];
        memcpy(yQueue_buf[i] + ywrite_ptr[i], pVideoframe->data[0], ylastbufsize);
        memcpy(yQueue_buf[i], pVideoframe->data[0] + ylastbufsize, pVideoframe->linesize[0] - ylastbufsize);
        ywrite_ptr[i] = pVideoframe->linesize[0] - ylastbufsize;
    }
    if(uvwrite_ptr[i] + pVideoframe->linesize[1] < uvbufsize[i]){
        memcpy(uvQueue_buf[i] + uvwrite_ptr[i], pVideoframe->data[1], pVideoframe->linesize[1]);
        uvwrite_ptr[i] += pVideoframe->linesize[1];
    }
    else{
        int uvlastbufsize = uvbufsize[i] - uvwrite_ptr[i];
        memcpy(uvQueue_buf[i] + uvwrite_ptr[i], pVideoframe->data[1], uvlastbufsize);
        memcpy(uvQueue_buf[i], pVideoframe->data[1] + uvlastbufsize, pVideoframe->linesize[1] - uvlastbufsize);
        uvwrite_ptr[i] = pVideoframe->linesize[1] - uvlastbufsize;
    }
    pthread_mutex_unlock(&lockerx);
    return true;
}
