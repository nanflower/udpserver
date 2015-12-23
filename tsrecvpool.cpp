#include "tsrecvpool.h"

volatile int m_nSampleCount;
unsigned char* m_pabyBuffer;
volatile unsigned long m_lHead;
volatile unsigned long m_lRear;
unsigned long m_MAXCNT;
unsigned long m_lBufferSize;
pthread_mutex_t m_mutex;

tsrecvpool::tsrecvpool()
{
    m_lBufferSize = 1000*987*188;
    m_MAXCNT = m_lBufferSize/188;
    m_pabyBuffer = new unsigned char[m_lBufferSize];
    if( NULL != m_pabyBuffer )
        memset( m_pabyBuffer, 0, m_lBufferSize );
    m_lHead = 0;
    m_lRear = 0;
    m_nSampleCount = 0;
    pthread_mutex_init( &m_mutex, NULL );
}

tsrecvpool::~tsrecvpool()
{
    if( m_pabyBuffer )
    {
        delete m_pabyBuffer;
        m_pabyBuffer = NULL;
    }
    pthread_mutex_destroy( &m_mutex );
}

bool tsrecvpool::WriteTsPacket(unsigned char *pData, int len)
{
    if( m_nSampleCount == m_MAXCNT )
        return false;
    pthread_mutex_lock( &m_mutex );
    int temp = len / 188;
//    if(len%188 != 0)
//        printf("temp = %d\n",len%188);
    if(m_lRear != m_MAXCNT)
    {
        memcpy( &m_pabyBuffer[m_lRear*188], pData, len );
        m_lRear += temp;
        m_nSampleCount += temp;
    }else
    {
        memcpy( &m_pabyBuffer[0], pData, len );
        m_lRear = 1;
        m_nSampleCount += temp;
    }
    //printf("m_lRear = %d, count = %d\n", m_lRear, m_nSampleCount);
    pthread_mutex_unlock( &m_mutex );
    return true;
}

bool tsrecvpool::GetTsPacket(unsigned char *pData, int size)
{

  //  printf("m_lhead = %d,m_lRear = %d, m_nSampleCount = %d, size = %d\n", m_lHead, m_lRear,m_nSampleCount,size );
    int temp = size/188;
    if( m_nSampleCount < temp )
        return false;

    pthread_mutex_lock( &m_mutex );
//    printf("size = %d, temp = %d\n",size, temp);
 //   if(m_lHead != m_MAXCNT)
    if(m_lHead != m_MAXCNT)
    {
        memcpy( pData, &m_pabyBuffer[m_lHead*188], size );
        m_lHead += temp;
        m_nSampleCount -= temp;
    }else
    {
        memcpy( pData, &m_pabyBuffer[0], size );
        m_lHead = 1;
        m_nSampleCount -= temp;
    }

    pthread_mutex_unlock( &m_mutex );
    return true;
}

int tsrecvpool::GetSampleCount()
{
    return m_nSampleCount;
}



//void tsrecvpool::init_queue( int size) {
//    pthread_mutex_init(&locker, NULL);
//    pthread_cond_init(&cond, NULL);
//    q_buf = (uint8_t*)av_mallocz(sizeof(uint8_t)*size);
//    read_ptr = write_ptr = 0;
//    bufsize = size;
//    printf("buffer size = %d\n",bufsize);
////	fprintf(stdout, "bufsize=%d\n", size);
//}

//void tsrecvpool::free_queue(void) {
//    pthread_mutex_destroy(&locker);
//    pthread_cond_destroy(&cond);
//    av_free(q_buf);
//}

//void tsrecvpool::put_queue(unsigned char* buf, int size) {
//    uint8_t* dst = q_buf + write_ptr;

//    pthread_mutex_lock(&locker);
//    //printf("dst = %d\n",size);
//    if ((write_ptr + size) > bufsize) {
//        memcpy(dst, buf, (bufsize - write_ptr));
//        memcpy(q_buf, buf+(bufsize - write_ptr), size-(bufsize - write_ptr));
//    } else {
//        memcpy(dst, buf, size*sizeof(uint8_t));
//    }
//    write_ptr = (write_ptr + size) % bufsize;
//    pthread_cond_signal(&cond);
//    pthread_mutex_unlock(&locker);
//}

//int tsrecvpool::get_queue(uint8_t* buf, int size) {
//    uint8_t* src = q_buf + read_ptr;
//    int wrap = 0;

//    pthread_mutex_lock(&locker);

//    int pos = write_ptr;

//    if (pos < read_ptr) {
//        pos += bufsize;
//        wrap = 1;
//    }

//    if ( (read_ptr + size) > pos) {
//        pthread_mutex_unlock(&locker);
//        return 1;
////		struct timespec timeout;
////		timeout.tv_sec=time(0)+1;
////		timeout.tv_nsec=0;
////		pthread_cond_timedwait(&cond, &locker, &timeout);
////		if ( (read_ptr + size) > pos ) {
////			pthread_mutex_unlock(&locker);
////			return 1;
////		}
//    }

//    if (wrap) {
//        fprintf(stdout, "wrap...\n");
//        memcpy(buf, src, (bufsize - read_ptr));
//        memcpy(buf+(bufsize - read_ptr), src+(bufsize - read_ptr), size-(bufsize - read_ptr));
//    } else {
//        memcpy(buf, src, sizeof(uint8_t)*size);
//    }
//    read_ptr = (read_ptr + size) % bufsize;
//    pthread_mutex_unlock(&locker);

//    return 0;
//}

