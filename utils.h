#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <sstream>

#include "time_defs.h"

#include "sysmem_allocator.h"


typedef char msdk_char;
typedef std::basic_string<msdk_char> msdk_string;
typedef std::basic_stringstream<msdk_char> msdk_stringstream;

//   vm/file_defs.h
#define MSDK_FOPEN(file, name, mode) file = fopen(name, mode)

#define msdk_fgets  fgets

// vm/strings_defs.h
#define msdk_printf   printf
#define msdk_sprintf  sprintf
#define msdk_vprintf  vprintf
#define msdk_fprintf  fprintf
#define msdk_strlen   strlen
#define msdk_strcmp   strcmp
#define msdk_strncmp  strncmp
#define msdk_strstr   strstr
#define msdk_atoi     atoi
#define msdk_atoll    atoll
#define msdk_strtol   strtol
#define msdk_strtod   strtod
#define msdk_itoa_decimal(value, str) \
  snprintf(str, sizeof(str)/sizeof(str[0])-1, "%d", value)

#define msdk_strcopy strcpy

#define msdk_strncopy_s(dst, num_dst, src, count) strncpy(dst, src, count)

#define MSDK_MEMCPY_BITSTREAM(bitstream, offset, src, count) memcpy((bitstream).Data + (offset), (src), (count))

#define MSDK_MEMCPY_BUF(bufptr, offset, maxsize, src, count) memcpy((bufptr)+ (offset), (src), (count))

#define MSDK_MEMCPY_VAR(dstVarName, src, count) memcpy(&(dstVarName), (src), (count))

#define MSDK_MEMCPY(dst, src, count) memcpy(dst, (src), (count))

enum MsdkTraceLevel {
    MSDK_TRACE_LEVEL_SILENT = -1,
    MSDK_TRACE_LEVEL_CRITICAL = 0,
    MSDK_TRACE_LEVEL_ERROR = 1,
    MSDK_TRACE_LEVEL_WARNING = 2,
    MSDK_TRACE_LEVEL_INFO = 3,
    MSDK_TRACE_LEVEL_DEBUG = 4,
};

enum msdkAPIFeature {
    MSDK_FEATURE_NONE,
    MSDK_FEATURE_MVC,
    MSDK_FEATURE_JPEG_DECODE,
    MSDK_FEATURE_LOW_LATENCY,
    MSDK_FEATURE_MVC_VIEWOUTPUT,
    MSDK_FEATURE_JPEG_ENCODE,
    MSDK_FEATURE_LOOK_AHEAD,
    MSDK_FEATURE_PLUGIN_API
};

class CAutoTimer
{
public:
    CAutoTimer(msdk_tick& _elapsed):
        elapsed(_elapsed),
        start(0)
    {
        elapsed = _elapsed;
        start = msdk_time_get_tick();
    }
    ~CAutoTimer()
    {
        elapsed += msdk_time_get_tick() - start;
    }
    msdk_tick Sync()
    {
        msdk_tick cur = msdk_time_get_tick();
        elapsed += cur - start;
        start = cur;
        return elapsed;
    }
protected:
    msdk_tick& elapsed;
    msdk_tick start;
private:
    CAutoTimer(const CAutoTimer&);
    void operator=(const CAutoTimer&);
};

class CTimer
{
public:
    CTimer():
        start(0)
    {
    }
    static msdk_tick GetFrequency()
    {
        if (!frequency) frequency = msdk_time_get_frequency();
        return frequency;
    }
    static mfxF64 ConvertToSeconds(msdk_tick elapsed)
    {
        return MSDK_GET_TIME(elapsed, 0, GetFrequency());
    }

    inline void Start()
    {
        start = msdk_time_get_tick();
    }
    inline msdk_tick GetDelta()
    {
        return msdk_time_get_tick() - start;
    }
    inline mfxF64 GetTime()
    {
        return MSDK_GET_TIME(msdk_time_get_tick(), start, GetFrequency());
    }

protected:
    static msdk_tick frequency;
    msdk_tick start;
private:
    CTimer(const CTimer&);
    void operator=(const CTimer&);
};

// function for getting a pointer to a specific external buffer from the array
mfxExtBuffer* GetExtBuffer(mfxExtBuffer** ebuffers, mfxU32 nbuffers, mfxU32 BufferId);

//declare used extended buffers
template<class T>
struct mfx_ext_buffer_id{
    enum {id = 0};
};
template<>struct mfx_ext_buffer_id<mfxExtCodingOption>{
    enum {id = MFX_EXTBUFF_CODING_OPTION};
};
template<>struct mfx_ext_buffer_id<mfxExtCodingOption2>{
    enum {id = MFX_EXTBUFF_CODING_OPTION2};
};
template<>struct mfx_ext_buffer_id<mfxExtAvcTemporalLayers>{
    enum {id = MFX_EXTBUFF_AVC_TEMPORAL_LAYERS};
};
template<>struct mfx_ext_buffer_id<mfxExtAVCRefListCtrl>{
    enum {id = MFX_EXTBUFF_AVC_REFLIST_CTRL};
};
template<>struct mfx_ext_buffer_id<mfxExtThreadsParam>{
    enum {id = MFX_EXTBUFF_THREADS_PARAM};
};

//helper function to initialize mfx ext buffer structure
template <class T>
void init_ext_buffer(T & ext_buffer)
{
    memset(&ext_buffer, 0, sizeof(ext_buffer));
    reinterpret_cast<mfxExtBuffer*>(&ext_buffer)->BufferId = mfx_ext_buffer_id<T>::id;
    reinterpret_cast<mfxExtBuffer*>(&ext_buffer)->BufferSz = sizeof(ext_buffer);
}

msdk_string NoFullPath(const msdk_string &);
int  msdk_trace_get_level();
void msdk_trace_set_level(int);
bool msdk_trace_is_printable(int);

#define msdk_err std::cerr
#define MSDK_DEC_WAIT_INTERVAL 300000
#define MSDK_ENC_WAIT_INTERVAL 300000
#define MSDK_VPP_WAIT_INTERVAL 300000
#define MSDK_WAIT_INTERVAL MSDK_DEC_WAIT_INTERVAL+3*MSDK_VPP_WAIT_INTERVAL+MSDK_ENC_WAIT_INTERVAL // an estimate for the longest pipeline we have in samples

#define MSDK_INVALID_SURF_IDX 0xFFFF

#define MSDK_MAX_FILENAME_LEN 1024

#define MSDK_PRINT_RET_MSG(ERR) {msdk_printf(MSDK_STRING("\nReturn on error: error code %d,\t%s\t%d\n\n"), (int)ERR, MSDK_STRING(__FILE__), __LINE__);}

#define MSDK_TRACE_LEVEL(level, ERR) if (level <= msdk_trace_get_level()) {msdk_err<<NoFullPath(MSDK_STRING(__FILE__)) << MSDK_STRING(" :")<< __LINE__ <<MSDK_STRING(" [") \
    <<level<<MSDK_STRING("] ") << ERR << std::endl;}

#define MSDK_TRACE_CRITICAL(ERR) MSDK_TRACE_LEVEL(MSDK_TRACE_LEVEL_CRITICAL, ERR)
#define MSDK_TRACE_ERROR(ERR) MSDK_TRACE_LEVEL(MSDK_TRACE_LEVEL_ERROR, ERR)
#define MSDK_TRACE_WARNING(ERR) MSDK_TRACE_LEVEL(MSDK_TRACE_LEVEL_WARNING, ERR)
#define MSDK_TRACE_INFO(ERR) MSDK_TRACE_LEVEL(MSDK_TRACE_LEVEL_INFO, ERR)
#define MSDK_TRACE_DEBUG(ERR) MSDK_TRACE_LEVEL(MSDK_TRACE_LEVEL_DEBUG, ERR)

#define MSDK_CHECK_ERROR(P, X, ERR)              {if ((X) == (P)) {MSDK_PRINT_RET_MSG(ERR); return ERR;}}
#define MSDK_CHECK_NOT_EQUAL(P, X, ERR)          {if ((X) != (P)) {MSDK_PRINT_RET_MSG(ERR); return ERR;}}
#define MSDK_CHECK_RESULT(P, X, ERR)             {if ((X) > (P)) {MSDK_PRINT_RET_MSG(ERR); return ERR;}}
#define MSDK_CHECK_PARSE_RESULT(P, X, ERR)       {if ((X) > (P)) {return ERR;}}
#define MSDK_CHECK_RESULT_SAFE(P, X, ERR, ADD)   {if ((X) > (P)) {ADD; MSDK_PRINT_RET_MSG(ERR); return ERR;}}
#define MSDK_IGNORE_MFX_STS(P, X)                {if ((X) == (P)) {P = MFX_ERR_NONE;}}
#define MSDK_CHECK_POINTER(P, ...)               {if (!(P)) {return __VA_ARGS__;}}
#define MSDK_CHECK_POINTER_NO_RET(P)             {if (!(P)) {return;}}
#define MSDK_CHECK_POINTER_SAFE(P, ERR, ADD)     {if (!(P)) {ADD; return ERR;}}
#define MSDK_BREAK_ON_ERROR(P)                   {if (MFX_ERR_NONE != (P)) break;}
#define MSDK_SAFE_DELETE_ARRAY(P)                {if (P) {delete[] P; P = NULL;}}
#define MSDK_SAFE_RELEASE(X)                     {if (X) { X->Release(); X = NULL; }}
#define MSDK_SAFE_FREE(X)                        {if (X) { free(X); X = NULL; }}

#ifndef MSDK_SAFE_DELETE
#define MSDK_SAFE_DELETE(P)                      {if (P) {delete P; P = NULL;}}
#endif // MSDK_SAFE_DELETE

#define MSDK_ZERO_MEMORY(VAR)                    {memset(&VAR, 0, sizeof(VAR));}
#define MSDK_MAX(A, B)                           (((A) > (B)) ? (A) : (B))
#define MSDK_MIN(A, B)                           (((A) < (B)) ? (A) : (B))
#define MSDK_ALIGN16(value)                      (((value + 15) >> 4) << 4) // round up to a multiple of 16
#define MSDK_ALIGN32(value)                      (((value + 31) >> 5) << 5) // round up to a multiple of 32
#define MSDK_ALIGN(value, alignment)             (alignment) * ( (value) / (alignment) + (((value) % (alignment)) ? 1 : 0))
#define MSDK_ARRAY_LEN(value)                    (sizeof(value) / sizeof(value[0]))

#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(par) (par)
#endif

#ifndef MFX_PRODUCT_VERSION
#define MFX_PRODUCT_VERSION "5.0.1604370.70"
#endif

#define MSDK_SAMPLE_VERSION MSDK_STRING(MFX_PRODUCT_VERSION)

#define MFX_IMPL_VIA_MASK(x) (0x0f00 & (x))

#define MSDK_SLEEP(msec) usleep(1000*msec)
#define MSDK_STRING(x)  x
#define MSDK_CHAR(x) x

#endif // UTILS_H
