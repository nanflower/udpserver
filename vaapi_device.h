#include "hw_device.h"
#include "vaapi_utils_drm.h"

CHWDevice* CreateVAAPIDevice(void);

class CVAAPIDeviceDRM : public CHWDevice
{
public:
    CVAAPIDeviceDRM(){}
    virtual ~CVAAPIDeviceDRM(void) {}

    virtual mfxStatus Init(mfxHDL /*hWindow*/, mfxU16 /*nViews*/, mfxU32 /*nAdapterNum*/) { return MFX_ERR_NONE;}
    virtual mfxStatus Reset(void) { return MFX_ERR_NONE; }
    virtual void Close(void) { }

    virtual mfxStatus SetHandle(mfxHandleType /*type*/, mfxHDL /*hdl*/) { return MFX_ERR_UNSUPPORTED; }
    virtual mfxStatus GetHandle(mfxHandleType type, mfxHDL *pHdl)
    {
        if ((MFX_HANDLE_VA_DISPLAY == type) && (NULL != pHdl))
        {
            *pHdl = m_DRMLibVA.GetVADisplay();

            return MFX_ERR_NONE;
        }
        return MFX_ERR_UNSUPPORTED;
    }

    virtual mfxStatus RenderFrame(mfxFrameSurface1 * /*pSurface*/, mfxFrameAllocator * /*pmfxAlloc*/) { return MFX_ERR_NONE; }
    virtual void      UpdateTitle(double /*fps*/) { }

protected:
    DRMLibVA m_DRMLibVA;
};
