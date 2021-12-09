/************************************************************************************

Filename    :   fb_spatial_entity.h
Content     :   spatial entity interface.
Language    :   C99

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

*************************************************************************************/

#pragma once

/*
  114 XR_FB_spatial_entity
*/

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef XR_FB_spatial_entity

// While experimental, the experimental version must be set to get extension definitions
#if defined(XR_FB_spatial_entity_EXPERIMENTAL_VERSION)
// Error if the chosen experimental version is beyond the latest defined in this header
#if XR_FB_spatial_entity_EXPERIMENTAL_VERSION < 1 || 2 < XR_FB_spatial_entity_EXPERIMENTAL_VERSION
#error "unknown experimental version for XR_FB_spatial_entity"
#endif

#define XR_FB_spatial_entity 1

#define XR_FBX1_spatial_entity_SPEC_VERSION 2
#define XR_FBX1_SPATIAL_ENTITY_EXTENSION_NAME "XR_FBX1_spatial_entity"

#define XR_FBX2_spatial_entity_SPEC_VERSION 2
#define XR_FBX2_SPATIAL_ENTITY_EXTENSION_NAME "XR_FBX2_spatial_entity"

#if XR_FB_spatial_entity_EXPERIMENTAL_VERSION == 1
#define XR_FB_spatial_entity_SPEC_VERSION XR_FBX1_spatial_entity_SPEC_VERSION
#define XR_FB_SPATIAL_ENTITY_EXTENSION_NAME XR_FBX1_SPATIAL_ENTITY_EXTENSION_NAME
#else
#define XR_FB_spatial_entity_SPEC_VERSION XR_FBX2_spatial_entity_SPEC_VERSION
#define XR_FB_SPATIAL_ENTITY_EXTENSION_NAME XR_FBX2_SPATIAL_ENTITY_EXTENSION_NAME
#endif

// Potentially long running requests return an async request identifier
// which will be part of messages returned via events. The nature of
// the response event or events depends on the request, but the async
// request identifier, returned immediately at request time, is used
// to associate responses with requests.
typedef uint64_t XrAsyncRequestIdFB;

static const XrResult XR_ERROR_COMPONENT_NOT_SUPPORTED_FB = (XrResult)-1000113000;
static const XrResult XR_ERROR_COMPONENT_NOT_ENABLED_FB = (XrResult)-1000113001;
static const XrResult XR_ERROR_SET_COMPONENT_ENABLE_PENDING_FB = (XrResult)-1000113002;
static const XrResult XR_ERROR_SET_COMPONENT_ENABLE_ALREADY_ENABLED_FB = (XrResult)-1000113003;

static const XrStructureType XR_TYPE_COMPONENT_ENABLE_REQUEST_FB = (XrStructureType)1000113000;
static const XrStructureType XR_TYPE_COMPONENT_STATUS_FB = (XrStructureType)1000113001;
static const XrStructureType XR_TYPE_EVENT_DATA_SET_COMPONENT_ENABLE_RESULT_FB =
    (XrStructureType)1000113002;
static const XrStructureType XR_TYPE_SPATIAL_ANCHOR_CREATE_INFO_FB = (XrStructureType)1000011303;
static const XrStructureType XR_TYPE_SYSTEM_SPATIAL_ENTITY_PROPERTIES_FB =
    (XrStructureType)1000011304;

// Every XrSpace object represents a spatial entity that may support an
// arbitrary number of component interfaces. If the component interface
// is advertised as supported, and the support is enabled, then functions
// associated with that interface may be used with that XrSpace.

// Whether a component interface is enabled at creation depends on the
// method of creation. All core v1.0 XrSpace creation functions enable
// the XR_COMPONENT_TYPE_LOCATABLE_FB component at creation.

// The XR_COMPONENT_TYPE_LOCATABLE_FB interface must be enabled to use
// xrLocateSpace() on a space or to use the space in any function where
// the runtime will need to locate the space.

// When functions use a space in a context where one or more necessary
// components are not enabled for the space,
// XR_ERROR_COMPONENT_NOT_ENABLED_FB must be returned.

// Issue: Should disable of LOCATABLE be allowed for any space?
// Resolution: Yes. It may not serve any useful purpose to disable
//             LOCATABLE for e.g. a STAGE space, but it signals the
//             app's intent not to locate the space, and keep it
//             consistent with other spaces where disabling LOCATABLE
//             would be desirable when the app is not actively tracking
//             the entity.

typedef enum XrComponentTypeFB {
    XR_COMPONENT_TYPE_LOCATABLE_FB = 0, // Works with xrLocateSpace, etc.
    XR_COMPONENT_TYPE_STORABLE_FB = 1, // enables save, load, erase, etc.
        XR_COMPONENT_TYPE_MAX_ENUM_FB = 0x7FFFFFFF
} XrComponentTypeFB;

// Type of spatial entity represented by an XrSpace which can be used in
// query info
typedef enum XrSpatialEntityTypeFBX1 {
    XR_SPATIAL_ENTITY_TYPE_INVALID_FBX1 = 0,
    XR_SPATIAL_ENTITY_TYPE_SPATIAL_ANCHOR_FBX1 = 1,
    XR_SPATIAL_ENTITY_TYPE_MAX_ENUM_FBX1 = 0x7FFFFFFF
} XrSpatialEntityTypeFBX1;

#if XR_FB_spatial_entity_EXPERIMENTAL_VERSION == 1
#define XrSpatialEntityTypeFB XrSpatialEntityTypeFBX1
#define XR_SPATIAL_ENTITY_TYPE_INVALID_FB XR_SPATIAL_ENTITY_TYPE_INVALID_FBX1
#define XR_SPATIAL_ENTITY_TYPE_SPATIAL_ANCHOR_FB XR_SPATIAL_ENTITY_TYPE_SPATIAL_ANCHOR_FBX1
#define XR_SPATIAL_ENTITY_TYPE_MAX_ENUM_FB XR_SPATIAL_ENTITY_TYPE_MAX_ENUM_FBX1
#endif

// Spatial Entity system properties
typedef struct XrSystemSpatialEntityPropertiesFB {
    XrStructureType type;
    const void* XR_MAY_ALIAS next;
    XrBool32 supportsSpatialEntity;
} XrSystemSpatialEntityPropertiesFB;

// Spatial Entity Uuid
#define XR_UUID_SIZE_FB 2
typedef struct XrSpatialEntityUuidFBX1 {
    XrSpatialEntityTypeFBX1 uuidType;
    uint64_t value[XR_UUID_SIZE_FB];
} XrSpatialEntityUuidFBX1;

#if XR_FB_spatial_entity_EXPERIMENTAL_VERSION == 1
#define XrSpatialEntityUuidFB XrSpatialEntityUuidFBX1
#else
typedef struct XrSpatialEntityUuidFB {
    uint64_t value[XR_UUID_SIZE_FB];
} XrSpatialEntityUuidFB;
#endif

// Issue: Should component enable just take a boolean?
// Resolution: No. It is possible that future component interfaces may
//             need more configuration on enable than simply a boolean.
//             While the base enable request struct only provides the
//             boolean, we have the next chain to future-proof this
//             request.

typedef struct XrComponentEnableRequestFB {
    XrStructureType type; // XR_TYPE_COMPONENT_ENABLE_REQUEST_FB
    const void* XR_MAY_ALIAS next;
    XrComponentTypeFB componentType;
    XrBool32 enable;
    XrDuration timeout;
} XrComponentEnableRequestFB;

// Next chain provided here for future component interfaces that
// may have more detailed status to provide.

// The change pending flag is set immediately upon requesting a
// component enabled change, but the change in the enabled state
// is not reflected in the status until the async request
// completes successfully.

typedef struct XrComponentStatusFB {
    XrStructureType type; // XR_TYPE_COMPONENT_STATUS_FB
    const void* XR_MAY_ALIAS next;
    XrBool32 enabled;
    XrBool32 changePending;
} XrComponentStatusFB;

// The SetComponentEnabled request delivers the result of the
// request via event. If the request was not successful, the
// component status will be unaltered except that the change
// pending status cleared.

typedef struct XrEventDataSetComponentEnableResultFB {
    XrStructureType type; // XR_TYPE_EVENT_DATA_SET_COMPONENT_ENABLE_RESULT_FB
    const void* XR_MAY_ALIAS next;
    XrAsyncRequestIdFB requestId;
    XrResult result;
    XrComponentTypeFB componentType;
    XrSpace space;
} XrEventDataSetComponentEnableResultFB;

// Create info struct used when creating a spatial anchor

typedef struct XrSpatialAnchorCreateInfoFB {
    XrStructureType type; // XR_TYPE_SPATIAL_ANCHOR_CREATE_INFO_FB
    void* XR_MAY_ALIAS next;
    XrSpace space;
    XrPosef poseInSpace;
    XrTime time;
} XrSpatialAnchorCreateInfoFB;

// The CreateSpatialAnchor method constructs a new spatial anchor space

typedef XrResult(XRAPI_PTR* PFN_xrCreateSpatialAnchorFB)(
    XrSession session,
    const XrSpatialAnchorCreateInfoFB* info,
    XrSpace* space);

#ifndef XR_NO_PROTOTYPES
#ifdef XR_EXTENSION_PROTOTYPES

// All the component interfaces that an entity supports can be discovered
// through the xrEnumerateSupportedComponentsFB() function. The list of
// supported components will not change over the life of the entity.

// The list of component interfaces available for an entity may depend on
// which extensions are enabled. Component interfaces will not be enumerated
// unless the corresponding extension that defines them is also enabled.

XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateSupportedComponentsFB(
    XrSpace space,
    uint32_t componentTypesCapacityInput,
    uint32_t* componentTypesCountOutput,
    XrComponentTypeFB* componentTypes);

// xrSetComponentEnabledFB() enables or disables any supported component
// interface for a space. Some component interfaces may take time to become
// ready. Rather than block, xrSetComponentEnabledFB() returns an async
// request identifier, and its result is returned via event.

// XR_ERROR_SET_COMPONENT_ENABLE_PENDING is returned if xrSetComponentEnabledFB()
// is called on a space that has an xrSetComponentEnabledFB() call pending already.

// Issue: If requestId is null, should that be a) an error, b) just don't return
//        the requestId, or c) function operates synchronously?
// Resolution: (b) It's not great form for apps to fire and forget these requests,
//             but they could pass a dummy arg and ignore it later anyway.

XRAPI_ATTR XrResult XRAPI_CALL xrSetComponentEnabledFB(
    XrSpace space,
    const XrComponentEnableRequestFB* request,
    XrAsyncRequestIdFB* requestId);

// xrGetComponentStatus is used to determine whether a component interface is
// currently enabled for a space.

XRAPI_ATTR XrResult XRAPI_CALL
xrGetComponentStatusFB(XrSpace space, XrComponentTypeFB componentType, XrComponentStatusFB* status);

// xrCreateSpatialAnchorFB is used to create a new spatial anchor space

XRAPI_ATTR XrResult XRAPI_CALL
xrCreateSpatialAnchorFB(XrSession session, const XrSpatialAnchorCreateInfoFB* info, XrSpace* space);

// xrGetSpatialEntityUuidFB is used to access the uuid for an XrSpace if supported

XRAPI_ATTR XrResult XRAPI_CALL
xrGetSpatialEntityUuidFBX1(XrSpace space, XrSpatialEntityUuidFBX1* uuid);

#if XR_FB_spatial_entity_EXPERIMENTAL_VERSION == 1
#define xrGetSpatialEntityUuidFB xrGetSpatialEntityUuidFBX1
#else
XRAPI_ATTR XrResult XRAPI_CALL xrGetSpatialEntityUuidFB(XrSpace space, XrSpatialEntityUuidFB* uuid);
#endif

#endif /* XR_EXTENSION_PROTOTYPES */
#endif /* !XR_NO_PROTOTYPES */

typedef XrResult(XRAPI_PTR* PFN_xrEnumerateSupportedComponentsFB)(
    XrSpace space,
    uint32_t componentTypesCapacityInput,
    uint32_t* componentTypesCountOutput,
    XrComponentTypeFB* componentTypes);

typedef XrResult(XRAPI_PTR* PFN_xrSetComponentEnabledFB)(
    XrSpace space,
    const XrComponentEnableRequestFB* request,
    XrAsyncRequestIdFB* asyncRequest);

typedef XrResult(XRAPI_PTR* PFN_xrGetComponentStatusFB)(
    XrSpace space,
    XrComponentTypeFB componentType,
    XrComponentStatusFB* status);

typedef XrResult(
    XRAPI_PTR* PFN_xrGetSpatialEntityUuidFBX1)(XrSpace space, XrSpatialEntityUuidFBX1* uuid);

#if XR_FB_spatial_entity_EXPERIMENTAL_VERSION == 1
#define PFN_xrGetSpatialEntityUuidFB PFN_xrGetSpatialEntityUuidFBX1
#else
typedef XrResult(
    XRAPI_PTR* PFN_xrGetSpatialEntityUuidFB)(XrSpace space, XrSpatialEntityUuidFB* uuid);
#endif

#endif // defined(XR_FB_spatial_entity_EXPERIMENTAL_VERSION)

#endif // XR_FB_spatial_entity

#ifdef __cplusplus
}
#endif
