/************************************************************************************

Filename    :   fb_passthrough_keyboard_hands.h
Content     :   MR Passthrough Keyboard Hand Presence API definitions.
Language    :   C99

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

*************************************************************************************/

#pragma once

#include <openxr/openxr_extension_helpers.h>

#if defined(__cplusplus)
extern "C" {
#endif

// Extension 204
#ifndef XR_FB_passthrough_keyboard_hands

#define XR_FB_passthrough_keyboard_hands 1
#define XR_FB_passthrough_keyboard_hands_SPEC_VERSION 1
#define XR_FB_PASSTHROUGH_KEYBOARD_HANDS_EXTENSION_NAME "XR_FB_passthrough_keyboard_hands"

// Passthrough layer purpose for keyboard hand presence.
#if defined(XR_FB_passthrough)
XR_ENUM(
    XrPassthroughLayerPurposeFB,
    XR_PASSTHROUGH_LAYER_PURPOSE_TRACKED_KEYBOARD_HANDS_FB,
    1000203001);
#endif

XR_STRUCT_ENUM(XR_TYPE_PASSTHROUGH_KEYBOARD_HANDS_INTENSITY_FB, 1000203002);
typedef struct XrPassthroughKeyboardHandsIntensityFB {
    /// Structure header
    /// @{
    XrStructureType type;
    const void* XR_MAY_ALIAS next;
    /// @}

    // An intensity for left tracked hand.
    // An intensity value can be in the range [0.0, 1.0] where 0.0 is the lowest intensity.
    float leftHandIntensity;
    // An intensity for right tracked hand.
    // An intensity value can be in the range [0.0, 1.0] where 0.0 is the lowest intensity.
    float rightHandIntensity;
} XrPassthroughKeyboardHandsIntensityFB;

typedef XrResult(XRAPI_PTR* PFN_xrPassthroughLayerSetKeyboardHandsIntensityFB)(
    XrPassthroughLayerFB layer,
    const XrPassthroughKeyboardHandsIntensityFB* intensity);

#ifndef XR_NO_PROTOTYPES
#ifdef XR_EXTENSION_PROTOTYPES
// Render hands over the keyboard (keyboard hands) with a specific intensity of hands mask
// passthrough.
XRAPI_ATTR XrResult XRAPI_CALL xrPassthroughLayerSetKeyboardHandsIntensityFB(
    XrPassthroughLayerFB layer,
    const XrPassthroughKeyboardHandsIntensityFB* intensity);
#endif /* XR_EXTENSION_PROTOTYPES */
#endif /* !XR_NO_PROTOTYPES */

#endif // XR_FB_passthrough_keyboard_hands

#ifdef __cplusplus
}
#endif
