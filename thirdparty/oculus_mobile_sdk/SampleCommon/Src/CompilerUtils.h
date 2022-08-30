// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/*******************************************************************************

Filename    :   CompilerUtils.h
Content     :   Macros and utilities for compile-time validation.
Created     :   March 20, 2018
Authors     :   Jonathan Wright
Language    :   C++

*******************************************************************************/

#pragma once

// included here with the intention of implementing a custom assert macro later.
#include <assert.h>

#ifndef OVR_UNUSED
#define OVR_UNUSED(a) (void)(a)
#endif
