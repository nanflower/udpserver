#ifndef AUDIO_ENCODE_H
#define AUDIO_ENCODE_H


#include <faac.h>

typedef struct tagSample
{
    unsigned long   lSampleLength;
    unsigned long   lTimeStamp;
    long   lDecodeTimeStamp;
    unsigned char  abySample[1];
}SAMPLE, *PSAMPLE;
//#include "loop_list_buffer.h"

class audio_encode
{
public:
    audio_encode();
    ~audio_encode();

    /*!
     * \brief  Init audio encoder
     */
    void InitAudioEncoder();

    /*!
     * \brief  Start faac encoder
     *
     * \param unsigned char*[in] one frame audio data to encoder
     * \param int[in] one frame audio data's  length to encoder
     * \param unsigned long[in] one frame audio data's timestamp to encoder
     */
    void StartFaacEnc( unsigned char* pBuffer, unsigned long nBytesRead, unsigned long lTimeStamp );

    /*!
     * \brief  Close audio encoder
     */
    void CloseFaacEnc();

    /*!
     * \brief  Get audio data after coding
     *
     * \return PSAMPLE: audio data after coding
     */
//    bool GetSampleBuffer( PSAMPLE pSample );

    /*!
     * \brief  clear audio buffer
     */
    void ClearAudioBuffer();

     int m_deviceid;
private:
    void OpenFaacEnc();
    void SetFaacEncConfigure();


private:
    unsigned long       m_lSampleRate;
    unsigned int          m_nChannels;
    unsigned int          m_nPCMBitSize;
    unsigned long       m_lInputSamples;
    unsigned long       m_lMaxOutputBytes;
    unsigned char*     m_pbAACBuffer;
    faacEncHandle      m_pEncoderHander;
    PSAMPLE             m_pSample;
//    CLoopListBuffer*            m_pLoopListBuffer;
};

#endif // AUDIO_ENCODE_H
