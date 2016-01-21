#include "utils.h"

msdk_tick CTimer::frequency = 0;

msdk_string NoFullPath(const msdk_string & file_path) {
    size_t pos = file_path.find_last_of(MSDK_STRING("\\/"));
    if (pos != msdk_string::npos) {
        return file_path.substr(pos + 1);
    }
    return file_path;
}

namespace {
int g_trace_level = MSDK_TRACE_LEVEL_INFO;
}

int msdk_trace_get_level() {
    return g_trace_level;
}

void msdk_trace_set_level(int newLevel) {
    g_trace_level = newLevel;
}

bool msdk_trace_is_printable(int level) {
    return g_trace_level >= level;
}

// function for getting a pointer to a specific external buffer from the array
mfxExtBuffer* GetExtBuffer(mfxExtBuffer** ebuffers, mfxU32 nbuffers, mfxU32 BufferId)
{
    if (!ebuffers) return 0;
    for(mfxU32 i=0; i<nbuffers; i++) {
        if (!ebuffers[i]) continue;
        if (ebuffers[i]->BufferId == BufferId) {
            return ebuffers[i];
        }
    }
    return 0;
}
