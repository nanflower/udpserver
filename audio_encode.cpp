#include "audio_encode.h"
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>

audio_encode::audio_encode()
{
    m_lSampleRate = 48000;
    m_nChannels = 2;
    m_nPCMBitSize = 16;
    m_lInputSamples = 0;
    m_lMaxOutputBytes = 0;
    m_pEncoderHander = NULL;
    m_pbAACBuffer = NULL;

    m_pSample = (PSAMPLE)new unsigned char[12+4096*10];
    if( NULL == m_pSample )
    {
        printf("%s:%d   Error:new m_pSample failed!!!\n", __FILE__, __LINE__ );
        return;
    }

}

audio_encode::~audio_encode()
{
    if( m_pSample )
    {
        delete m_pSample;
        m_pSample = NULL;
    }

    if( m_pbAACBuffer )
    {
        delete m_pbAACBuffer;
        m_pbAACBuffer = NULL;
    }

//    if( m_pLoopListBuffer )
//    {
//        delete m_pLoopListBuffer;
//        m_pLoopListBuffer = NULL;
//    }
}

void audio_encode::InitAudioEncoder()
{
//    m_pLoopListBuffer  = g_pLoopListBuffer[m_deviceid];
    OpenFaacEnc();

    if( NULL == m_pbAACBuffer )
    {
        m_pbAACBuffer = new unsigned char [m_lMaxOutputBytes];
        if( NULL == m_pbAACBuffer )
        {
            printf("%s:%d   Error:new m_pbAACBuffer failed!!!\n", __FILE__, __LINE__ );
            return;
        }
    }

    SetFaacEncConfigure();
}

void audio_encode::StartFaacEnc( unsigned char* pBuffer, unsigned long lBytesRead, unsigned long  lTimeStamp  )
{
    if( NULL == pBuffer || lBytesRead <= 0 || NULL == m_pEncoderHander || NULL == m_pbAACBuffer )
        return;

    // 输入样本数，用实际读入字节数计算
    m_lInputSamples = lBytesRead / ( m_nPCMBitSize / 8 );
    unsigned long lEncodeLength = faacEncEncode( m_pEncoderHander, (int*) pBuffer, m_lInputSamples, m_pbAACBuffer, m_lMaxOutputBytes );
    if( 0 == lEncodeLength || NULL == m_pSample )
        return;
    memcpy( &(m_pSample->abySample[0]), m_pbAACBuffer, lEncodeLength );
    m_pSample->lSampleLength = lEncodeLength;
    m_pSample->lTimeStamp = lTimeStamp;

//    if( NULL == m_pLoopListBuffer )
//        return;

//    m_pLoopListBuffer->Write( m_pSample , bAUDIO);
//   if(m_pLoopListBuffer->fpVideo)
//        fwrite(m_pbAACBuffer,lEncodeLength,1,m_pLoopListBuffer->fpVideo);
}

void audio_encode::CloseFaacEnc()
{
    if( m_pEncoderHander )
    {
        faacEncClose( m_pEncoderHander );
        m_pEncoderHander = NULL;
    }

    if( m_pbAACBuffer )
    {
        delete m_pbAACBuffer;
        m_pbAACBuffer = NULL;
    }
}

//bool audio_encode::GetSampleBuffer( PSAMPLE pSample )
//{
//    if( NULL == m_pLoopListBuffer )
//        return false;
//    return m_pLoopListBuffer->Get( pSample );
//}

void audio_encode::ClearAudioBuffer()
{
//    if( NULL == m_pLoopListBuffer )
//        return;
//    m_pLoopListBuffer->ClearBuffer();
}


void audio_encode::OpenFaacEnc()
{
    if( m_pEncoderHander )
        return;

    m_pEncoderHander = faacEncOpen( m_lSampleRate, m_nChannels, &m_lInputSamples, &m_lMaxOutputBytes );
    if( NULL == m_pEncoderHander )
    {
        printf("%s:%d   Error:Failed to call faacEncOpen()\n", __FILE__, __LINE__ );
        return;
    }
}

void audio_encode::SetFaacEncConfigure()
{
    if( NULL == m_pEncoderHander )
        return;

    faacEncConfigurationPtr pConfiguration = NULL;
    pConfiguration = faacEncGetCurrentConfiguration( m_pEncoderHander );
    if( NULL == pConfiguration )
    {
        printf("%s:%d   Error:Failed to call faacEncGetCurrentConfiguration()\n", __FILE__, __LINE__ );
        return;
    }

    pConfiguration->inputFormat = FAAC_INPUT_16BIT;
    pConfiguration->outputFormat = 1; //ADTS
    pConfiguration->bitRate = 32000;//64000;//128000;
    pConfiguration->aacObjectType = LOW;  // LC
    pConfiguration->mpegVersion = MPEG4;  // MPEG-4

    // Set encoding configuration
    faacEncSetConfiguration( m_pEncoderHander, pConfiguration );
}
