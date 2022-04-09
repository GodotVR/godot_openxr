/************************************************************************************
Filename    :   fb_keyboard_tracking.h
Content     :   Tracked Keyboards APIs
Language    :   C99
Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.
*************************************************************************************/
#pragma once

#include <openxr/openxr_extension_helpers.h>

/*
  117 XR_FB_keyboard_tracking
*/

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef XR_FB_keyboard_tracking

#if defined(XR_FB_keyboard_tracking_EXPERIMENTAL_VERSION)
#error \
    "XR_FB_keyboard_tracking_EXPERIMENTAL_VERSION is now deprecated, please migrate to the non-experimental version of this extension."
#endif

#define XR_FB_keyboard_tracking 1
#define XR_FB_keyboard_tracking_SPEC_VERSION 1
#define XR_FB_KEYBOARD_TRACKING_EXTENSION_NAME "XR_FB_keyboard_tracking"
#define XR_MAX_KEYBOARD_TRACKING_NAME_SIZE_FB 128

typedef XrFlags64 XrKeyboardTrackingFlagsFB;

// Flag bits for XrKeyboardTrackingFlagsFB
static const XrKeyboardTrackingFlagsFB XR_KEYBOARD_TRACKING_EXISTS_BIT_FB = 0x00000001;
static const XrKeyboardTrackingFlagsFB XR_KEYBOARD_TRACKING_LOCAL_BIT_FB = 0x00000002;
static const XrKeyboardTrackingFlagsFB XR_KEYBOARD_TRACKING_REMOTE_BIT_FB = 0x00000004;
static const XrKeyboardTrackingFlagsFB XR_KEYBOARD_TRACKING_CONNECTED_BIT_FB = 0x00000008;

typedef XrFlags64 XrKeyboardTrackingQueryFlagsFB;

// Flag bits for XrKeyboardTrackingQueryFlagsFB
static const XrKeyboardTrackingQueryFlagsFB XR_KEYBOARD_TRACKING_QUERY_LOCAL_BIT_FB = 0x00000002;
static const XrKeyboardTrackingQueryFlagsFB XR_KEYBOARD_TRACKING_QUERY_REMOTE_BIT_FB = 0x00000004;

// XrSystemKeyboardTrackingPropertiesFB extends XrSystemProperties
XR_STRUCT_ENUM(XR_TYPE_SYSTEM_KEYBOARD_TRACKING_PROPERTIES_FB, 1000116002);
typedef struct XrSystemKeyboardTrackingPropertiesFB {
    XrStructureType type;
    const void* XR_MAY_ALIAS next;
    XrBool32 supportsKeyboardTracking;
} XrSystemKeyboardTrackingPropertiesFB;

// Describes some intrinsic characteristics about a tracked keyboard,
// including name, size and tracking capabilities.
typedef struct XrKeyboardTrackingDescriptionFB {
    uint64_t trackedKeyboardId;
    XrVector3f size;
    XrKeyboardTrackingFlagsFB flags;
    char name[XR_MAX_KEYBOARD_TRACKING_NAME_SIZE_FB];
} XrKeyboardTrackingDescriptionFB;

// Describes a structure used to create a keyboard tracker of a specific type.
XR_STRUCT_ENUM(XR_TYPE_KEYBOARD_SPACE_CREATE_INFO_FB, 1000116009);
typedef struct XrKeyboardSpaceCreateInfoFB {
    XrStructureType type;
    void* XR_MAY_ALIAS next;
    uint64_t trackedKeyboardId;
} XrKeyboardSpaceCreateInfoFB;

// General input structure for system keyboard query
XR_STRUCT_ENUM(XR_TYPE_KEYBOARD_TRACKING_QUERY_FB, 1000116004);
typedef struct XrKeyboardTrackingQueryFB {
    XrStructureType type;
    void* XR_MAY_ALIAS next;
    XrKeyboardTrackingQueryFlagsFB flags;
} XrKeyboardTrackingQueryFB;

// Queries the system keyboard
typedef XrResult(XRAPI_PTR* PFN_xrQuerySystemTrackedKeyboardFB)(
    XrSession session,
    const XrKeyboardTrackingQueryFB* queryInfo,
    XrKeyboardTrackingDescriptionFB* keyboard);

// Creates a tracker identified by an XrSpace handle.
// This signals the system to begin looking for keyboard and it will
// continue to do so until the space is destroyed.
// The keyboard location is then computed via xrLocateSpace
typedef XrResult(XRAPI_PTR* PFN_xrCreateKeyboardSpaceFB)(
    XrSession session,
    const XrKeyboardSpaceCreateInfoFB* createInfo,
    XrSpace* keyboardSpace);

#ifndef XR_NO_PROTOTYPES
#ifdef XR_EXTENSION_PROTOTYPES

XRAPI_ATTR XrResult XRAPI_CALL xrQuerySystemTrackedKeyboardFB(
    XrSession session,
    const XrKeyboardTrackingQueryFB* queryInfo,
    XrKeyboardTrackingDescriptionFB* keyboard);

XRAPI_ATTR XrResult XRAPI_CALL xrCreateKeyboardSpaceFB(
    XrSession session,
    const XrKeyboardSpaceCreateInfoFB* createInfo,
    XrSpace* keyboardSpace);

#endif /* XR_EXTENSION_PROTOTYPES */
#endif /* !XR_NO_PROTOTYPES */

#endif // XR_FB_keyboard_tracking

#ifdef __cplusplus
}
#endif
