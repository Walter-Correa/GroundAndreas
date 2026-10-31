#pragma once

#include <cstdint>
#include <cstddef>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>
#include <iostream>

#include "../lib/sdk/amx/amx.h"
#include "../lib/sdk/amx/amx2.h"
#include "../lib/sdk/plugincommon.h"

#if defined(_WIN32) || defined(WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#else
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <unistd.h>
#endif

// Log printf function pointer definition
typedef void (*logprintf_t)(const char* format, ...);
extern logprintf_t logprintf;

extern void* pAMXFunctions;

inline cell FloatToCell(float f)
{
    cell c = 0;
    std::memcpy(&c, &f, sizeof(float));
    return c;
}

inline float CellToFloat(cell c)
{
    float f = 0.0f;
    std::memcpy(&f, &c, sizeof(float));
    return f;
}
