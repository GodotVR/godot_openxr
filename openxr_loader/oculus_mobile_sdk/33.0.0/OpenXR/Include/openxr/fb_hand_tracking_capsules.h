/************************************************************************************
Filename    :   fb_hand_tracking_capsules.h
Content     :   Hand tracking collision capsule data.
Language    :   C99
Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.
*************************************************************************************/
#pragma once

/*
  113 XR_FB_hand_tracking_capsules
*/

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef XR_FB_hand_tracking_capsules

/// define all types as consts here - will get to be moved to the main type enum once this is public
static const XrStructureType XR_TYPE_HAND_TRACKING_CAPSULES_STATE_FB = (XrStructureType)1000112000;

#define XR_FB_hand_tracking_capsules 1
#define XR_FB_hand_tracking_capsules_SPEC_VERSION 1
#define XR_FB_HAND_TRACKING_CAPSULES_EXTENSION_NAME "XR_FB_hand_tracking_capsules"
#define XR_FB_HAND_TRACKING_CAPSULE_POINT_COUNT 2
#define XR_FB_HAND_TRACKING_CAPSULE_COUNT 19
typedef struct XrHandCapsuleFB {
    XrVector3f points[XR_FB_HAND_TRACKING_CAPSULE_POINT_COUNT];
    float radius;
    XrHandJointEXT joint;
} XrHandCapsuleFB;

// XrHandTrackingCapsulesStateFB extends XrHandJointsLocateInfoEXT
typedef struct XrHandTrackingCapsulesStateFB {
    XrStructureType type;
    void* XR_MAY_ALIAS next;
    XrHandCapsuleFB capsules[XR_FB_HAND_TRACKING_CAPSULE_COUNT];
} XrHandTrackingCapsulesStateFB;

#endif // XR_FB_hand_tracking_capsules

#ifdef __cplusplus
}
#endif
