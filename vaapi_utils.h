/* ****************************************************************************** *\

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2011-2013 Intel Corporation. All Rights Reserved.

\* ****************************************************************************** */

#ifndef __VAAPI_UTILS_H__
#define __VAAPI_UTILS_H__

#include <va/va.h>
#include <utils.h>

class CLibVA
{
public:
    virtual ~CLibVA(void) {}
    VADisplay GetVADisplay(void) { return m_va_dpy; }

protected:
    CLibVA(void) :
        m_va_dpy(NULL)
    {}
    VADisplay m_va_dpy;
};

CLibVA* CreateLibVA(void);

mfxStatus va_to_mfx_status(VAStatus va_res);



#endif // #ifndef __VAAPI_UTILS_H__
