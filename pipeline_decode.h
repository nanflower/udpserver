/*****************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or
nondisclosure agreement with Intel Corporation and may not be copied
or disclosed except in accordance with the terms of that agreement.
Copyright(c) 2005-2015 Intel Corporation. All Rights Reserved.

*****************************************************************************/

#ifndef __PIPELINE_DECODE_H__
#define __PIPELINE_DECODE_H__

#include "sample_defs.h"

#if D3D_SURFACES_SUPPORT
#pragma warning(disable : 4201)
#include <d3d9.h>
#include <dxva2api.h>
#endif

#include <vector>
#include "hw_device.h"

#include "utils.h"
//#include "/*decode_render*/.h"
#include "mfx_buffering.h"
#include <memory>

//#include "sample_utils.h"
#include "sample_params.h"
#include "base_allocator.h"

#include "mfxmvc.h"
#include "mfxjpeg.h"
#include "mfxplugin.h"
#include "mfxplugin++.h"
#include "mfxvideo.h"
#include "mfxvideo++.h"

//#include "math.h"
//#include "mfxcommon.h"
//#include "mfxvp8.h"
//#include "mfxstructures.h"
//#include "mfxvideo.h"
//#include "mfxdefs.h"

#include "decodepool.h"
#include "transcodepool.h"

//#include "plugin_loader.h"

enum MemType {
    SYSTEM_MEMORY = 0x00,
    D3D9_MEMORY   = 0x01,
    D3D11_MEMORY  = 0x02,
};

enum eWorkMode {
  MODE_PERFORMANCE,
  MODE_RENDERING,
  MODE_FILE_DUMP
};

struct sInputParams
{
    int decodeID;

    decodepool* decode;
    transcodepool* transcode;
    mfxU32 videoType;
    eWorkMode mode;
    MemType memType;
    bool    bUseHWLib; // true if application wants to use HW mfx library
    bool    bIsMVC; // true if Multi-View Codec is in use
    bool    bLowLat; // low latency mode
    bool    bCalLat; // latency calculation
    mfxU32  nMaxFPS; //rendering limited by certain fps
    mfxU32  nWallCell;
    mfxU32  nWallW; //number of windows located in each row
    mfxU32  nWallH; //number of windows located in each column
    mfxU32  nWallMonitor; //monitor id, 0,1,.. etc
    bool    bWallNoTitle; //whether to show title for each window with fps value
    mfxU32  nWallTimeout; //timeout for -wall option
    mfxU32  numViews; // number of views for Multi-View Codec
    mfxU32  nRotation; // rotation for Motion JPEG Codec
    mfxU16  nAsyncDepth; // asyncronous queue
    mfxU16  gpuCopy; // GPU Copy mode (three-state option)
    mfxU16  nThreadsNum;
    mfxI32  SchedulingType;
    mfxI32  Priority;

    mfxU16  width;
    mfxU16  height;
    mfxU32  fourcc;
    mfxU32  nFrames;

    msdk_char     strSrcFile[MSDK_MAX_FILENAME_LEN];
    msdk_char     strDstFile[MSDK_MAX_FILENAME_LEN];
    sPluginParams pluginParams;

    sInputParams()
    {
        MSDK_ZERO_MEMORY(*this);
    }
};

//template<>struct mfx_ext_buffer_id<mfxExtMVCSeqDesc>{
//    enum {id = MFX_EXTBUFF_MVC_SEQ_DESC};
//};

struct CPipelineStatistics
{
    CPipelineStatistics():
        m_input_count(0),
        m_output_count(0),
        m_synced_count(0),
        m_tick_overall(0),
        m_tick_fread(0),
        m_tick_fwrite(0),
        m_timer_overall(m_tick_overall)
    {
    }
    virtual ~CPipelineStatistics(){}

    mfxU32 m_input_count;     // number of received incoming packets (frames or bitstreams)
    mfxU32 m_output_count;    // number of delivered outgoing packets (frames or bitstreams)
    mfxU32 m_synced_count;

    msdk_tick m_tick_overall; // overall time passed during processing
    msdk_tick m_tick_fread;   // part of tick_overall: time spent to receive incoming data
    msdk_tick m_tick_fwrite;  // part of tick_overall: time spent to deliver outgoing data

    CAutoTimer m_timer_overall; // timer which corresponds to m_tick_overall

private:
    CPipelineStatistics(const CPipelineStatistics&);
    void operator=(const CPipelineStatistics&);
};

class CDecodingPipeline:
    public CBuffering,
    public CPipelineStatistics
{
public:
    CDecodingPipeline();
    virtual ~CDecodingPipeline();

    virtual mfxStatus Init(sInputParams *pParams);
    virtual mfxStatus RunDecoding();
    virtual void Close();
    virtual mfxStatus ResetDecoder(sInputParams *pParams);
    virtual mfxStatus ResetDevice();

    void SetMultiView();
    void SetExtBuffersFlag()       { m_bIsExtBuffers = true; }
    virtual void PrintInfo();
    int m_deviceid;
protected: // functions
    virtual mfxStatus CreateRenderingWindow(sInputParams *pParams, bool try_s3d);
    virtual mfxStatus InitMfxParams(sInputParams *pParams);

    // function for allocating a specific external buffer
    template <typename Buffer>
    mfxStatus AllocateExtBuffer();
    virtual void DeleteExtBuffers();

    virtual mfxStatus AllocateExtMVCBuffers();
    virtual void    DeallocateExtMVCBuffers();

    virtual void AttachExtParam();

    virtual mfxStatus CreateAllocator();
    virtual mfxStatus CreateHWDevice();
    virtual mfxStatus AllocFrames();
    virtual void DeleteFrames();
    virtual void DeleteAllocator();

    /** \brief Performs SyncOperation on the current output surface with the specified timeout.
     *
     * @return MFX_ERR_NONE Output surface was successfully synced and delivered.
     * @return MFX_ERR_MORE_DATA Array of output surfaces is empty, need to feed decoder.
     * @return MFX_WRN_IN_EXECUTION Specified timeout have elapsed.
     * @return MFX_ERR_UNKNOWN An error has occurred.
     */
    virtual mfxStatus SyncOutputSurface(mfxU32 wait);
    virtual mfxStatus DeliverOutput(mfxFrameSurface1* frame);
    virtual void PrintPerFrameStat(bool force = false);

    virtual mfxStatus DeliverLoop(void);

    static unsigned int MFX_STDCALL DeliverThreadFunc(void* ctx);

    virtual mfxStatus ReadFrameFromBuffer(mfxBitstream* pBS);


protected: // variables
//    CSmplYUVWriter          m_FileWriter;
//    std::auto_ptr<CSmplBitstreamReader>  m_FileReader;

    decodepool* m_InputReader;
    transcodepool* m_OutputWriter;

    mfxBitstream            m_mfxBS; // contains encoded data

    MFXVideoSession         m_mfxSession;
    MFXVideoDECODE*         m_pmfxDEC;
    mfxVideoParam           m_mfxVideoParams;
    std::auto_ptr<MFXVideoUSER>  m_pUserModule;
    std::auto_ptr<MFXPlugin> m_pPlugin;
    std::vector<mfxExtBuffer *> m_ExtBuffers;

    MFXFrameAllocator*      m_pMFXAllocator;
    mfxAllocatorParams*     m_pmfxAllocatorParams;
    MemType                 m_memType;      // memory type of surfaces to use
    bool                    m_bExternalAlloc; // use memory allocator as external for Media SDK
    mfxFrameAllocResponse   m_mfxResponse; // memory allocation response for decoder

    msdkFrameSurface*       m_pCurrentFreeSurface; // surface detached from free surfaces array
    msdkOutputSurface*      m_pCurrentFreeOutputSurface; // surface detached from free output surfaces array
    msdkOutputSurface*      m_pCurrentOutputSurface; // surface detached from output surfaces array

    MSDKSemaphore*          m_pDeliverOutputSemaphore; // to access to DeliverOutput method
    MSDKEvent*              m_pDeliveredEvent; // to signal when output surfaces will be processed
    mfxStatus               m_error; // error returned by DeliverOutput method
    bool                    m_bStopDeliverLoop;

    eWorkMode               m_eWorkMode; // work mode for the pipeline
    bool                    m_bIsMVC; // enables MVC mode (need to support several files as an output)
    bool                    m_bIsExtBuffers; // indicates if external buffers were allocated
    bool                    m_bIsVideoWall; // indicates special mode: decoding will be done in a loop
    bool                    m_bIsCompleteFrame;
    bool                    m_bPrintLatency;

    mfxU32                  m_nTimeout; // enables timeout for video playback, measured in seconds
    mfxU32                  m_nMaxFps; // limit of fps, if isn't specified equal 0.
    mfxU32                  m_nFrames; //limit number of output frames

    std::vector<msdk_tick>  m_vLatency;

    CHWDevice               *m_hwdev;
#if D3D_SURFACES_SUPPORT
    IGFXS3DControl          *m_pS3DControl;

    CDecodeD3DRender         m_d3dRender;
#endif

private:
    CDecodingPipeline(const CDecodingPipeline&);
    void operator=(const CDecodingPipeline&);

};

#endif // __PIPELINE_DECODE_H__
