#ifndef __PIPELINE_ENCODE_H__
#define __PIPELINE_ENCODE_H__

#include "hw_device.h"
#include "vaapi_device.h"

#ifdef D3D_SURFACES_SUPPORT
#pragma warning(disable : 4201)
#endif

#include "base_allocator.h"

#include "mfxmvc.h"
#include "mfxvideo.h"
#include "mfxvp8.h"
#include "mfxvideo++.h"
#include "mfxplugin.h"
#include "mfxplugin++.h"
#include "mfxla.h"
#include "transcodepool.h"
#include <vector>
#include <memory>
#include "outudppool.h"

enum {
    MVC_DISABLED          = 0x0,
    MVC_ENABLED           = 0x1,
    MVC_VIEWOUTPUT        = 0x2,    // 2 output bitstreams
};

struct sParams
{
    int ndeviceid;
    mfxU16 nTargetUsage;
    mfxU32 CodecId;
    mfxU32 ColorFormat;
    mfxU16 nPicStruct;
    mfxU16 nWidth; // source picture width
    mfxU16 nHeight; // source picture height
    mfxF64 dFrameRate;
    mfxU16 nBitRate;
    mfxU16 MVC_flags;
    mfxU16 nQuality; // quality parameter for JPEG encoder
    mfxU32 numViews; // number of views for Multi-View Codec
    mfxU16 nDstWidth; // destination picture width, specified if resizing required
    mfxU16 nDstHeight; // destination picture height, specified if resizing required
    bool bUseHWLib; // true if application wants to use HW MSDK library
    msdk_char strSrcFile[MSDK_MAX_FILENAME_LEN];

    std::vector<msdk_char*> srcFileBuff;
    std::vector<msdk_char*> dstFileBuff;

    mfxU32  HEVCPluginVersion;
    mfxU8 nRotationAngle; // if specified, enables rotation plugin in mfx pipeline
    msdk_char strPluginDLLPath[MSDK_MAX_FILENAME_LEN]; // plugin dll path and name
    mfxU16 nAsyncDepth; // depth of asynchronous pipeline, this number can be tuned to achieve better performance
    mfxU16 nRateControlMethod;
    mfxU16 nLADepth; // depth of the look ahead bitrate control  algorithm
    mfxU16 nMaxSliceSize; //maximum size of slice
    mfxU16 nQPI;
    mfxU16 nQPP;
    mfxU16 nQPB;
    mfxU16 nNumSlice;
    //Capture
    transcodepool* vd;
//    struct capturebuffer* vbbuffer;
    mfxU16 nCodecLevel;
    mfxU16 nCodecProfile;
    mfxU16 nGopRefDist;
    mfxU16 nGopPicSize;
    mfxU16 nICQQuality;
    mfxU16 nGopOptFlag;
    mfxU16  BufferSizeInKB;
};

struct sTask
{
    mfxBitstream mfxBS;
    mfxSyncPoint EncSyncP;
    std::list<mfxSyncPoint> DependentVppTasks;
    outudppool*  m_pLoopListBuffer;
    PSAMPLE m_pSample;

    sTask();
    mfxStatus WriteBitstream();
    mfxStatus Reset();
    mfxStatus Init(mfxU32 nBufferSize, outudppool*  pLoopListBuffer = NULL, PSAMPLE pSample = NULL );
    mfxStatus Close();
    mfxStatus InitMfxBitstream(mfxBitstream* pBitstream, mfxU32 nSize);
};

class CEncTaskPool
{
public:
    CEncTaskPool();
    virtual ~CEncTaskPool();

    virtual mfxStatus Init(MFXVideoSession* pmfxSession, outudppool*  pLoopListBuffer,
                           mfxU32 nPoolSize, mfxU32 nBufferSize, PSAMPLE pSample);
    virtual mfxStatus GetFreeTask(sTask **ppTask);
    virtual mfxStatus SynchronizeFirstTask();
    virtual void Close();

protected:
    sTask* m_pTasks;
    mfxU32 m_nPoolSize;
    mfxU32 m_nTaskBufferStart;
    MFXVideoSession* m_pMfxSession;

    virtual mfxU32 GetFreeTaskIndex();
};

/* This class implements a pipeline with 2 mfx components: vpp (video preprocessing) and encode */
class CEncodingPipeline
{
public:
    CEncodingPipeline();
    virtual ~CEncodingPipeline();

    virtual mfxStatus Init(sParams *pParams);
    virtual mfxStatus Run();
    virtual void Close();
    virtual mfxStatus ResetMFXComponents(sParams* pParams);
    void SetNumView(mfxU32 numViews) { m_nNumView = numViews; }
    bool GetBuffer( PSAMPLE pSample );
    int GetSampleCount();
    bool GetTimeStamp(unsigned long &lTimeStamp);
    int GetBitRate();
    void ClearVideoBuffer();
    void Quit();
    void StopEncoder( bool bStop );
    int m_deviceid;
protected:
    mfxEncodeCtrl m_EncodeCtrl;
    outudppool*  m_pLoopListBuffer;
    CEncTaskPool m_TaskPool;
    PSAMPLE       m_pSample;

//    mfxExtLAControl  m_ExtLAControl;
    MFXVideoSession m_mfxSession;
    MFXVideoENCODE* m_pMfxENC;
    MFXVideoVPP* m_pMfxVPP;

    mfxVideoParam m_MfxEncParams;
    mfxVideoParam m_mfxVppParams;

    mfxU16 m_MVCflags; // MVC codec is in use

    std::auto_ptr<MFXVideoUSER> m_pUserModule;
    std::auto_ptr<MFXPlugin> m_pPlugin;

    MFXFrameAllocator* m_pMFXAllocator;
    mfxAllocatorParams* m_pmfxAllocatorParams;
    bool m_bExternalAlloc; // use memory allocator as external for Media SDK

    mfxFrameSurface1* m_pEncSurfaces; // frames array for encoder input (vpp output)
    mfxFrameSurface1* m_pVppSurfaces; // frames array for vpp input
    mfxFrameAllocResponse m_EncResponse;  // memory allocation response for encoder
    mfxFrameAllocResponse m_VppResponse;  // memory allocation response for vpp

    mfxU32 m_nNumView;

    // for disabling VPP algorithms
    mfxExtVPPDoNotUse m_VppDoNotUse;
    // for MVC encoder and VPP configuration
    mfxExtMVCSeqDesc m_MVCSeqDesc;
    mfxExtCodingOption m_CodingOption;
    // for look ahead BRC configuration
    mfxExtCodingOption2 m_CodingOption2;
    mfxExtCodingOption3 m_CodingOption3;

    // external parameters for each component are stored in a vector
    std::vector<mfxExtBuffer*> m_VppExtParams;
    std::vector<mfxExtBuffer*> m_EncExtParams;

    CHWDevice *m_pHwDev;
    transcodepool* m_pVd;
    bool              m_bSelfCaluBuffsizeInKB;
    bool               m_bExitApplication;
    bool               m_bStopEncoder;

private:
    virtual mfxStatus InitMfxEncParams(sParams *pParams);
    virtual mfxStatus InitMfxVppParams(sParams *pParams);
    virtual mfxStatus InitSaveBuffer( int nW, int nH );
    virtual mfxStatus AllocAndInitVppDoNotUse();
    virtual void FreeVppDoNotUse();
    virtual mfxStatus AllocAndInitMVCSeqDesc();
    virtual void FreeMVCSeqDesc();
    virtual mfxStatus CreateAllocator();
    virtual void DeleteAllocator();
    virtual mfxStatus CreateHWDevice();
    virtual void DeleteHWDevice();
    virtual mfxStatus AllocFrames();
    virtual void DeleteFrames();
    mfxStatus AllocateSufficientBuffer(mfxBitstream* pBS);
    virtual mfxStatus GetFreeTask(sTask **ppTask );
    virtual mfxStatus LoadFrameFromBuffer(mfxFrameSurface1* pSurface,  unsigned long *plTimeStamp);
    mfxU16 GetFreeSurfaceIndex(mfxFrameSurface1* pSurfacesPool, mfxU16 nPoolSize);
    mfxU16 GetFreeSurface(mfxFrameSurface1* pSurfacesPool, mfxU16 nPoolSize);
    mfxStatus ExtendMfxBitstream(mfxBitstream* pBitstream, mfxU32 nSize);
    mfxStatus ConvertFrameRate(mfxF64 dFrameRate, mfxU32* pnFrameRateExtN, mfxU32* pnFrameRateExtD);
    mfxU32 GetNumber(mfxSession session = 0);
};

#endif // __PIPELINE_ENCODE_H__
