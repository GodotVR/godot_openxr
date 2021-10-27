/************************************************************************************
Filename    :   fb_hand_tracking_pointer.h
Content     :   Hand tracking collision capsule data.
Language    :   C99
Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.
*************************************************************************************/
#pragma once

/*
  112 XR_FB_hand_tracking_pointer
*/

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef XR_FB_hand_tracking_aim

/// latest structs enums
static const XrStructureType XR_TYPE_HAND_TRACKING_AIM_STATE_FB = (XrStructureType)1000111001;

#define XR_FB_hand_tracking_aim 1
#define XR_FB_hand_tracking_aim_SPEC_VERSION 1
#define XR_FB_HAND_TRACKING_AIM_EXTENSION_NAME "XR_FB_hand_tracking_aim"
typedef XrFlags64 XrHandTrackingAimFlagsFB;

// Flag bits for XrHandTrackingAimFlagsFB
static const XrHandTrackingAimFlagsFB XR_HAND_TRACKING_AIM_COMPUTED_BIT_FB = 0x00000001;
static const XrHandTrackingAimFlagsFB XR_HAND_TRACKING_AIM_VALID_BIT_FB = 0x00000002;
static const XrHandTrackingAimFlagsFB XR_HAND_TRACKING_AIM_INDEX_PINCHING_BIT_FB = 0x00000004;
static const XrHandTrackingAimFlagsFB XR_HAND_TRACKING_AIM_MIDDLE_PINCHING_BIT_FB = 0x00000008;
static const XrHandTrackingAimFlagsFB XR_HAND_TRACKING_AIM_RING_PINCHING_BIT_FB = 0x00000010;
static const XrHandTrackingAimFlagsFB XR_HAND_TRACKING_AIM_LITTLE_PINCHING_BIT_FB = 0x00000020;
static const XrHandTrackingAimFlagsFB XR_HAND_TRACKING_AIM_SYSTEM_GESTURE_BIT_FB = 0x00000040;
static const XrHandTrackingAimFlagsFB XR_HAND_TRACKING_AIM_DOMINANT_HAND_BIT_FB = 0x00000080;
static const XrHandTrackingAimFlagsFB XR_HAND_TRACKING_AIM_MENU_PRESSED_BIT_FB = 0x00000100;

// XrHandTrackingAimStateFB extends XrHandJointsLocateInfoEXT
typedef struct XrHandTrackingAimStateFB {
    XrStructureType type;
    void* XR_MAY_ALIAS next;
    XrHandTrackingAimFlagsFB status;
    XrPosef aimPose;
    float pinchStrengthIndex;
    float pinchStrengthMiddle;
    float pinchStrengthRing;
    float pinchStrengthLittle;
} XrHandTrackingAimStateFB;

#endif // XR_FB_hand_tracking_aim

#ifdef __cplusplus
}
#endif
