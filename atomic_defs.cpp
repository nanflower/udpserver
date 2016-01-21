#include "atomic_defs.h"

static mfxU16 msdk_atomic_add16(volatile mfxU16 *mem, mfxU16 val)
{
    asm volatile ("lock; xaddw %0,%1"
                  : "=r" (val), "=m" (*mem)
                  : "0" (val), "m" (*mem)
                  : "memory", "cc");
    return val;
}

mfxU16 msdk_atomic_inc16(volatile mfxU16 *pVariable)
{
    return msdk_atomic_add16(pVariable, 1) + 1;
}

/* Thread-safe 16-bit variable decrementing */
mfxU16 msdk_atomic_dec16(volatile mfxU16 *pVariable)
{
    return msdk_atomic_add16(pVariable, (mfxU16)-1) + 1;
}
