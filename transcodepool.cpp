#include "transcodepool.h"

//int Timewrite
pthread_mutex_t lockerx;
//    pthread_cond_t ycond[PIN_NUM];
uint8_t* yQueue_buf;
int ybufsize;
volatile int ywrite_ptr;
volatile int yread_ptr;
unsigned long long TimeStamp;

FILE *fp_temp;

transcodepool::transcodepool()
{

}

transcodepool::~transcodepool()
{
    for(int i=0; i<PIN_NUM; i++){
        av_free(yQueue_buf);
    }
    pthread_mutex_destroy(&lockerx);
}

void transcodepool::Init()
{
    for(int i=0; i<PIN_NUM; i++){
        yQueue_buf = (uint8_t*)av_mallocz(sizeof(uint8_t)*720*576*30);
        yread_ptr = 0;
        ywrite_ptr = 0;
        ybufsize = 720*576*30;
    }

    fp_temp = fopen("temp.yuv","ab+");
    pthread_mutex_init(&lockerx, NULL);

    TimeStamp = 0;
//    for(int i=0; i<20; i++)
}

bool transcodepool::GetFrame( uint8_t *YFrameBuf, int DataLength, unsigned long * plTimeStamp, int i )
{

    int width = 720;
    int height = 576;
    DataLength = width * height * 3/2;

    while(1){
//        printf(" write = %d, read = %d \n", ywrite_ptr, yread_ptr);
        if( (ywrite_ptr == yread_ptr + DataLength) || (ywrite_ptr == DataLength&&yread_ptr == ybufsize) ){
            printf("write = %d , read =%d , char size = %d\n", ywrite_ptr, yread_ptr, sizeof(uint8_t));
            break;
        }
    }
    printf("out write = %d, out read = %d, pts =%lld \n",ywrite_ptr, yread_ptr, TimeStamp);

    pthread_mutex_lock(&lockerx);

    if(ywrite_ptr == yread_ptr || (yread_ptr == ybufsize && ywrite_ptr == 0) )
        return false;
    else if(yread_ptr == ybufsize){
        memcpy(YFrameBuf, yQueue_buf, DataLength);
        yread_ptr = DataLength;
    }
    else {
        printf("before memcpy read = %d, write = %d, data = %d\n", yread_ptr, ywrite_ptr, DataLength);
        memcpy(YFrameBuf, yQueue_buf + yread_ptr, DataLength);
//        memcpy(YFrameBuf, yQueue_buf, DataLength);
        yread_ptr += DataLength;
    }
    fwrite(yQueue_buf + yread_ptr - DataLength, DataLength, 1 ,fp_temp);
    *plTimeStamp = TimeStamp;

    pthread_mutex_unlock(&lockerx);
    return true;
}

bool transcodepool::PutFrame( AVFrame *pVideoframe, int i )
{

    TimeStamp = pVideoframe->pts;

    pthread_mutex_lock(&lockerx);
    printf("thread 1 pts = %lld, write = %d, read = %d, bufsize = %d\n", TimeStamp, ywrite_ptr, yread_ptr, ybufsize);

    int j=0;
    if(ywrite_ptr + pVideoframe->width*pVideoframe->height*3/2 <= ybufsize){
        for( j=0; j<pVideoframe->height; j++){
            memcpy(yQueue_buf + ywrite_ptr + pVideoframe->width*j, pVideoframe->data[0] + pVideoframe->linesize[0]*j, pVideoframe->width);
        }
        ywrite_ptr += pVideoframe->width*j;
        printf("Y write =%d\n",ywrite_ptr);
        for( j=0; j<pVideoframe->height/2; j++){
            memcpy(yQueue_buf + ywrite_ptr + pVideoframe->width/2*j, pVideoframe->data[1] + pVideoframe->linesize[1]*j, pVideoframe->width/2);
        }
        ywrite_ptr += pVideoframe->width/2*j;
        for( j=0; j<pVideoframe->height/2; j++){
            memcpy(yQueue_buf + ywrite_ptr + pVideoframe->width/2*j, pVideoframe->data[2] + pVideoframe->linesize[2]*j, pVideoframe->width/2);
        }
        ywrite_ptr += pVideoframe->width/2*j;
        printf("U write =%d\n",ywrite_ptr);
    }
    else{
        ywrite_ptr = 0;
        for( j=0; j<pVideoframe->height; j++){
            memcpy(yQueue_buf + ywrite_ptr + pVideoframe->width*j, pVideoframe->data[0] + pVideoframe->linesize[0]*j, pVideoframe->width);
        }
        ywrite_ptr += pVideoframe->width*j;
        for( j=0; j<pVideoframe->height/2; j++){
            memcpy(yQueue_buf + ywrite_ptr + pVideoframe->width/2*j, pVideoframe->data[1] + pVideoframe->linesize[1]*j, pVideoframe->width/2);
        }
        ywrite_ptr += pVideoframe->width/2*j;
        for( j=0; j<pVideoframe->height/2; j++){
            memcpy(yQueue_buf + ywrite_ptr + pVideoframe->width/2*j, pVideoframe->data[2] + pVideoframe->linesize[2]*j, pVideoframe->width/2);
        }
        ywrite_ptr += pVideoframe->width/2*j;

    }

    pthread_mutex_unlock(&lockerx);
    return true;
}
