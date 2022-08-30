// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/*******************************************************************************

Filename	:   Log.c
Content		:	Functions for debug logging.
Created		:   February 21, 2018
Authors		:   J.M.P. van Waveren, Jonathan Wright
Language	:   C++

*******************************************************************************/

#include "Log.h"

#include <stdio.h>
#include <string.h>

#if defined(WIN32)
#include "windows.h"
#endif

#if defined(__cplusplus)
extern "C" {
#endif

static void StripPath(const char* filePath, char* strippedTag, size_t const strippedTagSize) {
    // scan backwards from the end to the first slash
    const int len = (int)(strlen(filePath));
    int slash;
    for (slash = len - 1; slash > 0 && filePath[slash] != '/' && filePath[slash] != '\\'; slash--) {
    }
    if (filePath[slash] == '/' || filePath[slash] == '\\') {
        slash++;
    }
    // copy forward until a dot or 0
    size_t i;
    for (i = 0; i < strippedTagSize - 1; i++) {
        const char c = filePath[slash + i];
        if (c == '.' || c == 0) {
            break;
        }
        strippedTag[i] = c;
    }
    strippedTag[i] = 0;
}

void LogWithFilenameTag(const int priority, const char* filename, const char* fmt, ...) {
    // we keep stack allocations to a minimum to keep logging side-effects to a minimum
    char tag[32];
    char msg[256 - sizeof(tag)];
    StripPath(filename, tag, sizeof(tag));

    // we use vsnprintf here rather than using __android_log_vprintf so we can control the size
    // and method of allocations as much as possible.
    va_list argPtr;
    va_start(argPtr, fmt);
    vsnprintf(msg, sizeof(msg), fmt, argPtr);
    va_end(argPtr);

#if defined(ANDROID)
    __android_log_write(priority, tag, msg);
#elif defined(WIN32)
    OutputDebugStringA("[");
    OutputDebugStringA(tag);
    switch (priority) {
        case SAMPLES_LOG_ERROR:
            OutputDebugStringA("] {ERROR} ");
            break;
        case SAMPLES_LOG_WARN:
            OutputDebugStringA("] {WARN} ");
            break;
        case SAMPLES_LOG_INFO:
            OutputDebugStringA("] {INFO} ");
            break;
        case SAMPLES_LOG_VERBOSE:
        default:
            OutputDebugStringA("] {VERBOSE} ");
            break;
    }
    OutputDebugStringA(msg);
    OutputDebugStringA("\n");
#else
    (void)priority;
    printf("[%s] %s\n", tag, msg);
#endif // defined(ANDROID)
}

#if defined(__cplusplus)
} // extern "C"
#endif
