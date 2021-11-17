/************************************************************************************
Filename    :   fb_hand_tracking_mesh.h
Content     :   Hand tracking mesh driver.
Language    :   C99
Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.
*************************************************************************************/
#pragma once

/*
  111 XR_FB_hand_tracking_mesh
*/

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef XR_FB_hand_tracking_mesh

/// latest struct enums
static const XrStructureType XR_TYPE_HAND_TRACKING_MESH_FB = (XrStructureType)1000110001;
static const XrStructureType XR_TYPE_HAND_TRACKING_SCALE_FB = (XrStructureType)1000110003;

#define XR_FB_hand_tracking_mesh 1
#define XR_FB_hand_tracking_mesh_SPEC_VERSION 1
#define XR_FB_HAND_TRACKING_MESH_EXTENSION_NAME "XR_FB_hand_tracking_mesh"
typedef struct XrVector4sFB {
    int16_t x;
    int16_t y;
    int16_t z;
    int16_t w;
} XrVector4sFB;

typedef struct XrHandTrackingMeshFB {
    XrStructureType type;
    void* XR_MAY_ALIAS next;
    uint32_t jointCapacityInput;
    uint32_t jointCountOutput;
    XrPosef* jointBindPoses;
    float* jointRadii;
    XrHandJointEXT* jointParents;
    uint32_t vertexCapacityInput;
    uint32_t vertexCountOutput;
    XrVector3f* vertexPositions;
    XrVector3f* vertexNormals;
    XrVector2f* vertexUVs;
    XrVector4sFB* vertexBlendIndices;
    XrVector4f* vertexBlendWeights;
    uint32_t indexCapacityInput;
    uint32_t indexCountOutput;
    int16_t* indices;
} XrHandTrackingMeshFB;

// XrHandTrackingScaleFB extends XrHandJointsLocateInfoEXT
typedef struct XrHandTrackingScaleFB {
    XrStructureType type;
    void* XR_MAY_ALIAS next;
    float sensorOutput;
    float currentOutput;
    XrBool32 overrideHandScale;
    float overrideValueInput;
} XrHandTrackingScaleFB;

typedef XrResult(
    XRAPI_PTR* PFN_xrGetHandMeshFB)(XrHandTrackerEXT handTracker, XrHandTrackingMeshFB* mesh);

#ifndef XR_NO_PROTOTYPES
#ifdef XR_EXTENSION_PROTOTYPES
XRAPI_ATTR XrResult XRAPI_CALL
xrGetHandMeshFB(XrHandTrackerEXT handTracker, XrHandTrackingMeshFB* mesh);
#endif /* XR_EXTENSION_PROTOTYPES */
#endif /* !XR_NO_PROTOTYPES */

#endif // XR_FB_hand_tracking_mesh

#ifdef __cplusplus
}
#endif
