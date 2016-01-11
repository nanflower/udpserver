/* ****************************************************************************** *\

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2012-2013 Intel Corporation. All Rights Reserved.

\* ****************************************************************************** */

#ifndef __VAAPI_UTILS_DRM_H__
#define __VAAPI_UTILS_DRM_H__

#include <va/va_drm.h>
#include "vaapi_utils.h"

class DRMLibVA : public CLibVA
{
public:
    DRMLibVA(void);
    virtual ~DRMLibVA(void);

protected:
    int m_fd;
};



#endif // #ifndef __VAAPI_UTILS_DRM_H__
