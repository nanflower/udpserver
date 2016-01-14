#include "outudppool.h"

outudppool::outudppool( unsigned long lBufSize )
{
    m_lBufferSize = lBufSize;
    m_MAXCNT = m_lBufferSize/188;
    m_pabyBuffer = new BYTE[m_lBufferSize];
    if( NULL != m_pabyBuffer )
        memset( m_pabyBuffer, 0, m_lBufferSize );

    m_lHead = 0;
    m_lRear = 0;
    m_nSampleCount = 0;
    continuity_cnt = 0;
    fpVideo = NULL;
    m_tmpPcr = 0;
    pthread_mutex_init( &m_mutex, NULL );
}

outudppool::~outudppool(void)
{
    if( m_pabyBuffer )
    {
        delete m_pabyBuffer;
        m_pabyBuffer = NULL;
    }
    pthread_mutex_destroy( &m_mutex );
}

bool outudppool::Get( PSAMPLE pGetSample, bool bGetSampleFromBuffer )
{
    if( 0 == m_nSampleCount )	//	buffer is empty
        return false;

    if( NULL == m_pabyBuffer || NULL == pGetSample )
        return false;

    pthread_mutex_lock( &m_mutex );
    bool bRet = true;
    PBYTE pBuf = m_pabyBuffer;
    PSAMPLE pSample = (PSAMPLE)(pBuf+m_lHead);
    unsigned long lRear = m_lRear;
    unsigned long lSize = pSample->lSampleLength+sizeof( pSample->lTimeStamp )+sizeof( pSample->lSampleLength )
            +sizeof( pSample->lDecodeTimeStamp );
    if( lSize >= m_lBufferSize)
        return false;

    memcpy( pGetSample, pSample, lSize );

    if( lRear > m_lHead )
    {
        if( ( lRear - m_lHead ) >= lSize )
        {
            if( bGetSampleFromBuffer )
            {
                m_lHead += lSize;
                --m_nSampleCount;
            }
        }
        else
        {
            bRet = false;
        }
    }
    else
    {
        if( NLOOPBUF_SIZE - m_lHead >= lSize && pSample->lSampleLength != 0 )
        {
            if( bGetSampleFromBuffer )
            {
                m_lHead += lSize;
                --m_nSampleCount;
            }
        }
        else
        {
            pSample = (PSAMPLE)pBuf;
            lSize = pSample->lSampleLength+sizeof( pSample->lTimeStamp )+sizeof( pSample->lSampleLength )
                    +sizeof( pSample->lDecodeTimeStamp );
            if( lSize >= m_lBufferSize)
                return false;
            memcpy( pGetSample, pSample, lSize );
            if( lRear >= lSize )
            {
                if( bGetSampleFromBuffer )
                {
                    m_lHead = lSize;
                    --m_nSampleCount;
                }
            }
            else
                bRet = false;
        }
    }

    pthread_mutex_unlock( &m_mutex );
    return bRet;
}

int outudppool::GetSampleCount()
{
    return m_nSampleCount;
}

void outudppool::ClearBuffer()
{
    if( NULL != m_pabyBuffer )
        memset( m_pabyBuffer, 0, m_lBufferSize );

    m_lHead = 0;
    m_lRear = 0;
    m_nSampleCount = 0;
}


