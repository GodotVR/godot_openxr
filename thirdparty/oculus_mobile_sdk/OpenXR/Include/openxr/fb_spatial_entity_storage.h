/************************************************************************************

Filename    :   fb_spatial_entity_storage.h
Content     :   spatial entity interface.
Language    :   C99

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

*************************************************************************************/

#pragma once

#include <openxr/fb_spatial_entity.h>

/*
  159 XR_FB_spatial_entity_storage
*/

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef XR_FB_spatial_entity_storage

#ifndef XR_FB_spatial_entity
#error "This extension depends XR_FB_spatial_entity which has not been defined"
#endif

// While experimental, the experimental version must be set to get extension definitions
#if defined(XR_FB_spatial_entity_storage_EXPERIMENTAL_VERSION)
// Error if the chosen experimental version is beyond the latest defined in this header
#if XR_FB_spatial_entity_storage_EXPERIMENTAL_VERSION < 1 || \
    2 < XR_FB_spatial_entity_storage_EXPERIMENTAL_VERSION
#error "unknown experimental version for XR_FB_spatial_entity_storage"
#endif

#define XR_FB_spatial_entity_storage 1

#define XR_FBX1_spatial_entity_storage_SPEC_VERSION 2
#define XR_FBX1_SPATIAL_ENTITY_STORAGE_EXTENSION_NAME "XR_FBX1_spatial_entity_storage"

#define XR_FBX2_spatial_entity_storage_SPEC_VERSION 2
#define XR_FBX2_SPATIAL_ENTITY_STORAGE_EXTENSION_NAME "XR_FBX2_spatial_entity_storage"

#if XR_FB_spatial_entity_storage_EXPERIMENTAL_VERSION == 1
#define XR_FB_spatial_entity_storage_SPEC_VERSION XR_FBX1_spatial_entity_storage_SPEC_VERSION
#define XR_FB_SPATIAL_ENTITY_STORAGE_EXTENSION_NAME XR_FBX1_SPATIAL_ENTITY_STORAGE_EXTENSION_NAME
#else
#define XR_FB_spatial_entity_storage_SPEC_VERSION XR_FBX2_spatial_entity_storage_SPEC_VERSION
#define XR_FB_SPATIAL_ENTITY_STORAGE_EXTENSION_NAME XR_FBX2_SPATIAL_ENTITY_STORAGE_EXTENSION_NAME
#endif

// In order to persist XrSpaces between application uses the XR_COMPONENT_TYPE_STORABLE_FB
// component can be enabled on a spatial entity that supports this functionality.  If this
// component has been enabled it allows for application developers to access the ability to
// save, load, and erase persisted XrSpaces.

// Storage location to be used to store, load, erase, and query spatial entities from
typedef enum XrSpatialEntityStorageLocationFB {
    XR_SPATIAL_ENTITY_STORAGE_LOCATION_INVALID_FB = 0,
    XR_SPATIAL_ENTITY_STORAGE_LOCATION_LOCAL_FB = 1, // local device storage
        XR_SPATIAL_ENTITY_STORAGE_LOCATION_MAX_ENUM_FB = 0x7FFFFFFF
} XrSpatialEntityStorageLocationFB;

typedef enum XrSpatialEntityStoragePersistenceModeFB {
    XR_SPATIAL_ENTITY_STORAGE_PERSISTENCE_MODE_INVALID_FB = 0,
    XR_SPATIAL_ENTITY_STORAGE_PERSISTENCE_MODE_INDEFINITE_HIGH_PRI_FB = 1,
    XR_SPATIAL_ENTITY_STORAGE_PERSISTENCE_MODE_MAX_ENUM_FB = 0x7FFFFFFF
} XrSpatialEntityStoragePersistenceModeFB;

static const XrStructureType XR_TYPE_SPATIAL_ENTITY_STORAGE_SAVE_INFO_FB =
    (XrStructureType)1000077000;
static const XrStructureType XR_TYPE_SPATIAL_ENTITY_STORAGE_ERASE_INFO_FB =
    (XrStructureType)1000077001;

static const XrStructureType XR_TYPE_SPATIAL_ENTITY_STORAGE_LOCATION_INFO_FB =
    (XrStructureType)1000077003;

static const XrStructureType XR_TYPE_EVENT_SPATIAL_ENTITY_STORAGE_SAVE_RESULT_FBX1 =
    (XrStructureType)1000077100;
static const XrStructureType XR_TYPE_EVENT_SPATIAL_ENTITY_STORAGE_ERASE_RESULT_FBX1 =
    (XrStructureType)1000077101;

#if XR_FB_spatial_entity_storage_EXPERIMENTAL_VERSION == 1
#define XR_TYPE_EVENT_SPATIAL_ENTITY_STORAGE_SAVE_RESULT_FB \
    XR_TYPE_EVENT_SPATIAL_ENTITY_STORAGE_SAVE_RESULT_FBX1
#define XR_TYPE_EVENT_SPATIAL_ENTITY_STORAGE_ERASE_RESULT_FB \
    XR_TYPE_EVENT_SPATIAL_ENTITY_STORAGE_ERASE_RESULT_FBX1
#define XR_TYPE_EVENT_SPATIAL_ENTITY_STORAGE_LOAD_RESULT_FB \
    XR_TYPE_EVENT_SPATIAL_ENTITY_STORAGE_LOAD_RESULT_FBX1
#else
static const XrStructureType XR_TYPE_EVENT_SPATIAL_ENTITY_STORAGE_SAVE_RESULT_FB =
    (XrStructureType)1000077103;
static const XrStructureType XR_TYPE_EVENT_SPATIAL_ENTITY_STORAGE_ERASE_RESULT_FB =
    (XrStructureType)1000077104;
static const XrStructureType XR_TYPE_EVENT_SPATIAL_ENTITY_STORAGE_LOAD_RESULT_FB =
    (XrStructureType)1000077105;
#endif

// Events

// Do to the asynchronos nature of storage events there are several result events that will
// be returned when these functions have been completed.

// Save Result
// The save result event contains the success of the save/write operation to the
// specified location as well as the XrSpace handle on which the save operation was attempted
// on in addition to the unique Uuid and the triggered async request id from the initial
// calling function
typedef struct XrEventSpatialEntityStorageSaveResultFBX1 {
    XrStructureType type; // XR_TYPE_EVENT_SPATIAL_ENTITY_STORAGE_SAVE_RESULT_FBX1
    const void* XR_MAY_ALIAS next; // contains a XrSpatialEntityIdInfoFB to access uuid
    XrResult result;
    XrSpace space;
    XrSpatialEntityUuidFBX1 uuid;
    XrAsyncRequestIdFB request;
} XrEventSpatialEntityStorageSaveResultFBX1;

#if XR_FB_spatial_entity_storage_EXPERIMENTAL_VERSION == 1
#define XrEventSpatialEntityStorageSaveResultFB XrEventSpatialEntityStorageSaveResultFBX1
#else
typedef struct XrEventSpatialEntityStorageSaveResultFB {
    XrStructureType type; // XR_TYPE_EVENT_SPATIAL_ENTITY_STORAGE_SAVE_RESULT_FB
    const void* XR_MAY_ALIAS next; // contains a XrSpatialEntityIdInfoFB to access uuid
    XrResult result;
    XrSpace space;
    XrSpatialEntityUuidFB uuid;
    XrAsyncRequestIdFB request;
} XrEventSpatialEntityStorageSaveResultFB;
#endif

// Erase Result
// The erase result event contains the success of the erase operation from the specified storage
// location.  It will also provide the uuid of the anchor and the async request id from the initial
// calling function
typedef struct XrEventSpatialEntityStorageEraseResultFBX1 {
    XrStructureType type; // XR_TYPE_EVENT_SPATIAL_ENTITY_STORAGE_ERASE_RESULT_FBX1
    const void* XR_MAY_ALIAS next; // contains a XrSpatialEntityIdInfoFB to access uuid
    XrResult result;
    XrSpatialEntityStorageLocationFB location;
    XrSpatialEntityUuidFBX1 uuid;
    XrAsyncRequestIdFB request;
} XrEventSpatialEntityStorageEraseResultFBX1;

#if XR_FB_spatial_entity_storage_EXPERIMENTAL_VERSION == 1
#define XrEventSpatialEntityStorageEraseResultFB XrEventSpatialEntityStorageEraseResultFBX1
#else
typedef struct XrEventSpatialEntityStorageEraseResultFB {
    XrStructureType type; // XR_TYPE_EVENT_SPATIAL_ENTITY_STORAGE_ERASE_RESULT_FB
    const void* XR_MAY_ALIAS next; // contains a XrSpatialEntityIdInfoFB to access uuid
    XrResult result;
    XrSpatialEntityStorageLocationFB location;
    XrSpatialEntityUuidFB uuid;
    XrAsyncRequestIdFB request;
} XrEventSpatialEntityStorageEraseResultFB;
#endif


// Info Structs

// Storage location info is used by the query filters and added to the next chain in order to
// specify which location the query filter wishes to perform it query from
typedef struct XrSpatialEntityStorageLocationInfoFB {
    XrStructureType type; // XR_TYPE_SPATIAL_ENTITY_STORAGE_LOCATION_INFO_FB
    const void* XR_MAY_ALIAS next;
    XrSpatialEntityStorageLocationFB location;
} XrSpatialEntityStorageLocationInfoFB;

// Spatial entity save information used by xrSpatialEntitySaveSpaceFB
typedef struct XrSpatialEntityStorageSaveInfoFB {
    XrStructureType type; // XR_TYPE_SPATIAL_ENTITY_STORAGE_SAVE_INFO_FB
    const void* XR_MAY_ALIAS next;
    XrSpace space;
    XrSpatialEntityStorageLocationFB location;
    XrSpatialEntityStoragePersistenceModeFB persistenceMode;
} XrSpatialEntityStorageSaveInfoFB;

// Spatial entity erase information used by xrSpatialEntityEraseSpaceFB
typedef struct XrSpatialEntityStorageEraseInfoFB {
    XrStructureType type; // XR_TYPE_SPATIAL_ENTITY_STORAGE_ERASE_INFO_FB
    const void* XR_MAY_ALIAS next;
    XrSpace space;
    XrSpatialEntityStorageLocationFB location;
} XrSpatialEntityStorageEraseInfoFB;


typedef XrResult(XRAPI_PTR* PFN_xrSpatialEntitySaveSpaceFB)(
    XrSession session,
    const XrSpatialEntityStorageSaveInfoFB* info,
    XrAsyncRequestIdFB* request);

typedef XrResult(XRAPI_PTR* PFN_xrSpatialEntityEraseSpaceFB)(
    XrSession session,
    const XrSpatialEntityStorageEraseInfoFB* info,
    XrAsyncRequestIdFB* request);


#ifndef XR_NO_PROTOTYPES
#ifdef XR_EXTENSION_PROTOTYPES

XRAPI_ATTR XrResult XRAPI_CALL xrSpatialEntitySaveSpaceFB(
    XrSession session,
    const XrSpatialEntityStorageSaveInfoFB* info,
    XrAsyncRequestIdFB* request);

XRAPI_ATTR XrResult XRAPI_CALL xrSpatialEntityEraseSpaceFB(
    XrSession session,
    const XrSpatialEntityStorageEraseInfoFB* info,
    XrAsyncRequestIdFB* request);


#endif // XR_EXTENSION_PROTOTYPES
#endif // XR_NO_PROTOTYPES

#endif // defined(XR_FB_spatial_entity_storage_EXPERIMENTAL_VERSION)

#endif // XR_FB_spatial_entity_storage

#ifdef __cplusplus
}
#endif
