// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/*******************************************************************************

Filename    :   System.cpp
Content     :	Global system functions.
Created     :   February 21, 2018
Authors     :   J.M.P. van Waveren, Jonathan Wright
Language    :   C++

*******************************************************************************/

#include "System.h"

#include <cstdio>
#include "time.h"
#include <string.h>

namespace OVRFW {

double GetTimeInSeconds() {
    struct timespec now;
#if !defined(WIN32)
    clock_gettime(CLOCK_MONOTONIC, &now);
#else
    timespec_get(&now, TIME_UTC);
#endif
    return (now.tv_sec * 1e9 + now.tv_nsec) * 0.000000001;
}

} // namespace OVRFW
