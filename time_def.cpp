
#include "time_defs.h"
#include <sys/time.h>

#define MSDK_TIME_MHZ 1000000

msdk_tick msdk_time_get_tick(void)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return (msdk_tick)tv.tv_sec * (msdk_tick)MSDK_TIME_MHZ + (msdk_tick)tv.tv_usec;
}

msdk_tick msdk_time_get_frequency(void)
{
    return (msdk_tick)MSDK_TIME_MHZ;
}
