#include "vaapi_device.h"

CHWDevice* CreateVAAPIDevice(void)
{
    return new CVAAPIDeviceDRM();
}
