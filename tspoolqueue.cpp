#include "tspoolqueue.h"

pthread_mutex_t locker;
//pthread_cond_t cond;
uint8_t* q_buf;
uint8_t* dst;
uint8_t* src;
int bufsize;
volatile int write_ptr;
volatile int read_ptr;

pthread_mutex_t locker1;
//pthread_cond_t cond;
uint8_t* q_buf1;
uint8_t* dst1;
uint8_t* src1;
int bufsize1;
volatile int write_ptr1;
volatile int read_ptr1;

pthread_mutex_t decodelocker1;
FILE *fpv1;

tspoolqueue::tspoolqueue()
{
    pthread_mutex_init(&locker, NULL);
//    pthread_cond_init(&cond, NULL);
    q_buf = (uint8_t*)av_mallocz(sizeof(uint8_t)*1024*1024*2);
    write_ptr = 0;
    read_ptr = 0;
    bufsize = 1024*1024*2;
    printf("buffer size = %d\n",bufsize);
    dst = q_buf;
    src = q_buf;


    pthread_mutex_init(&locker1, NULL);
//    pthread_cond_init(&cond, NULL);
    q_buf1 = (uint8_t*)av_mallocz(sizeof(uint8_t)*1024*1024*2);
    read_ptr1 = write_ptr1 = 0;
    bufsize1 = 1024*1024*2;
    printf("buffer size = %d\n",bufsize1);

    fpv1 = fopen("channel 1 video 1.h264","ab");
    pthread_mutex_init(&decodelocker1, NULL);
}


tspoolqueue::~tspoolqueue()
{
    pthread_mutex_destroy(&locker);
//    pthread_cond_destroy(&cond);
    av_free(q_buf);
    pthread_mutex_destroy(&locker1);
    av_free(q_buf1);
    pthread_mutex_destroy(&decodelocker1);
}


void tspoolqueue::free_queue(void) {
    pthread_mutex_destroy(&locker);
//    pthread_cond_destroy(&cond);
    av_free(q_buf);
    pthread_mutex_destroy(&locker1);
    av_free(q_buf1);
    pthread_mutex_destroy(&decodelocker1);
}


void tspoolqueue::put_queue(unsigned char* buf, int size) {
//    printf(" put queue :   tid %lu\n",(unsigned long)pthread_self());
    pthread_mutex_lock(&locker);
    dst = q_buf + write_ptr;
    if ((write_ptr + size) > bufsize) {
        memcpy(dst, buf, (bufsize - write_ptr));
        memcpy(q_buf, buf+(bufsize - write_ptr), size-(bufsize - write_ptr));
    } else {
        memcpy(dst, buf, size*sizeof(uint8_t));
    }
    write_ptr = (write_ptr + size) % bufsize;
//    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&locker);
}

int tspoolqueue::get_queue(uint8_t* buf, int size) {
 //   printf(" get queue : tid %lu write_ptr : %d read_ptr : %d\n",(unsigned long)pthread_self(),write_ptr,read_ptr);
  //  printf("size = %d\n",size);

    pthread_mutex_lock(&locker);
    src = q_buf + read_ptr;
    int wrap = 0;

    int pos = write_ptr;

    if (pos < read_ptr) {
        pos += bufsize;
        if(size + read_ptr > bufsize)
            wrap = 1;
    }

    if ( (read_ptr + size) > pos) {
        pthread_mutex_unlock(&locker);
        return 1;
//		struct timespec timeout;
//		timeout.tv_sec=time(0)+1;
//		timeout.tv_nsec=0;
//		pthread_cond_timedwait(&cond, &locker, &timeout);
//		if ( (read_ptr + size) > pos ) {
//			pthread_mutex_unlock(&locker);
//			return 1;
//		}
    }

    if (wrap) {
        fprintf(stdout, "wrap...\n");
        memcpy(buf, src, (bufsize - read_ptr));
//        memcpy(buf+(bufsize - read_ptr), src+(bufsize - read_ptr), size-(bufsize - read_ptr));
        src = q_buf + 0;
        memcpy(buf+(bufsize - read_ptr), src, size-(bufsize - read_ptr));
        printf("get 1\n");
    } else {
        memcpy(buf, src, sizeof(uint8_t)*size);
    }
    read_ptr = (read_ptr + size) % bufsize;
    pthread_mutex_unlock(&locker);

    return 0;
}

void tspoolqueue::put_queue1(unsigned char* buf, int size) {
//    printf(" put queue :   tid %lu\n",(unsigned long)pthread_self());
    dst1 = q_buf1 + write_ptr1;
    pthread_mutex_lock(&locker1);
    if ((write_ptr1 + size) > bufsize1) {
        memcpy(dst1, buf, (bufsize1 - write_ptr1));
        memcpy(q_buf1, buf+(bufsize1 - write_ptr1), size-(bufsize1 - write_ptr1));
    } else {
        memcpy(dst1, buf, size*sizeof(uint8_t));
    }
    write_ptr1 = (write_ptr1 + size) % bufsize1;
//    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&locker1);
}

int tspoolqueue::get_queue1(uint8_t* buf, int size) {
  //  printf(" get queue1 : tid %lu write_ptr1 : %d read_ptr1 : %d\n",(unsigned long)pthread_self(),write_ptr1,read_ptr1);
    src1 = q_buf1 + read_ptr1;
    int wrap = 0;
  //  printf("size = %d\n",size);

    pthread_mutex_lock(&locker1);

    int pos = write_ptr1;

    if (pos < read_ptr1) {
        pos += bufsize1;
        wrap = 1;
    }

    if ( (read_ptr1 + size) > pos) {
        pthread_mutex_unlock(&locker1);
        return 1;
//		struct timespec timeout;
//		timeout.tv_sec=time(0)+1;
//		timeout.tv_nsec=0;
//		pthread_cond_timedwait(&cond, &locker, &timeout);
//		if ( (read_ptr + size) > pos ) {
//			pthread_mutex_unlock(&locker);
//			return 1;
//		}
    }

    if (wrap) {
        fprintf(stdout, "wrap...\n");
        memcpy(buf, src1, (bufsize1 - read_ptr1));
        memcpy(buf+(bufsize1 - read_ptr1), src1+(bufsize1 - read_ptr1), size-(bufsize1 - read_ptr1));
    } else {
        memcpy(buf, src1, sizeof(uint8_t)*size);
    }
    read_ptr1 = (read_ptr1 + size) % bufsize1;
    pthread_mutex_unlock(&locker1);

    return 0;
}

void tspoolqueue::write_buffer(uint8_t* buf, int size) {
//    pthread_mutex_lock(&decodelocker1);
    fwrite(buf, size, 1, fpv1);
//    pthread_mutex_lock(&decodelocker1);
}
