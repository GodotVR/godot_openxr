/************************************************************************************
Filename    :   fb_render_model.h
Content     :   Render Model APIs.
Language    :   C99
Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.
*************************************************************************************/
#pragma once

/*
  120 XR_FB_render_model
*/

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef XR_FB_render_model

#if defined(XR_FB_render_model_EXPERIMENTAL_VERSION)
#error \
    "XR_FB_render_model_EXPERIMENTAL_VERSION is now deprecated, please migrate to the non-experimental version of this extension."
#endif

#define XR_FB_render_model 1
#define XR_FB_render_model_SPEC_VERSION 1
#define XR_FB_RENDER_MODEL_EXTENSION_NAME "XR_FB_render_model"


#define XR_MAX_RENDER_MODEL_NAME_SIZE_FB 64
#define XR_NULL_RENDER_MODEL_KEY_FB 0
XR_DEFINE_ATOM(XrRenderModelKeyFB)

XR_RESULT_ENUM(XR_ERROR_RENDER_MODEL_KEY_INVALID_FB, -1000119000);
XR_RESULT_ENUM(XR_RENDER_MODEL_UNAVAILABLE_FB, 1000119020);

typedef XrFlags64 XrRenderModelFlagsFB;

XR_STRUCT_ENUM(XR_TYPE_RENDER_MODEL_PATH_INFO_FB, 1000119000);
typedef struct XrRenderModelPathInfoFB {
    XrStructureType type;
    void* XR_MAY_ALIAS next;
    XrPath path;
} XrRenderModelPathInfoFB;

XR_STRUCT_ENUM(XR_TYPE_RENDER_MODEL_PROPERTIES_FB, 1000119001);
typedef struct XrRenderModelPropertiesFB {
    XrStructureType type;
    void* XR_MAY_ALIAS next;
    uint32_t vendorId;
    char modelName[XR_MAX_RENDER_MODEL_NAME_SIZE_FB];
    XrRenderModelKeyFB modelKey;
    uint32_t modelVersion;
    XrRenderModelFlagsFB flags;
} XrRenderModelPropertiesFB;

XR_STRUCT_ENUM(XR_TYPE_RENDER_MODEL_BUFFER_FB, 1000119002);
typedef struct XrRenderModelBufferFB {
    XrStructureType type;
    void* XR_MAY_ALIAS next;
    uint32_t bufferCapacityInput;
    uint32_t bufferCountOutput;
    uint8_t* buffer;
} XrRenderModelBufferFB;

XR_STRUCT_ENUM(XR_TYPE_RENDER_MODEL_LOAD_INFO_FB, 1000119003);
typedef struct XrRenderModelLoadInfoFB {
    XrStructureType type;
    void* XR_MAY_ALIAS next;
    XrRenderModelKeyFB modelKey;
} XrRenderModelLoadInfoFB;

// system properties
XR_STRUCT_ENUM(XR_TYPE_SYSTEM_RENDER_MODEL_PROPERTIES_FB, 1000119004);
typedef struct XrSystemRenderModelPropertiesFB {
    XrStructureType type;
    const void* XR_MAY_ALIAS next;
    XrBool32 supportsRenderModelLoading;
} XrSystemRenderModelPropertiesFB;

typedef XrResult(XRAPI_PTR* PFN_xrEnumerateRenderModelPathsFB)(
    XrSession session,
    uint32_t pathCapacityInput,
    uint32_t* pathCountOutput,
    XrRenderModelPathInfoFB* pathInfo);

typedef XrResult(XRAPI_PTR* PFN_xrGetRenderModelPropertiesFB)(
    XrSession session,
    const XrPath path,
    XrRenderModelPropertiesFB* properties);

typedef XrResult(XRAPI_PTR* PFN_xrLoadRenderModelFB)(
    XrSession session,
    const XrRenderModelLoadInfoFB* info,
    XrRenderModelBufferFB* buffer);

#ifndef XR_NO_PROTOTYPES
#ifdef XR_EXTENSION_PROTOTYPES

XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateRenderModelPathsFB(
    XrSession session,
    uint32_t pathCapacityInput,
    uint32_t* pathCountOutput,
    XrRenderModelPathInfoFB* pathInfo);

XRAPI_ATTR XrResult XRAPI_CALL xrGetRenderModelPropertiesFB(
    XrSession session,
    const XrPath path,
    XrRenderModelPropertiesFB* properties);

XRAPI_ATTR XrResult XRAPI_CALL xrLoadRenderModelFB(
    XrSession session,
    const XrRenderModelLoadInfoFB* info,
    XrRenderModelBufferFB* buffer);

#endif /* XR_EXTENSION_PROTOTYPES */
#endif /* !XR_NO_PROTOTYPES */

#endif // XR_FB_render_model

#ifdef __cplusplus
}
#endif
