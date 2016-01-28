/*********************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2005-2014 Intel Corporation. All Rights Reserved.

**********************************************************************************/


#include "pipeline_encode.h"
#include "sysmem_allocator.h"
#include "global.h"

#if D3D_SURFACES_SUPPORT
#include "d3d_allocator.h"
#include "d3d11_allocator.h"

#include "d3d_device.h"
#include "d3d11_device.h"
#endif

#ifdef LIBVA_SUPPORT
#include "vaapi_allocator.h"
#include "vaapi_device.h"
#endif

FILE *fpout_v;
FILE *fp_yuv;

static void WipeMfxBitstream(mfxBitstream* pBitstream)
{
    MSDK_CHECK_POINTER(pBitstream);

    //free allocated memory
    MSDK_SAFE_DELETE_ARRAY(pBitstream->Data);
}

CEncTaskPool::CEncTaskPool()
{
    m_pTasks  = NULL;
    m_pMfxSession       = NULL;
    m_nTaskBufferStart  = 0;
    m_nPoolSize         = 0;
}

CEncTaskPool::~CEncTaskPool()
{
    Close();
}

mfxStatus CEncTaskPool::Init(MFXVideoSession* pmfxSession, outudppool*  pLoopListBuffer,
                             mfxU32 nPoolSize, mfxU32 nBufferSize, PSAMPLE pSample )
{
    MSDK_CHECK_POINTER(pmfxSession, MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(pLoopListBuffer, MFX_ERR_NULL_PTR);

    MSDK_CHECK_ERROR(nPoolSize, 0, MFX_ERR_UNDEFINED_BEHAVIOR);
    MSDK_CHECK_ERROR(nBufferSize, 0, MFX_ERR_UNDEFINED_BEHAVIOR);

    MSDK_CHECK_POINTER(pSample, MFX_ERR_NULL_PTR);

    m_pMfxSession = pmfxSession;
    m_nPoolSize = nPoolSize;

    m_pTasks = new sTask [m_nPoolSize];
    MSDK_CHECK_POINTER(m_pTasks, MFX_ERR_MEMORY_ALLOC);

    mfxStatus sts = MFX_ERR_NONE;

    for ( mfxU32 i = 0; i < m_nPoolSize; i++)
    {
        sts = m_pTasks[i].Init( nBufferSize, pLoopListBuffer, pSample );
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
    }
    fpout_v = fopen("transcodeV.264","wb+");
    fp_yuv = fopen("tempV.yuv","ab+");

    return MFX_ERR_NONE;
}

mfxStatus CEncTaskPool::SynchronizeFirstTask()
{
    MSDK_CHECK_POINTER(m_pTasks, MFX_ERR_NOT_INITIALIZED);
    MSDK_CHECK_POINTER(m_pMfxSession, MFX_ERR_NOT_INITIALIZED);

    mfxStatus sts  = MFX_ERR_NONE;

    // non-null sync point indicates that task is in execution
    if (NULL != m_pTasks[m_nTaskBufferStart].EncSyncP)
    {
        sts = m_pMfxSession->SyncOperation(m_pTasks[m_nTaskBufferStart].EncSyncP, MSDK_WAIT_INTERVAL);

        if (MFX_ERR_NONE == sts)
        {
            /*  写输出流 */
            sts = m_pTasks[m_nTaskBufferStart].WriteBitstream();
            MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

            sts = m_pTasks[m_nTaskBufferStart].Reset();
            MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

            // move task buffer start to the next executing task
            // the first transform frame to the right with non zero sync point
            for (mfxU32 i = 0; i < m_nPoolSize; i++)
            {
                m_nTaskBufferStart = (m_nTaskBufferStart + 1) % m_nPoolSize;
                if (NULL != m_pTasks[m_nTaskBufferStart].EncSyncP)
                {
                    break;
                }
            }
        }
        else if (MFX_ERR_ABORTED == sts)
        {
            while (!m_pTasks[m_nTaskBufferStart].DependentVppTasks.empty())
            {
                // find out if the error occurred in a VPP task to perform recovery procedure if applicable
                sts = m_pMfxSession->SyncOperation(*m_pTasks[m_nTaskBufferStart].DependentVppTasks.begin(), 0);

                if (MFX_ERR_NONE == sts)
                {
                    m_pTasks[m_nTaskBufferStart].DependentVppTasks.pop_front();
                    sts = MFX_ERR_ABORTED; // save the status of the encode task
                    continue; // go to next vpp task
                }
                else
                {
                    break;
                }
            }
        }

        return sts;
    }
    else
    {
        return MFX_ERR_NOT_FOUND; // no tasks left in task buffer
    }
}

mfxU32 CEncTaskPool::GetFreeTaskIndex()
{
    mfxU32 off = 0;

    if (m_pTasks)
    {
        for (off = 0; off < m_nPoolSize; off++)
        {
            if (NULL == m_pTasks[(m_nTaskBufferStart + off) % m_nPoolSize].EncSyncP)
            {
                break;
            }
        }
    }

    if (off >= m_nPoolSize)
        return m_nPoolSize;

    return (m_nTaskBufferStart + off) % m_nPoolSize;
}

mfxStatus CEncTaskPool::GetFreeTask(sTask **ppTask)
{
    MSDK_CHECK_POINTER(ppTask, MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(m_pTasks, MFX_ERR_NOT_INITIALIZED);

    mfxU32 index = GetFreeTaskIndex();

    if (index >= m_nPoolSize)
    {
        return MFX_ERR_NOT_FOUND;
    }

    // return the address of the task
    *ppTask = &m_pTasks[index];

    return MFX_ERR_NONE;
}

void CEncTaskPool::Close()
{
    if (m_pTasks)
    {
        for (mfxU32 i = 0; i < m_nPoolSize; i++)
        {
            m_pTasks[i].Close();
        }
    }

    MSDK_SAFE_DELETE_ARRAY(m_pTasks);

    m_pMfxSession = NULL;
    m_nTaskBufferStart = 0;
    m_nPoolSize = 0;
}

sTask::sTask()
    : EncSyncP(0)
    , m_pLoopListBuffer( NULL )
    , m_pSample( NULL )
{
    MSDK_ZERO_MEMORY(mfxBS);
}

mfxStatus sTask::Init( mfxU32 nBufferSize, outudppool*  pLoopListBuffer, PSAMPLE pSample )
{
    Close();

    m_pLoopListBuffer = pLoopListBuffer;
    m_pSample = pSample;

    mfxStatus sts = Reset();
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    sts = InitMfxBitstream(&mfxBS, nBufferSize);
    MSDK_CHECK_RESULT_SAFE(sts, MFX_ERR_NONE, sts, WipeMfxBitstream(&mfxBS));

    return sts;
}

mfxStatus sTask::Close()
{
    WipeMfxBitstream(&mfxBS);
    EncSyncP = 0;
    DependentVppTasks.clear();

    return MFX_ERR_NONE;
}

mfxStatus sTask::WriteBitstream()
{
    if ( !m_pLoopListBuffer || !m_pSample )
        return MFX_ERR_NOT_INITIALIZED;

    // write buffer
    memcpy( &(m_pSample->abySample[0]), mfxBS.Data + mfxBS.DataOffset, mfxBS.DataLength );
    m_pSample->lSampleLength = mfxBS.DataLength;
    m_pSample->lTimeStamp = mfxBS.TimeStamp;
    m_pSample->lDecodeTimeStamp = mfxBS.DecodeTimeStamp;

//    if(m_pLoopListBuffer->fpVideo)
        fwrite(m_pSample->abySample,m_pSample->lSampleLength,1,fpout_v);

    //写输出
//    m_pLoopListBuffer->Write( m_pSample, bVIDEO);


    // mark that we don't need bit stream data any more
    mfxBS.DataLength = 0;

    return MFX_ERR_NONE;
}

mfxStatus sTask::Reset()
{
    // mark sync point as free
    EncSyncP = NULL;

    // prepare bit stream
    mfxBS.DataOffset = 0;
    mfxBS.DataLength = 0;
    mfxBS.TimeStamp = 0;

    DependentVppTasks.clear();

    return MFX_ERR_NONE;
}
mfxStatus sTask::InitMfxBitstream(mfxBitstream* pBitstream, mfxU32 nSize)
{
    //check input params
    MSDK_CHECK_POINTER(pBitstream, MFX_ERR_NULL_PTR);
    MSDK_CHECK_ERROR(nSize, 0, MFX_ERR_NOT_INITIALIZED);

    //prepare pBitstream
    WipeMfxBitstream(pBitstream);

    //prepare buffer
    pBitstream->Data = new mfxU8[nSize];
    MSDK_CHECK_POINTER(pBitstream->Data, MFX_ERR_MEMORY_ALLOC);

    pBitstream->MaxLength = nSize;

    return MFX_ERR_NONE;
}

mfxStatus CEncodingPipeline::AllocAndInitMVCSeqDesc()
{
    // a simple example of mfxExtMVCSeqDesc structure filling
    // actually equal to the "Default dependency mode" - when the structure fields are left 0,
    // but we show how to properly allocate and fill the fields

    mfxU32 i;

    // mfxMVCViewDependency array
    m_MVCSeqDesc.NumView = m_nNumView;
    m_MVCSeqDesc.NumViewAlloc = m_nNumView;
    m_MVCSeqDesc.View = new mfxMVCViewDependency[m_MVCSeqDesc.NumViewAlloc];
    MSDK_CHECK_POINTER(m_MVCSeqDesc.View, MFX_ERR_MEMORY_ALLOC);
    for (i = 0; i < m_MVCSeqDesc.NumViewAlloc; ++i)
    {
        MSDK_ZERO_MEMORY(m_MVCSeqDesc.View[i]);
        m_MVCSeqDesc.View[i].ViewId = (mfxU16) i; // set view number as view id
    }

    // set up dependency for second view
    m_MVCSeqDesc.View[1].NumAnchorRefsL0 = 1;
    m_MVCSeqDesc.View[1].AnchorRefL0[0] = 0;     // ViewId 0 - base view

    m_MVCSeqDesc.View[1].NumNonAnchorRefsL0 = 1;
    m_MVCSeqDesc.View[1].NonAnchorRefL0[0] = 0;  // ViewId 0 - base view

    // viewId array
    m_MVCSeqDesc.NumViewId = m_nNumView;
    m_MVCSeqDesc.NumViewIdAlloc = m_nNumView;
    m_MVCSeqDesc.ViewId = new mfxU16[m_MVCSeqDesc.NumViewIdAlloc];
    MSDK_CHECK_POINTER(m_MVCSeqDesc.ViewId, MFX_ERR_MEMORY_ALLOC);
    for (i = 0; i < m_MVCSeqDesc.NumViewIdAlloc; ++i)
    {
        m_MVCSeqDesc.ViewId[i] = (mfxU16) i;
    }

    // create a single operation point containing all views
    m_MVCSeqDesc.NumOP = 1;
    m_MVCSeqDesc.NumOPAlloc = 1;
    m_MVCSeqDesc.OP = new mfxMVCOperationPoint[m_MVCSeqDesc.NumOPAlloc];
    MSDK_CHECK_POINTER(m_MVCSeqDesc.OP, MFX_ERR_MEMORY_ALLOC);
    for (i = 0; i < m_MVCSeqDesc.NumOPAlloc; ++i)
    {
        MSDK_ZERO_MEMORY(m_MVCSeqDesc.OP[i]);
        m_MVCSeqDesc.OP[i].NumViews = (mfxU16) m_nNumView;
        m_MVCSeqDesc.OP[i].NumTargetViews = (mfxU16) m_nNumView;
        m_MVCSeqDesc.OP[i].TargetViewId = m_MVCSeqDesc.ViewId; // points to mfxExtMVCSeqDesc::ViewId
    }

    return MFX_ERR_NONE;
}

mfxStatus CEncodingPipeline::AllocAndInitVppDoNotUse()
{
    m_VppDoNotUse.NumAlg = 4;

    m_VppDoNotUse.AlgList = new mfxU32 [m_VppDoNotUse.NumAlg];
    MSDK_CHECK_POINTER(m_VppDoNotUse.AlgList,  MFX_ERR_MEMORY_ALLOC);

    m_VppDoNotUse.AlgList[0] = MFX_EXTBUFF_VPP_DENOISE; // turn off denoising (on by default)
    m_VppDoNotUse.AlgList[1] = MFX_EXTBUFF_VPP_SCENE_ANALYSIS; // turn off scene analysis (on by default)
    m_VppDoNotUse.AlgList[2] = MFX_EXTBUFF_VPP_DETAIL; // turn off detail enhancement (on by default)
    m_VppDoNotUse.AlgList[3] = MFX_EXTBUFF_VPP_PROCAMP; // turn off processing amplified (on by default)

    return MFX_ERR_NONE;

} // CEncodingPipeline::AllocAndInitVppDoNotUse()

void CEncodingPipeline::FreeMVCSeqDesc()
{
    MSDK_SAFE_DELETE_ARRAY(m_MVCSeqDesc.View);
    MSDK_SAFE_DELETE_ARRAY(m_MVCSeqDesc.ViewId);
    MSDK_SAFE_DELETE_ARRAY(m_MVCSeqDesc.OP);
}

void CEncodingPipeline::FreeVppDoNotUse()
{
    MSDK_SAFE_DELETE_ARRAY(m_VppDoNotUse.AlgList);
}
#include <QDebug>

mfxStatus CEncodingPipeline::InitMfxEncParams(sParams *pInParams)
{
    m_MfxEncParams.mfx.CodecLevel = pInParams->nCodecLevel;
    m_MfxEncParams.mfx.CodecProfile = pInParams->nCodecProfile;
    m_MfxEncParams.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;// pInParams->nPicStruct;
    m_MfxEncParams.mfx.RateControlMethod = pInParams->nRateControlMethod;
    m_MfxEncParams.mfx.GopPicSize = pInParams->nGopPicSize;
    m_MfxEncParams.mfx.GopRefDist = pInParams->nGopRefDist;

//    //transcode add
//    m_MfxEncParams.mfx.GopOptFlag = MFX_GOP_STRICT;
//    m_MfxEncParams.mfx.RateControlMethod = MFX_RATECONTROL_CQP;

    m_MfxEncParams.mfx.IdrInterval = 0;
    if ( m_MfxEncParams.mfx.RateControlMethod == MFX_RATECONTROL_CBR )
    {
        m_MfxEncParams.mfx.BufferSizeInKB = pInParams->BufferSizeInKB;
        m_MfxEncParams.mfx.MaxKbps = pInParams->nBitRate;
        m_MfxEncParams.mfx.TargetKbps = pInParams->nBitRate;
    }
    else if( m_MfxEncParams.mfx.RateControlMethod == MFX_RATECONTROL_AVBR )
    {
        m_MfxEncParams.mfx.BufferSizeInKB = 3*pInParams->nBitRate;
        m_MfxEncParams.mfx.TargetKbps = pInParams->nBitRate;
        m_MfxEncParams.mfx.Accuracy = 300;
        m_MfxEncParams.mfx.Convergence = 2500;
    }
    else if( m_MfxEncParams.mfx.RateControlMethod == MFX_RATECONTROL_ICQ ||
             m_MfxEncParams.mfx.RateControlMethod == MFX_RATECONTROL_LA_ICQ )
    {
        //m_MfxEncParams.mfx.BufferSizeInKB = pInParams->BufferSizeInKB;
        //m_MfxEncParams.mfx.MaxKbps = pInParams->nBitRate;
        if( pInParams->nICQQuality > 51 )
            pInParams->nICQQuality = 51;
        else if( pInParams->nICQQuality <= 0 )
            pInParams->nICQQuality = 1;
        m_MfxEncParams.mfx.ICQQuality = pInParams->nICQQuality;
    }
    else if ( m_MfxEncParams.mfx.RateControlMethod == MFX_RATECONTROL_CQP )
    {
        m_MfxEncParams.mfx.QPI = pInParams->nQPI;
        m_MfxEncParams.mfx.QPP = pInParams->nQPP+5;
        m_MfxEncParams.mfx.QPB = pInParams->nQPB+12;
    }
    else if( m_MfxEncParams.mfx.RateControlMethod == MFX_RATECONTROL_LA )
    {
        m_MfxEncParams.mfx.TargetKbps = pInParams->nBitRate;
    }
    else
    {
        // for other BitRate
        m_MfxEncParams.mfx.BufferSizeInKB = pInParams->BufferSizeInKB;
        m_MfxEncParams.mfx.MaxKbps = pInParams->nBitRate;
        m_MfxEncParams.mfx.TargetKbps = pInParams->nBitRate;
    }
    m_MfxEncParams.mfx.NumSlice = pInParams->nNumSlice;
    m_MfxEncParams.mfx.TargetUsage  = pInParams->nTargetUsage; // trade-off between quality and speed
    m_MfxEncParams.mfx.GopOptFlag = pInParams->nGopOptFlag;

    m_MfxEncParams.mfx.CodecId  = pInParams->CodecId;
    ConvertFrameRate(pInParams->dFrameRate, &m_MfxEncParams.mfx.FrameInfo.FrameRateExtN, &m_MfxEncParams.mfx.FrameInfo.FrameRateExtD);
    m_MfxEncParams.mfx.EncodedOrder  = 0; // binary flag, 0 signals encoder to take frames in display order

    // specify memory type
    m_MfxEncParams.IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY;

    // frame info parameters
    m_MfxEncParams.mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
//    m_MfxEncParams.mfx.FrameInfo.FourCC = MFX_FOURCC_YV12;
    m_MfxEncParams.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;

    // set frame size and crops
    // width must be a multiple of 16
    // height must be a multiple of 16 in case of frame picture and a multiple of 32 in case of field picture
    m_MfxEncParams.mfx.FrameInfo.Width  = MSDK_ALIGN16(pInParams->nDstWidth);
    m_MfxEncParams.mfx.FrameInfo.Height = (MFX_PICSTRUCT_PROGRESSIVE == m_MfxEncParams.mfx.FrameInfo.PicStruct)?
                MSDK_ALIGN16(pInParams->nDstHeight) : MSDK_ALIGN32(pInParams->nDstHeight);

    m_MfxEncParams.mfx.FrameInfo.CropX = 0;
    m_MfxEncParams.mfx.FrameInfo.CropY = 0;
    m_MfxEncParams.mfx.FrameInfo.CropW = pInParams->nDstWidth;
    m_MfxEncParams.mfx.FrameInfo.CropH = pInParams->nDstHeight;

    // we don't specify profile and level and let the encoder choose those basing on parameters
    // we must specify profile only for MVC codec
    if (MVC_ENABLED & m_MVCflags)
    {
        m_MfxEncParams.mfx.CodecProfile = MFX_PROFILE_AVC_STEREO_HIGH;
    }

    // configure and attach external parameters
    if (MVC_ENABLED & pInParams->MVC_flags)
        m_EncExtParams.push_back((mfxExtBuffer *)&m_MVCSeqDesc);

    if (MVC_VIEWOUTPUT & pInParams->MVC_flags)
    {
        // ViewOuput option requested
        m_CodingOption.ViewOutput = MFX_CODINGOPTION_ON;
        m_EncExtParams.push_back((mfxExtBuffer *)&m_CodingOption);
    }

    // configure the depth of the look ahead BRC if specified in command line
    if (pInParams->nLADepth || pInParams->nMaxSliceSize)
    {
//        MSDK_ZERO_MEMORY(m_ExtLAControl);
//        m_ExtLAControl.Header.BufferId = MFX_EXTBUFF_LOOKAHEAD_CTRL;
//        m_ExtLAControl.Header.BufferSz = sizeof(m_ExtLAControl);
//        m_ExtLAControl.LookAheadDepth = pInParams->nLADepth;
//        m_ExtLAControl.DependencyDepth = 40;
//        m_EncExtParams.push_back((mfxExtBuffer *)&m_ExtLAControl);

        m_CodingOption2.LookAheadDepth = pInParams->nLADepth;
        //m_CodingOption2.MaxSliceSize   = 1;//pInParams->nMaxSliceSize;
        m_EncExtParams.push_back((mfxExtBuffer *)&m_CodingOption2);

//        m_CodingOption3.WinBRCMaxAvgKbps = 1200;
//        m_CodingOption3.WinBRCSize = 50;
//        m_EncExtParams.push_back((mfxExtBuffer *)&m_CodingOption3);
    }

    if (!m_EncExtParams.empty())
    {
        m_MfxEncParams.ExtParam = &m_EncExtParams[0]; // vector is stored linearly in memory
        m_MfxEncParams.NumExtParam = (mfxU16)m_EncExtParams.size();
    }

    m_MfxEncParams.AsyncDepth = 1;//pInParams->nAsyncDepth;

    return MFX_ERR_NONE;
}

mfxStatus CEncodingPipeline::InitMfxVppParams(sParams *pInParams)
{
    MSDK_CHECK_POINTER(pInParams,  MFX_ERR_NULL_PTR);

    // specify memory type
    m_mfxVppParams.IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY | MFX_IOPATTERN_OUT_SYSTEM_MEMORY;

    // input frame info
    m_mfxVppParams.vpp.In.FourCC    = MFX_FOURCC_NV12;
//    m_mfxVppParams.vpp.In.FourCC    = MFX_FOURCC_YV12;
    m_mfxVppParams.vpp.In.PicStruct = pInParams->nPicStruct;
    ConvertFrameRate(pInParams->dFrameRate, &m_mfxVppParams.vpp.In.FrameRateExtN, &m_mfxVppParams.vpp.In.FrameRateExtD);

    // width must be a multiple of 16
    // height must be a multiple of 16 in case of frame picture and a multiple of 32 in case of field picture
    m_mfxVppParams.vpp.In.Width     = MSDK_ALIGN16(pInParams->nWidth);
    m_mfxVppParams.vpp.In.Height    = (MFX_PICSTRUCT_PROGRESSIVE == m_mfxVppParams.vpp.In.PicStruct)?
                MSDK_ALIGN16(pInParams->nHeight) : MSDK_ALIGN32(pInParams->nHeight);

    // set crops in input mfxFrameInfo for correct work of file reader
    // VPP itself ignores crops at initialization
    m_mfxVppParams.vpp.In.CropW = pInParams->nWidth;
    m_mfxVppParams.vpp.In.CropH = pInParams->nHeight;

    // fill output frame info
    MSDK_MEMCPY_VAR(m_mfxVppParams.vpp.Out,&m_mfxVppParams.vpp.In, sizeof(mfxFrameInfo));

    // only resizing is supported
    m_mfxVppParams.vpp.Out.Width = MSDK_ALIGN16(pInParams->nDstWidth);
    m_mfxVppParams.vpp.Out.Height = (MFX_PICSTRUCT_PROGRESSIVE == m_mfxVppParams.vpp.Out.PicStruct)?
                MSDK_ALIGN16(pInParams->nDstHeight) : MSDK_ALIGN32(pInParams->nDstHeight);

    // configure and attach external parameters
    AllocAndInitVppDoNotUse();
    m_VppExtParams.push_back((mfxExtBuffer *)&m_VppDoNotUse);

    if (MVC_ENABLED & pInParams->MVC_flags)
        m_VppExtParams.push_back((mfxExtBuffer *)&m_MVCSeqDesc);

    m_mfxVppParams.ExtParam = &m_VppExtParams[0]; // vector is stored linearly in memory
    m_mfxVppParams.NumExtParam = (mfxU16)m_VppExtParams.size();

    m_mfxVppParams.AsyncDepth = pInParams->nAsyncDepth;
    return MFX_ERR_NONE;
}

mfxStatus CEncodingPipeline::CreateHWDevice()
{
    mfxStatus sts = MFX_ERR_NONE;

    m_pHwDev = CreateVAAPIDevice();
    if ( NULL == m_pHwDev )
        return MFX_ERR_MEMORY_ALLOC;

    sts = m_pHwDev->Init( NULL, 0, GetNumber(m_mfxSession) );
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
    return MFX_ERR_NONE;
}

bool CEncodingPipeline::GetBuffer( PSAMPLE pSample )
{
    if( NULL == m_pLoopListBuffer )
        return false;
    return m_pLoopListBuffer->Get( pSample );
}

int CEncodingPipeline::GetSampleCount()
{
    if( m_pLoopListBuffer )
        return m_pLoopListBuffer->GetSampleCount();
    return 0;
}

bool CEncodingPipeline::GetTimeStamp(unsigned long &lTimeStamp)
{
    bool bRet = false;
    if( NULL == m_pLoopListBuffer )
        return bRet;

    PSAMPLE pSample = (PSAMPLE)new BYTE[8+4096*1024];
    if( NULL == pSample )
        return bRet;

    if( m_pLoopListBuffer->Get( pSample, false ) )
    {
        lTimeStamp = pSample->lTimeStamp;
        bRet = true;
    }
    delete pSample;
    pSample = NULL;
    return bRet;
}

int CEncodingPipeline::GetBitRate()
{
    if( NULL == m_pMfxENC )
        return 0;

    mfxEncodeStat mfxStat;
    mfxStatus sts =  m_pMfxENC->GetEncodeStat( &mfxStat );
    if( MFX_ERR_NONE == sts )
    {
        if( mfxStat.NumFrame )
            return (mfxStat.NumBit/mfxStat.NumFrame*m_MfxEncParams.mfx.FrameInfo.FrameRateExtN)/1000;
    }
    return 0;
}

void CEncodingPipeline::ClearVideoBuffer()
{
    if( m_pLoopListBuffer )
        m_pLoopListBuffer->ClearBuffer();
}

void CEncodingPipeline::Quit()
{
    m_bExitApplication = true;
}

void CEncodingPipeline::StopEncoder( bool bStop )
{
    m_bStopEncoder = bStop;
}

mfxStatus CEncodingPipeline::AllocFrames()
{
    MSDK_CHECK_POINTER(m_pMfxENC, MFX_ERR_NOT_INITIALIZED);

    mfxStatus sts = MFX_ERR_NONE;
    mfxFrameAllocRequest EncRequest;
    mfxFrameAllocRequest VppRequest[2];

    mfxU16 nEncSurfNum = 0; // number of surfaces for encoder
    mfxU16 nVppSurfNum = 0; // number of surfaces for vpp

    MSDK_ZERO_MEMORY(EncRequest);
    MSDK_ZERO_MEMORY(VppRequest[0]);
    MSDK_ZERO_MEMORY(VppRequest[1]);

    // Calculate the number of surfaces for components.
    // QueryIOSurf functions tell how many surfaces are required to produce at least 1 output.
    // To achieve better performance we provide extra surfaces.
    // 1 extra surface at input allows to get 1 extra output.

    sts = m_pMfxENC->QueryIOSurf(&m_MfxEncParams, &EncRequest);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    if (EncRequest.NumFrameSuggested < m_MfxEncParams.AsyncDepth)
        return MFX_ERR_MEMORY_ALLOC;

    // The number of surfaces shared by vpp output and encode input.
    nEncSurfNum = EncRequest.NumFrameSuggested;

    if (m_pMfxVPP)
    {
        // VppRequest[0] for input frames request, VppRequest[1] for output frames request
        sts = m_pMfxVPP->QueryIOSurf(&m_mfxVppParams, VppRequest);
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

        // The number of surfaces for vpp input - so that vpp can work at async depth = m_nAsyncDepth
        nVppSurfNum = VppRequest[0].NumFrameSuggested;
        // If surfaces are shared by 2 components, c1 and c2. NumSurf = c1_out + c2_in - AsyncDepth + 1
        nEncSurfNum += nVppSurfNum - m_MfxEncParams.AsyncDepth + 1;
    }

    // prepare allocation requests
    EncRequest.NumFrameSuggested = EncRequest.NumFrameMin = nEncSurfNum;
    MSDK_MEMCPY_VAR(EncRequest.Info, &(m_MfxEncParams.mfx.FrameInfo), sizeof(mfxFrameInfo));
    if (m_pMfxVPP)
    {
        EncRequest.Type |= MFX_MEMTYPE_FROM_VPPOUT; // surfaces are shared between vpp output and encode input
    }

    // alloc frames for encoder
    sts = m_pMFXAllocator->Alloc(m_pMFXAllocator->pthis, &EncRequest, &m_EncResponse);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // alloc frames for vpp if vpp is enabled
    if (m_pMfxVPP)
    {
        VppRequest[0].NumFrameSuggested = VppRequest[0].NumFrameMin = nVppSurfNum;
        MSDK_MEMCPY_VAR(VppRequest[0].Info, &(m_mfxVppParams.mfx.FrameInfo), sizeof(mfxFrameInfo));

        sts = m_pMFXAllocator->Alloc(m_pMFXAllocator->pthis, &(VppRequest[0]), &m_VppResponse);
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
    }

    // prepare mfxFrameSurface1 array for encoder
    m_pEncSurfaces = new mfxFrameSurface1 [m_EncResponse.NumFrameActual];
    MSDK_CHECK_POINTER(m_pEncSurfaces, MFX_ERR_MEMORY_ALLOC);

    for (int i = 0; i < m_EncResponse.NumFrameActual; i++)
    {
        memset(&(m_pEncSurfaces[i]), 0, sizeof(mfxFrameSurface1));
        MSDK_MEMCPY_VAR(m_pEncSurfaces[i].Info, &(m_MfxEncParams.mfx.FrameInfo), sizeof(mfxFrameInfo));

        if (m_bExternalAlloc)
        {
            m_pEncSurfaces[i].Data.MemId = m_EncResponse.mids[i];
        }
        else
        {
            // get YUV pointers
            sts = m_pMFXAllocator->Lock(m_pMFXAllocator->pthis, m_EncResponse.mids[i], &(m_pEncSurfaces[i].Data));
            MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
        }
    }

    // prepare mfxFrameSurface1 array for vpp if vpp is enabled
    if (m_pMfxVPP)
    {
        m_pVppSurfaces = new mfxFrameSurface1 [m_VppResponse.NumFrameActual];
        MSDK_CHECK_POINTER(m_pVppSurfaces, MFX_ERR_MEMORY_ALLOC);

        for (int i = 0; i < m_VppResponse.NumFrameActual; i++)
        {
            MSDK_ZERO_MEMORY(m_pVppSurfaces[i]);
            MSDK_MEMCPY_VAR(m_pVppSurfaces[i].Info, &(m_mfxVppParams.mfx.FrameInfo), sizeof(mfxFrameInfo));

            if (m_bExternalAlloc)
            {
                m_pVppSurfaces[i].Data.MemId = m_VppResponse.mids[i];
            }
            else
            {
                sts = m_pMFXAllocator->Lock(m_pMFXAllocator->pthis, m_VppResponse.mids[i], &(m_pVppSurfaces[i].Data));
                MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
            }
        }
    }

    return MFX_ERR_NONE;
}

mfxStatus CEncodingPipeline::CreateAllocator()
{
    mfxStatus sts = MFX_ERR_NONE;

    //in case of system memory allocator we also have to pass MFX_HANDLE_VA_DISPLAY to HW library
    mfxIMPL impl;
    m_mfxSession.QueryIMPL( &impl );

    if( MFX_IMPL_HARDWARE == MFX_IMPL_BASETYPE( impl ) )
    {
        sts = CreateHWDevice();
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

        mfxHDL hdl = NULL;
        sts = m_pHwDev->GetHandle( MFX_HANDLE_VA_DISPLAY, &hdl ) ;
        // provide device manager to MediaSDK
        sts = m_mfxSession.SetHandle( MFX_HANDLE_VA_DISPLAY, hdl );
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
    }

    // create system memory allocator
    m_pMFXAllocator = new SysMemFrameAllocator;
    MSDK_CHECK_POINTER(m_pMFXAllocator, MFX_ERR_MEMORY_ALLOC);

    /* In case of system memory we demonstrate "no external allocator" usage model.
        We don't call SetAllocator, Media SDK uses internal allocator.
        We use system memory allocator simply as a memory manager for application*/

    // initialize memory allocator
    sts = m_pMFXAllocator->Init(m_pmfxAllocatorParams);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    return MFX_ERR_NONE;
}

void CEncodingPipeline::DeleteFrames()
{
    // delete surfaces array
    MSDK_SAFE_DELETE_ARRAY(m_pEncSurfaces);
    MSDK_SAFE_DELETE_ARRAY(m_pVppSurfaces);

    // delete frames
    if (m_pMFXAllocator)
    {
        m_pMFXAllocator->Free(m_pMFXAllocator->pthis, &m_EncResponse);
        m_pMFXAllocator->Free(m_pMFXAllocator->pthis, &m_VppResponse);
    }
}

void CEncodingPipeline::DeleteHWDevice()
{
    MSDK_SAFE_DELETE(m_pHwDev);
}

void CEncodingPipeline::DeleteAllocator()
{
    // delete allocator
    MSDK_SAFE_DELETE(m_pMFXAllocator);
    MSDK_SAFE_DELETE(m_pmfxAllocatorParams);

    DeleteHWDevice();
}

CEncodingPipeline::CEncodingPipeline()
{
    memset(&m_EncodeCtrl, 0, sizeof(mfxEncodeCtrl));
    m_pMfxENC = NULL;
    m_pMfxVPP = NULL;
    m_pMFXAllocator = NULL;
    m_pmfxAllocatorParams = NULL;
    m_bExternalAlloc = false;
    m_pEncSurfaces = NULL;
    m_pVppSurfaces = NULL;

    m_MVCflags = MVC_DISABLED;
    m_nNumView = 0;

    m_pLoopListBuffer = NULL;
    m_pSample = NULL;
    m_pVd = NULL;
    m_bSelfCaluBuffsizeInKB = false;

//    MSDK_ZERO_MEMORY(m_ExtLAControl);
//    m_ExtLAControl.Header.BufferId = MFX_EXTBUFF_LOOKAHEAD_CTRL;
//    m_ExtLAControl.Header.BufferSz = sizeof(m_ExtLAControl);

    MSDK_ZERO_MEMORY(m_MVCSeqDesc);
    m_MVCSeqDesc.Header.BufferId = MFX_EXTBUFF_MVC_SEQ_DESC;
    m_MVCSeqDesc.Header.BufferSz = sizeof(m_MVCSeqDesc);

    MSDK_ZERO_MEMORY(m_VppDoNotUse);
    m_VppDoNotUse.Header.BufferId = MFX_EXTBUFF_VPP_DONOTUSE;
    m_VppDoNotUse.Header.BufferSz = sizeof(m_VppDoNotUse);

    MSDK_ZERO_MEMORY(m_CodingOption);
    m_CodingOption.Header.BufferId = MFX_EXTBUFF_CODING_OPTION;
    m_CodingOption.Header.BufferSz = sizeof(m_CodingOption);
    //  wcl  2015.03.16 add
    m_CodingOption.CAVLC = MFX_CODINGOPTION_OFF;
    m_CodingOption.EndOfStream = MFX_CODINGOPTION_OFF;
    MSDK_ZERO_MEMORY(m_CodingOption2);
    m_CodingOption2.Header.BufferId = MFX_EXTBUFF_CODING_OPTION2;
    m_CodingOption2.Header.BufferSz = sizeof(m_CodingOption2);
    // wcl  2015.03.16
    m_CodingOption2.MaxFrameSize = 5000000;
    m_CodingOption2.RepeatPPS = MFX_CODINGOPTION_OFF;
    m_CodingOption2.BitrateLimit = MFX_CODINGOPTION_OFF;


    MSDK_ZERO_MEMORY(m_CodingOption3);
    m_CodingOption3.Header.BufferId = MFX_EXTBUFF_CODING_OPTION3;
    m_CodingOption3.Header.BufferSz = sizeof(m_CodingOption3);

#if D3D_SURFACES_SUPPORT
    m_pHwDev = NULL;
#endif

    MSDK_ZERO_MEMORY(m_MfxEncParams);
    MSDK_ZERO_MEMORY(m_mfxVppParams);

    MSDK_ZERO_MEMORY(m_EncResponse);
    MSDK_ZERO_MEMORY(m_VppResponse);

    m_bExitApplication = false;
    m_bStopEncoder = false;


}

CEncodingPipeline::~CEncodingPipeline()
{
    Close();
    if( m_pLoopListBuffer )
    {
        delete m_pLoopListBuffer;
        m_pLoopListBuffer = NULL;
    }
    if( m_pSample )
    {
        delete m_pSample;
        m_pSample = NULL;
    }
}

mfxStatus CEncodingPipeline::InitSaveBuffer( int nW, int nH )
{
    mfxStatus sts = MFX_ERR_NONE;



    if(  NULL == m_pLoopListBuffer )
    {
        long lPitch  = ((nW &~ 15) * 12+ 7) / 8;
        long lHeight = (nH + 31) &~ 31;
        m_pLoopListBuffer = g_pLoopListBuffer[m_deviceid];
        //m_pLoopListBuffer  = new CLoopListBuffer(lPitch*lHeight*2);
//        if(m_deviceid == 0)
//        {
//            FILE *fpVideo = fopen("cctv0.264", "wa+");
//            m_pLoopListBuffer->fpVideo = fpVideo;
//        }
        MSDK_CHECK_POINTER(m_pLoopListBuffer, MFX_ERR_MEMORY_ALLOC);
    }
    if( NULL == m_pSample )
    {
        m_pSample = (PSAMPLE)new BYTE[8+4096*1024];
        MSDK_CHECK_POINTER(m_pSample, MFX_ERR_MEMORY_ALLOC);
    }

    return sts;
}

mfxStatus CEncodingPipeline::Init( sParams *pParams )
{
    MSDK_CHECK_POINTER( pParams, MFX_ERR_NULL_PTR );
    mfxStatus sts = MFX_ERR_NONE;

//    char pFileName[20];
//    sprintf( pFileName,"cctv%d.wav", pParams->ndeviceid );
//    fpAudio = fopen(pFileName, "wa+");
//    sprintf( pFileName,"cctv%d.264", pParams->ndeviceid );
//    fpVideo = fopen(pFileName, "wa+");

    m_pVd = pParams->vd;
    m_MVCflags = pParams->MVC_flags;

    // prepare save buffer
    sts = InitSaveBuffer( pParams->nDstWidth, pParams->nDstHeight );
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // we set version to 1.0 and later we will query actual version of the library which will got leaded
    mfxVersion min_version;
    min_version.Minor = 13;
    min_version.Major = 1;
    mfxIMPL impl = MFX_IMPL_HARDWARE_ANY;
    sts = m_mfxSession.Init( impl, &min_version );
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // create and init frame allocator
    sts = CreateAllocator();
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    sts = InitMfxEncParams(pParams);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    sts = InitMfxVppParams(pParams);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // MVC specific options
    if (MVC_ENABLED & m_MVCflags)
    {
        sts = AllocAndInitMVCSeqDesc();
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
    }

    // create encoder
    m_pMfxENC = new MFXVideoENCODE(m_mfxSession);
    MSDK_CHECK_POINTER(m_pMfxENC, MFX_ERR_MEMORY_ALLOC);

    // create preprocessor if resizing was requested from command line
    // or if different FourCC is set in InitMfxVppParams
    if (pParams->nWidth  != pParams->nDstWidth ||
            pParams->nHeight != pParams->nDstHeight ||
            m_mfxVppParams.vpp.In.FourCC != m_mfxVppParams.vpp.Out.FourCC)
    {
        m_pMfxVPP = new MFXVideoVPP(m_mfxSession);
        MSDK_CHECK_POINTER(m_pMfxVPP, MFX_ERR_MEMORY_ALLOC);
    }

    sts = ResetMFXComponents(pParams);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    return MFX_ERR_NONE;
}

void CEncodingPipeline::Close()
{
    MSDK_SAFE_DELETE(m_pMfxENC);
    MSDK_SAFE_DELETE(m_pMfxVPP);

    FreeMVCSeqDesc();
    FreeVppDoNotUse();

    DeleteFrames();

    m_pPlugin.reset();

    m_TaskPool.Close();
    m_mfxSession.Close();

    if (m_pLoopListBuffer)
        MSDK_SAFE_DELETE(m_pLoopListBuffer);

    // allocator if used as external for MediaSDK must be deleted after SDK components
    DeleteAllocator();
}

mfxStatus CEncodingPipeline::ResetMFXComponents(sParams* pParams)
{
    MSDK_CHECK_POINTER(pParams, MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(m_pMfxENC, MFX_ERR_NOT_INITIALIZED);

    mfxStatus sts = MFX_ERR_NONE;

    sts = m_pMfxENC->Close();
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_NOT_INITIALIZED);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    if (m_pMfxVPP)
    {
        sts = m_pMfxVPP->Close();
        MSDK_IGNORE_MFX_STS(sts, MFX_ERR_NOT_INITIALIZED);
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
    }

    // free allocated frames
    DeleteFrames();

    m_TaskPool.Close();

    sts = AllocFrames();
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
    // wcl 2015.03.17 add
    if(m_pMfxENC == NULL)
        printf("ERR m_pMfxENC = NULL\n");
    sts = m_pMfxENC->Init( &m_MfxEncParams );
    if (MFX_WRN_PARTIAL_ACCELERATION == sts)
    {
        msdk_printf(MSDK_STRING("WARNING: partial acceleration\n"));
        MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    }
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    mfxVideoParam par;
    MSDK_ZERO_MEMORY( par );
    m_pMfxENC->GetVideoParam( &par );
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    m_pMfxENC->Close();
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    m_MfxEncParams.mfx.BufferSizeInKB = par.mfx.BufferSizeInKB;
    if( !m_bSelfCaluBuffsizeInKB )
    {
        if( m_MfxEncParams.mfx.FrameInfo.Width == 1280 )
            m_MfxEncParams.mfx.BufferSizeInKB = m_MfxEncParams.mfx.BufferSizeInKB/10;
        else if( m_MfxEncParams.mfx.FrameInfo.Width == 1120)
            m_MfxEncParams.mfx.BufferSizeInKB = m_MfxEncParams.mfx.BufferSizeInKB/8;
        else if( m_MfxEncParams.mfx.FrameInfo.Width == 960)
            m_MfxEncParams.mfx.BufferSizeInKB = m_MfxEncParams.mfx.BufferSizeInKB/8;
        else if( m_MfxEncParams.mfx.FrameInfo.Width <= 800)
            m_MfxEncParams.mfx.BufferSizeInKB = m_MfxEncParams.mfx.BufferSizeInKB/6;
//        else if( m_MfxEncParams.mfx.FrameInfo.Width == 480)
//            m_MfxEncParams.mfx.BufferSizeInKB = m_MfxEncParams.mfx.BufferSizeInKB/6;
        m_bSelfCaluBuffsizeInKB = true;
    }
    // wcl add
    sts = m_pMfxENC->Init(&m_MfxEncParams);
    if (MFX_WRN_PARTIAL_ACCELERATION == sts)
    {
        msdk_printf(MSDK_STRING("WARNING: partial acceleration\n"));
        MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    }

    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    if (m_pMfxVPP)
    {
        sts = m_pMfxVPP->Init(&m_mfxVppParams);
        if (MFX_WRN_PARTIAL_ACCELERATION == sts)
        {
            msdk_printf(MSDK_STRING("WARNING: partial acceleration\n"));
            MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
        }
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
    }

    mfxU32 nEncodedDataBufferSize = m_MfxEncParams.mfx.FrameInfo.Width * m_MfxEncParams.mfx.FrameInfo.Height * 4;
    sts = m_TaskPool.Init( &m_mfxSession, m_pLoopListBuffer, m_MfxEncParams.AsyncDepth, nEncodedDataBufferSize, m_pSample );
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    return MFX_ERR_NONE;
}

mfxStatus CEncodingPipeline::AllocateSufficientBuffer(mfxBitstream* pBS)
{
    MSDK_CHECK_POINTER(pBS, MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(m_pMfxENC, MFX_ERR_NOT_INITIALIZED);

    mfxVideoParam par;
    MSDK_ZERO_MEMORY(par);

    // find out the required buffer size
    mfxStatus sts = m_pMfxENC->GetVideoParam(&par);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // reallocate bigger buffer for output
    sts = ExtendMfxBitstream(pBS, par.mfx.BufferSizeInKB * 1000);
    MSDK_CHECK_RESULT_SAFE(sts, MFX_ERR_NONE, sts, WipeMfxBitstream(pBS));

    return MFX_ERR_NONE;
}

mfxStatus CEncodingPipeline::GetFreeTask(sTask **ppTask)
{
    mfxStatus sts = MFX_ERR_NONE;

    sts = m_TaskPool.GetFreeTask(ppTask);
    if (MFX_ERR_NOT_FOUND == sts)
    {
        sts = m_TaskPool.SynchronizeFirstTask();
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

        // try again
        sts = m_TaskPool.GetFreeTask(ppTask);
    }

    return sts;
}

mfxStatus CEncodingPipeline::LoadFrameFromBuffer(mfxFrameSurface1* pSurface,  unsigned long *plTimeStamp)
{
    MSDK_CHECK_POINTER(pSurface, MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(m_pVd, MFX_ERR_NULL_PTR);

    mfxU16 w, h, pitch;
    mfxU8 *ptr;
    mfxFrameInfo& pInfo = pSurface->Info;
    mfxFrameData& pData = pSurface->Data;
    unsigned char * YFrameBuf;
    YFrameBuf = (uint8_t*)av_mallocz(sizeof(uint8_t)*720*576*3/2);

    // this reader supports only NV12 mfx surfaces for code transparency,
    // other formats may be added if application requires such functionality
    if ( MFX_FOURCC_NV12 != pInfo.FourCC && MFX_FOURCC_YV12 != pInfo.FourCC )
        return MFX_ERR_UNSUPPORTED;

    if ( pInfo.CropH > 0 && pInfo.CropW > 0 )
    {
        w = pInfo.CropW;
        h = pInfo.CropH;
    }
    else
    {
        w = pInfo.Width;
        h = pInfo.Height;
    }

    pitch = pData.Pitch;
    ptr = pData.Y + pInfo.CropX + pInfo.CropY * pData.Pitch;

    int YLength = 0;

    if( !m_pVd->GetFrame( YFrameBuf, YLength, plTimeStamp, 0 ))
    {
        return MFX_TASK_BUSY;
    }
//    fwrite( YFrameBuf, h*w*3/2, 1, fp_yuv);

    mfxU16 i=0;
    mfxU32 j=0;
    for(i = 0; i < h; i++)
    {
        memcpy( ptr + i*pitch, YFrameBuf+w*i, w );
    }

    ptr = pData.UV + pInfo.CropX + (pInfo.CropY / 2) * pitch;
    for( i=0; i<h/2; i++)
    {
        memcpy( ptr + i*pitch, YFrameBuf + w*h + w*i, w);
    }

//    mfxU8 buf[1024];
//    ptr = pData.UV + pInfo.CropX + (pInfo.CropY / 2) * pitch;
//    for( i=0; i<h/2; i++)
//    {
//        memcpy(buf, YFrameBuf + w*h + w/2*i, w/2);
//        for (j = 0; j < w/2; j++)
//        {
//            ptr[i * pitch + j * 2] = buf[j];
//        }
//    }
//    for( i=0; i<h/2; i++)
//    {
//        memcpy(buf, YFrameBuf + w*h*5/4 + w/2*i, w/2);
//        for (j = 0; j < w/2; j++)
//        {
//            ptr[i * pitch + j * 2 + 1] = buf[j];
//        }
//    }

    av_free(YFrameBuf);

    return MFX_ERR_NONE;
}

mfxU16 CEncodingPipeline::GetFreeSurface(mfxFrameSurface1* pSurfacesPool, mfxU16 nPoolSize)
{
    mfxU32 SleepInterval = 10; // milliseconds
    mfxU16 idx = MSDK_INVALID_SURF_IDX;

    //wait if there's no free surface
    for (mfxU32 i = 0; i < MSDK_WAIT_INTERVAL; i += SleepInterval)
    {
        idx = GetFreeSurfaceIndex(pSurfacesPool, nPoolSize);
        if (MSDK_INVALID_SURF_IDX != idx)
            break;
        else
            MSDK_SLEEP(SleepInterval);
    }

    return idx;
}

mfxU16 CEncodingPipeline::GetFreeSurfaceIndex(mfxFrameSurface1* pSurfacesPool, mfxU16 nPoolSize)
{
    if (pSurfacesPool)
    {
        for (mfxU16 i = 0; i < nPoolSize; i++)
        {
            if (0 == pSurfacesPool[i].Data.Locked)
                return i;
        }
    }

    return MSDK_INVALID_SURF_IDX;
}

mfxStatus CEncodingPipeline::ExtendMfxBitstream(mfxBitstream* pBitstream, mfxU32 nSize)
{
    MSDK_CHECK_POINTER(pBitstream, MFX_ERR_NULL_PTR);

    MSDK_CHECK_ERROR(nSize <= pBitstream->MaxLength, true, MFX_ERR_UNSUPPORTED);

    mfxU8* pData = new mfxU8[nSize];
    MSDK_CHECK_POINTER(pData, MFX_ERR_MEMORY_ALLOC);

    memmove(pData, pBitstream->Data + pBitstream->DataOffset, pBitstream->DataLength);

    WipeMfxBitstream(pBitstream);

    pBitstream->Data       = pData;
    pBitstream->DataOffset = 0;
    pBitstream->MaxLength  = nSize;

    return MFX_ERR_NONE;
}

mfxStatus CEncodingPipeline::ConvertFrameRate(mfxF64 dFrameRate, mfxU32* pnFrameRateExtN, mfxU32* pnFrameRateExtD)
{
    MSDK_CHECK_POINTER(pnFrameRateExtN, MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(pnFrameRateExtD, MFX_ERR_NULL_PTR);

    mfxU32 fr;

    fr = (mfxU32)(dFrameRate + .5);

    if (fabs(fr - dFrameRate) < 0.0001)
    {
        *pnFrameRateExtN = fr;
        *pnFrameRateExtD = 1;
        return MFX_ERR_NONE;
    }

    fr = (mfxU32)(dFrameRate * 1.001 + .5);

    if (fabs(fr * 1000 - dFrameRate * 1001) < 10)
    {
        *pnFrameRateExtN = fr * 1000;
        *pnFrameRateExtD = 1001;
        return MFX_ERR_NONE;
    }

    *pnFrameRateExtN = (mfxU32)(dFrameRate * 10000 + .5);
    *pnFrameRateExtD = 10000;

    return MFX_ERR_NONE;
}

mfxU32 CEncodingPipeline::GetNumber(mfxSession session)
{
    mfxU32 adapterNum = 0; // default
    mfxIMPL impl = MFX_IMPL_SOFTWARE; // default in case no HW IMPL is found

    // we don't care for error codes in further code; if something goes wrong we fall back to the default adapter
    if (session)
    {
        MFXQueryIMPL(session, &impl);
    }
    else
    {
        // an auxiliary session, internal for this function
        mfxSession auxSession;
        memset(&auxSession, 0, sizeof(auxSession));

        mfxVersion ver = { {1, 1 }}; // minimum API version which supports multiple devices
        MFXInit(MFX_IMPL_HARDWARE_ANY, &ver, &auxSession);
        MFXQueryIMPL(auxSession, &impl);
        MFXClose(auxSession);
    }

    // extract the base implementation type
    mfxIMPL baseImpl = MFX_IMPL_BASETYPE(impl);

    const struct
    {
        // actual implementation
        mfxIMPL impl;
        // adapter's number
        mfxU32 adapterID;

    } implTypes[] = {
        {MFX_IMPL_HARDWARE, 0},
        {MFX_IMPL_SOFTWARE, 0},
        {MFX_IMPL_HARDWARE2, 1},
        {MFX_IMPL_HARDWARE3, 2},
        {MFX_IMPL_HARDWARE4, 3}
    };


    // get corresponding adapter number
    for (mfxU8 i = 0; i < sizeof(implTypes)/sizeof(*implTypes); i++)
    {
        if (implTypes[i].impl == baseImpl)
        {
            adapterNum = implTypes[i].adapterID;
            break;
        }
    }
    return adapterNum;
}

mfxStatus CEncodingPipeline::Run()
{
    m_EncodeCtrl.QP = 25;
    MSDK_CHECK_POINTER(m_pMfxENC, MFX_ERR_NOT_INITIALIZED);

    mfxStatus sts = MFX_ERR_NONE;

    mfxFrameSurface1* pSurf = NULL; // dispatching pointer

    sTask *pCurrentTask = NULL; // a pointer to the current task
    mfxU16 nEncSurfIdx = 0;     // index of free surface for encoder input (vpp output)
    mfxU16 nVppSurfIdx = 0;     // index of free surface for vpp input

    mfxSyncPoint VppSyncPoint = NULL; // a sync point associated with an asynchronous vpp call
    bool bVppMultipleOutput = false;  // this flag is true if VPP produces more frames at output
    // than consumes at input. E.g. framerate conversion 30 fps -> 60 fps


    // Since in sample we support just 2 views
    // we will change this value between 0 and 1 in case of MVC
    mfxU16 currViewNum = 0;
    unsigned long  lTimeStamp = 0;

    sts = MFX_ERR_NONE;

    // main loop, preprocessing and encoding
    while (MFX_ERR_NONE <= sts || MFX_ERR_MORE_DATA == sts)
    {
        if( m_bExitApplication ) // quit
            return MFX_ERR_NONE;

        if( m_bStopEncoder ) // stop encoder
            return (mfxStatus)10000;

        // get a pointer to a free task (bit stream and sync point for encoder)
        sts = GetFreeTask(&pCurrentTask);
        MSDK_BREAK_ON_ERROR(sts);

        // find free surface for encoder input
        nEncSurfIdx = GetFreeSurface(m_pEncSurfaces, m_EncResponse.NumFrameActual);
        MSDK_CHECK_ERROR(nEncSurfIdx, MSDK_INVALID_SURF_IDX, MFX_ERR_MEMORY_ALLOC);

        // point pSurf to encoder surface
        pSurf = &m_pEncSurfaces[nEncSurfIdx];
        if (!bVppMultipleOutput)
        {
            // if vpp is enabled find free surface for vpp input and point pSurf to vpp surface
            if (m_pMfxVPP)
            {
                nVppSurfIdx = GetFreeSurface(m_pVppSurfaces, m_VppResponse.NumFrameActual);
                MSDK_CHECK_ERROR(nVppSurfIdx, MSDK_INVALID_SURF_IDX, MFX_ERR_MEMORY_ALLOC);
                pSurf = &m_pVppSurfaces[nVppSurfIdx];
            }

            // load frame from file to surface data
            // if we share allocator with Media SDK we need to call Lock to access surface data and...
            if (m_bExternalAlloc)
            {
                // get YUV pointers
                sts = m_pMFXAllocator->Lock(m_pMFXAllocator->pthis, pSurf->Data.MemId, &(pSurf->Data));
                MSDK_BREAK_ON_ERROR(sts);
            }

            pSurf->Info.FrameId.ViewId = currViewNum;
//            printf(" get frame form buffer \n");
            sts = LoadFrameFromBuffer( pSurf, &lTimeStamp );
//            printf("timestamp = %ld, length = %d \n",lTimeStamp, pSurf->Info.BufferSize);
            if( sts == MFX_TASK_BUSY )
                continue;
            MSDK_BREAK_ON_ERROR(sts);
            pSurf->Data.TimeStamp = lTimeStamp;
            if ( MVC_ENABLED & m_MVCflags )
                currViewNum ^= 1;
            // ... after we're done call Unlock
            if (m_bExternalAlloc)
            {
                sts = m_pMFXAllocator->Unlock(m_pMFXAllocator->pthis, pSurf->Data.MemId, &(pSurf->Data));
                MSDK_BREAK_ON_ERROR(sts);
            }
        }

        // perform preprocessing if required
        if (m_pMfxVPP)
        {
            bVppMultipleOutput = false; // reset the flag before a call to VPP
            for (;;)
            {
                sts = m_pMfxVPP->RunFrameVPPAsync(&m_pVppSurfaces[nVppSurfIdx], &m_pEncSurfaces[nEncSurfIdx],
                                                  NULL, &VppSyncPoint);

                if (MFX_ERR_NONE < sts && !VppSyncPoint) // repeat the call if warning and no output
                {
                    if (MFX_WRN_DEVICE_BUSY == sts)
                        MSDK_SLEEP(1); // wait if device is busy
                }
                else if (MFX_ERR_NONE < sts && VppSyncPoint)
                {
                    sts = MFX_ERR_NONE; // ignore warnings if output is available
                    break;
                }
                else
                    break; // not a warning
            }

            // process errors
            if (MFX_ERR_MORE_DATA == sts)
            {
                continue;
            }
            else if (MFX_ERR_MORE_SURFACE == sts)
            {
                bVppMultipleOutput = true;
            }
            else
            {
                MSDK_BREAK_ON_ERROR(sts);
            }
        }

        // save the id of preceding vpp task which will produce input data for the encode task
        if (VppSyncPoint)
        {
            pCurrentTask->DependentVppTasks.push_back(VppSyncPoint);
            VppSyncPoint = NULL;
        }

        for (;;)
        {
            // at this point surface for encoder contains either a frame from file or a frame processed by vpp
            sts = m_pMfxENC->EncodeFrameAsync(&m_EncodeCtrl, &m_pEncSurfaces[nEncSurfIdx], &pCurrentTask->mfxBS, &pCurrentTask->EncSyncP);
            // get next surface and new task for 2nd bitstream in ViewOutput mode
            MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_BITSTREAM);
            break;
        }
    }

    // means that the input file has ended, need to go to buffering loops
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);
    // exit in case of other errors
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    if (m_pMfxVPP)
    {
        // loop to get buffered frames from vpp
        while (MFX_ERR_NONE <= sts || MFX_ERR_MORE_DATA == sts || MFX_ERR_MORE_SURFACE == sts)
            // MFX_ERR_MORE_SURFACE can be returned only by RunFrameVPPAsync
            // MFX_ERR_MORE_DATA is accepted only from EncodeFrameAsync
        {
            // find free surface for encoder input (vpp output)
            nEncSurfIdx = GetFreeSurface(m_pEncSurfaces, m_EncResponse.NumFrameActual);
            MSDK_CHECK_ERROR(nEncSurfIdx, MSDK_INVALID_SURF_IDX, MFX_ERR_MEMORY_ALLOC);

            for (;;)
            {
                sts = m_pMfxVPP->RunFrameVPPAsync(NULL, &m_pEncSurfaces[nEncSurfIdx], NULL, &VppSyncPoint);

                if (MFX_ERR_NONE < sts && !VppSyncPoint) // repeat the call if warning and no output
                {
                    if (MFX_WRN_DEVICE_BUSY == sts)
                        MSDK_SLEEP(1); // wait if device is busy
                }
                else if (MFX_ERR_NONE < sts && VppSyncPoint)
                {
                    sts = MFX_ERR_NONE; // ignore warnings if output is available
                    break;
                }
                else
                    break; // not a warning
            }

            if (MFX_ERR_MORE_SURFACE == sts)
            {
                continue;
            }
            else
            {
                MSDK_BREAK_ON_ERROR(sts);
            }

            // get a free task (bit stream and sync point for encoder)
            sts = GetFreeTask(&pCurrentTask);
            MSDK_BREAK_ON_ERROR(sts);

            // save the id of preceding vpp task which will produce input data for the encode task
            if (VppSyncPoint)
            {
                pCurrentTask->DependentVppTasks.push_back(VppSyncPoint);
                VppSyncPoint = NULL;
            }

            for (;;)
            {
                sts = m_pMfxENC->EncodeFrameAsync(&m_EncodeCtrl, &m_pEncSurfaces[nEncSurfIdx], &pCurrentTask->mfxBS, &pCurrentTask->EncSyncP);

                if (MFX_ERR_NONE < sts && !pCurrentTask->EncSyncP) // repeat the call if warning and no output
                {
                    if (MFX_WRN_DEVICE_BUSY == sts)
                        MSDK_SLEEP(1); // wait if device is busy
                }
                else if (MFX_ERR_NONE < sts && pCurrentTask->EncSyncP)
                {
                    sts = MFX_ERR_NONE; // ignore warnings if output is available
                    break;
                }
                else if (MFX_ERR_NOT_ENOUGH_BUFFER == sts)
                {
                    sts = AllocateSufficientBuffer(&pCurrentTask->mfxBS);
                    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
                }
                else
                {
                    // get next surface and new task for 2nd bitstream in ViewOutput mode
                    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_BITSTREAM);
                    break;
                }
            }
        }

        // MFX_ERR_MORE_DATA is the correct status to exit buffering loop with
        // indicates that there are no more buffered frames
        MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);
        // exit in case of other errors
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
    }

    // loop to get buffered frames from encoder
    while (MFX_ERR_NONE <= sts)
    {
        // get a free task (bit stream and sync point for encoder)
        sts = GetFreeTask(&pCurrentTask);
        MSDK_BREAK_ON_ERROR(sts);

        for (;;)
        {
            sts = m_pMfxENC->EncodeFrameAsync(&m_EncodeCtrl, NULL, &pCurrentTask->mfxBS, &pCurrentTask->EncSyncP);

            if (MFX_ERR_NONE < sts && !pCurrentTask->EncSyncP) // repeat the call if warning and no output
            {
                if (MFX_WRN_DEVICE_BUSY == sts)
                    MSDK_SLEEP(1); // wait if device is busy
            }
            else if (MFX_ERR_NONE < sts && pCurrentTask->EncSyncP)
            {
                sts = MFX_ERR_NONE; // ignore warnings if output is available
                break;
            }
            else if (MFX_ERR_NOT_ENOUGH_BUFFER == sts)
            {
                sts = AllocateSufficientBuffer(&pCurrentTask->mfxBS);
                MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
            }
            else
            {
                // get new task for 2nd bitstream in ViewOutput mode
                MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_BITSTREAM);
                break;
            }
        }
        MSDK_BREAK_ON_ERROR(sts);
    }

    // MFX_ERR_MORE_DATA is the correct status to exit buffering loop with
    // indicates that there are no more buffered frames
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);
    // exit in case of other errors
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // synchronize all tasks that are left in task pool
    while (MFX_ERR_NONE == sts)
    {
        sts = m_TaskPool.SynchronizeFirstTask();
    }

    // MFX_ERR_NOT_FOUND is the correct status to exit the loop with
    // EncodeFrameAsync and SyncOperation don't return this status
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_NOT_FOUND);
    // report any errors that occurred in asynchronous part
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    return sts;
}
