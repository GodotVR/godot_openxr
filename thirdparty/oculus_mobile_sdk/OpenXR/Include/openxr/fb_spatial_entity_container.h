/************************************************************************************

Filename    :   fb_spatial_entity_container.h
Content     :   Spatial entity container functionality.
Language    :   C99

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

*************************************************************************************/

#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

// Extension 200

#ifndef XR_FB_spatial_entity_container
#define XR_FB_spatial_entity_container 1


#ifndef XR_FB_spatial_entity_container_EXPERIMENTAL_VERSION
#define XR_FB_spatial_entity_container_SPEC_VERSION 1
#define XR_FB_SPATIAL_ENTITY_CONTAINER_EXTENSION_NAME "XR_FB_spatial_entity_container"
#else
#error "unknown experimental version number for XR_FB_spatial_entity_container_EXPERIMENTAL_VERSION"
#endif

// Space container component.
static const XrStructureType XR_TYPE_SPACE_CONTAINER_FB = (XrStructureType)1000199000;
typedef struct XrSpaceContainerFB {
    XrStructureType type;
    const void* XR_MAY_ALIAS next;

    // Input, capacity of UUID list.
    uint32_t uuidCapacityInput;

    // Output, number of spatial entities included in the list.
    uint32_t uuidCountOutput;

    // List of spatial entities contained in the entity to which this component is attached.
    XrUuidEXT* uuids;
} XrSpaceContainerFB;

// Get space container component.
typedef XrResult(XRAPI_PTR* PFN_xrGetSpaceContainerFB)(
    XrSession session,
    XrSpace space,
    XrSpaceContainerFB* spaceContainerOutput);

#ifndef XR_NO_PROTOTYPES
#ifdef XR_EXTENSION_PROTOTYPES

// Get space container component.
// Note: This functions uses two-call idiom:
// 1) When uuidsCapacityInput == 0, only uuidsCountOutput will be updated and no UUIDs will be
// copied;
// 2) When uuidsCapacityInput >= uuidsCountOutput, UUIDs will be copied to
// spaceContainerOutput;
// 3) Otherwise returns XR_ERROR_SIZE_INSUFFICIENT.
XRAPI_ATTR XrResult XRAPI_CALL
xrGetSpaceContainerFB(XrSession session, XrSpace space, XrSpaceContainerFB* spaceContainerOutput);

#endif // XR_EXTENSION_PROTOTYPES
#endif // !XR_NO_PROTOTYPES


#endif // XR_FB_spatial_entity_container

#ifdef __cplusplus
}
#endif
