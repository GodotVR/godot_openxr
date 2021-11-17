/************************************************************************************

Filename    :   fb_passthrough.h
Content     :   Passthrough functionality.
Language    :   C99

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

*************************************************************************************/

#pragma once

#include <openxr/openxr_extension_helpers.h>
#include <openxr/fb_triangle_mesh.h>

#if defined(__cplusplus)
extern "C" {
#endif

// Extension 119

#ifndef XR_FB_passthrough

#if defined(XR_FB_passthrough_EXPERIMENTAL_VERSION)
#error "Experimental versions of XR_FB_passthrough extension are no longer supported"
#endif

#define XR_FB_passthrough 1
#define XR_FB_passthrough_SPEC_VERSION 1
#define XR_FB_PASSTHROUGH_EXTENSION_NAME "XR_FB_passthrough"

// Handles for object types
XR_DEFINE_HANDLE(XrPassthroughFB)
XR_DEFINE_HANDLE(XrPassthroughLayerFB)
XR_DEFINE_HANDLE(XrGeometryInstanceFB)

// The system is in an unexpected state for the specific call.
// Usually these refer to an internal state management mismatch.
XR_RESULT_ENUM(XR_ERROR_UNEXPECTED_STATE_PASSTHROUGH_FB, -1000118000);
// Trying to create an MR feature when one was already created and only one instance is allowed.
XR_RESULT_ENUM(XR_ERROR_FEATURE_ALREADY_CREATED_PASSTHROUGH_FB, -1000118001);
// Requested functionality requires a feature to be created first.
XR_RESULT_ENUM(XR_ERROR_FEATURE_REQUIRED_PASSTHROUGH_FB, -1000118002);
// Requested functionality is not permitted - application is not allowed to perform the requested
// operation.
XR_RESULT_ENUM(XR_ERROR_NOT_PERMITTED_PASSTHROUGH_FB, -1000118003);
// There were no sufficient resources available to perform an operation.
// The error may be returned for different kinds of resources,
// including GPU/rendering, memory, or application-provided buffer sizes.
XR_RESULT_ENUM(XR_ERROR_INSUFFICIENT_RESOURCES_PASSTHROUGH_FB, -1000118004);
// Error that does not provide any more specifics. Will use for internal errors as well.
XR_RESULT_ENUM(XR_ERROR_UNKNOWN_PASSTHROUGH_FB, -1000118050);

// Passthrough system properties
XR_STRUCT_ENUM(XR_TYPE_SYSTEM_PASSTHROUGH_PROPERTIES_FB, 1000118000);
typedef struct XrSystemPassthroughPropertiesFB {
    XrStructureType type;
    const void* XR_MAY_ALIAS next;
    XrBool32 supportsPassthrough;
} XrSystemPassthroughPropertiesFB;

// Composition layer purpose values
typedef enum XrPassthroughLayerPurposeFB {
    // Layer will be used for reconstruction passthrough.
    XR_PASSTHROUGH_LAYER_PURPOSE_RECONSTRUCTION_FB = 0,
    // Layer will be used for surface-projected passthrough.
    XR_PASSTHROUGH_LAYER_PURPOSE_PROJECTED_FB = 1,
    // Special value, forces 32 bit integer base type.
    XR_PASSTHROUGH_LAYER_PURPOSE_MAX_ENUM_FB = 0x7FFFFFFF
} XrPassthroughLayerPurposeFB;

// Passthrough layer configuration
typedef XrFlags64 XrPassthroughFlagsFB;

// Flag bits for XrPassthroughFlagsFB

// Indicates the passthrough object defaults to running on creation
static const XrPassthroughFlagsFB XR_PASSTHROUGH_IS_RUNNING_AT_CREATION_BIT_FB = 0x00000001;

// Passthrough feature create info
XR_STRUCT_ENUM(XR_TYPE_PASSTHROUGH_CREATE_INFO_FB, 1000118001);
typedef struct XrPassthroughCreateInfoFB {
    XrStructureType type;
    const void* XR_MAY_ALIAS next;
    XrPassthroughFlagsFB flags;
} XrPassthroughCreateInfoFB;

// Passthrough layer create info
XR_STRUCT_ENUM(XR_TYPE_PASSTHROUGH_LAYER_CREATE_INFO_FB, 1000118002);
typedef struct XrPassthroughLayerCreateInfoFB {
    XrStructureType type;
    const void* XR_MAY_ALIAS next;
    XrPassthroughFB passthrough;
    XrPassthroughFlagsFB flags;
    XrPassthroughLayerPurposeFB purpose;
} XrPassthroughLayerCreateInfoFB;

// Composition layer structure for a passthrough layer - used with `xrEndFrame`
XR_STRUCT_ENUM(XR_TYPE_COMPOSITION_LAYER_PASSTHROUGH_FB, 1000118003);
typedef struct XrCompositionLayerPassthroughFB {
    XrStructureType type;
    const void* XR_MAY_ALIAS next;
    // Layer flags.
    // Chromatic aberration correction flag will be ignored,
    // as layer content is provided by the system.
    // Flags that affect blend modes will be taken into account, along with
    // optionally chained blend mode structure values.
    XrCompositionLayerFlags layerFlags;
    // Space is added for compatibility with XrCompositionLayerBaseHeader.
    // Must be XR_NULL_HANDLE.
    XrSpace space;
    // Handle of a layer providing passthrough content.
    XrPassthroughLayerFB layerHandle;
} XrCompositionLayerPassthroughFB;

// Parts that connect passthrough and meshes depend on the mesh extension.
#if defined(XR_FB_triangle_mesh)

// Passthrough Geometry Instance create info
XR_STRUCT_ENUM(XR_TYPE_GEOMETRY_INSTANCE_CREATE_INFO_FB, 1000118004);
typedef struct XrGeometryInstanceCreateInfoFB {
    XrStructureType type;
    const void* XR_MAY_ALIAS next;
    XrPassthroughLayerFB layer;
    XrTriangleMeshFB mesh;
    XrSpace baseSpace;
    XrPosef pose;
    XrVector3f scale;
} XrGeometryInstanceCreateInfoFB;

XR_STRUCT_ENUM(XR_TYPE_GEOMETRY_INSTANCE_TRANSFORM_FB, 1000118005);
typedef struct XrGeometryInstanceTransformFB {
    XrStructureType type;
    const void* XR_MAY_ALIAS next;
    XrSpace baseSpace;
    XrTime time;
    XrPosef pose;
    XrVector3f scale;
} XrGeometryInstanceTransformFB;

#endif // defined(XR_FB_triangle_mesh)

XR_STRUCT_ENUM(XR_TYPE_PASSTHROUGH_STYLE_FB, 1000118020);
typedef struct XrPassthroughStyleFB {
    XrStructureType type;
    const void* XR_MAY_ALIAS next;
    // A factor by which the camera image / color map values are multiplied during rendering. Must
    // be a number in the range [0, 1]. Setting this value to 0 disables rendering of the camera
    // images. Edge rendering is not affected by this value.
    float textureOpacityFactor;
    // RGBA color of edges. The passthrough feature extracts edges from theoriginal camera feed and
    // renders them on top of the camera texture. Edge rendering is disabled when the alpha value
    // of `edgeColor` is zero.
    XrColor4f edgeColor;
} XrPassthroughStyleFB;

#define XR_PASSTHROUGH_COLOR_MAP_MONO_SIZE_FB 256

// Passthrough color map configuration: Mono -> RGBA
// Chain to `XrPassthroughStyleFB`
XR_STRUCT_ENUM(XR_TYPE_PASSTHROUGH_COLOR_MAP_MONO_TO_RGBA_FB, 1000118021);
typedef struct XrPassthroughColorMapMonoToRgbaFB {
    XrStructureType type;
    const void* XR_MAY_ALIAS next;
    // Color mapping translates each grayscale input value from the camera feed to an RGBA value
    // that is shown in passthrough. The color map is provided as an array of 256 color values
    // (RGBA) that are mapped to the corresponding intensities from the camera feed.
    // NOTE: For XR_FB_passthrough, all color values are treated as linear.
    XrColor4f textureColorMap[XR_PASSTHROUGH_COLOR_MAP_MONO_SIZE_FB];
} XrPassthroughColorMapMonoToRgbaFB;

// Passthrough color map configuration: Mono -> RGBA
XR_STRUCT_ENUM(XR_TYPE_PASSTHROUGH_COLOR_MAP_MONO_TO_MONO_FB, 1000118022);
typedef struct XrPassthroughColorMapMonoToMonoFB {
    XrStructureType type;
    const void* XR_MAY_ALIAS next;
    // Color mapping translates each grayscale input value from the camera feed to a grayscale
    // value that is shown in passthrough. The color map is provided as an array of 256 scalar
    // values that are mapped to the corresponding intensities from the camera feed.
    // NOTE: For XR_FB_passthrough, all color values are treated as linear.
    uint8_t textureColorMap[XR_PASSTHROUGH_COLOR_MAP_MONO_SIZE_FB];
} XrPassthroughColorMapMonoToMonoFB;

// Passthrough state change event

typedef XrFlags64 XrPassthroughStateChangedFlagsFB;

// Indicates the system component failed and the application must re-initialize.
static const XrPassthroughStateChangedFlagsFB XR_PASSTHROUGH_STATE_CHANGED_REINIT_REQUIRED_BIT_FB =
    0x00000001;
// Indicates the system component encountered a non-recoverable error.
// Reboot, firmware update or even a reset may be required.
static const XrPassthroughStateChangedFlagsFB
    XR_PASSTHROUGH_STATE_CHANGED_NON_RECOVERABLE_ERROR_BIT_FB = 0x00000002;
// Indicates the system component encountered a recoverable error.
// The system will attempt to recover, but until it does the functionality may be unavaliable.
// For example, passthrough will not be rendered.
static const XrPassthroughStateChangedFlagsFB
    XR_PASSTHROUGH_STATE_CHANGED_RECOVERABLE_ERROR_BIT_FB = 0x00000004;
// Indicates the system component recovered from a previous error.
static const XrPassthroughStateChangedFlagsFB XR_PASSTHROUGH_STATE_CHANGED_RESTORED_ERROR_BIT_FB =
    0x00000008;

XR_STRUCT_ENUM(XR_TYPE_EVENT_DATA_PASSTHROUGH_STATE_CHANGED_FB, 1000118030);
typedef struct XrEventDataPassthroughStateChangedFB {
    /// Structure header
    /// @{
    XrStructureType type;
    const void* XR_MAY_ALIAS next;
    /// @}

    /// Event details: flags that describe the state change
    XrPassthroughStateChangedFlagsFB flags;
} XrEventDataPassthroughStateChangedFB;

// Create a passthrough feature
typedef XrResult(XRAPI_PTR* PFN_xrCreatePassthroughFB)(
    XrSession session,
    const XrPassthroughCreateInfoFB* createInfo,
    XrPassthroughFB* featureOut);
// Destroy a previously created passthrough feature
typedef XrResult(XRAPI_PTR* PFN_xrDestroyPassthroughFB)(XrPassthroughFB feature);

typedef XrResult(XRAPI_PTR* PFN_xrPassthroughStartFB)(XrPassthroughFB passthrough);
// Pause the passthrough feature
typedef XrResult(XRAPI_PTR* PFN_xrPassthroughPauseFB)(XrPassthroughFB passthrough);

typedef XrResult(XRAPI_PTR* PFN_xrCreatePassthroughLayerFB)(
    XrSession session,
    const XrPassthroughLayerCreateInfoFB* createInfo,
    XrPassthroughLayerFB* layerOut);
typedef XrResult(XRAPI_PTR* PFN_xrDestroyPassthroughLayerFB)(XrPassthroughLayerFB layer);

typedef XrResult(XRAPI_PTR* PFN_xrPassthroughLayerPauseFB)(XrPassthroughLayerFB layer);
typedef XrResult(XRAPI_PTR* PFN_xrPassthroughLayerResumeFB)(XrPassthroughLayerFB layer);

typedef XrResult(XRAPI_PTR* PFN_xrPassthroughLayerSetStyleFB)(
    XrPassthroughLayerFB layer,
    const XrPassthroughStyleFB* style);

// Parts that connect passthrough and meshes depend on the mesh extension.
#if defined(XR_FB_triangle_mesh)
typedef XrResult(XRAPI_PTR* PFN_xrCreateGeometryInstanceFB)(
    XrSession session,
    const XrGeometryInstanceCreateInfoFB* createInfo,
    XrGeometryInstanceFB* outGeomtetryInstance);
typedef XrResult(XRAPI_PTR* PFN_xrDestroyGeometryInstanceFB)(XrGeometryInstanceFB instance);
typedef XrResult(XRAPI_PTR* PFN_xrGeometryInstanceSetTransformFB)(
    XrGeometryInstanceFB instance,
    const XrGeometryInstanceTransformFB* transformation);
#endif // defined(XR_FB_triangle_mesh)

#ifndef XR_NO_PROTOTYPES
#ifdef XR_EXTENSION_PROTOTYPES

// Create a passthrough feature
XRAPI_ATTR XrResult XRAPI_CALL xrCreatePassthroughFB(
    XrSession session,
    const XrPassthroughCreateInfoFB* createInfo,
    XrPassthroughFB* featureOut);
// Destroy a previously created passthrough feature
XRAPI_ATTR XrResult XRAPI_CALL xrDestroyPassthroughFB(XrPassthroughFB feature);

// Passthrough feature state management functions
// Start the passthrough feature
XRAPI_ATTR XrResult XRAPI_CALL xrPassthroughStartFB(XrPassthroughFB passthrough);
// Pause the passthrough feature
XRAPI_ATTR XrResult XRAPI_CALL xrPassthroughPauseFB(XrPassthroughFB passthrough);

XRAPI_ATTR XrResult XRAPI_CALL xrCreatePassthroughLayerFB(
    XrSession session,
    const XrPassthroughLayerCreateInfoFB* config,
    XrPassthroughLayerFB* layerOut);
XRAPI_ATTR XrResult XRAPI_CALL xrDestroyPassthroughLayerFB(XrPassthroughLayerFB layer);

XRAPI_ATTR XrResult XRAPI_CALL xrPassthroughLayerPauseFB(XrPassthroughLayerFB layer);
XRAPI_ATTR XrResult XRAPI_CALL xrPassthroughLayerResumeFB(XrPassthroughLayerFB layer);

// Set the style of an existing passthrough layer. If the enabled feature set
// doesn’t change, this is a lightweight operation that can be called in every
// frame to animate the style. Changes that may incur a bigger cost:
// - Enabling/disabling the color mapping, or changing the type of mapping
//   (monochromatic to RGBA or back).
// - Changing `textureOpacityFactor` from 0 to non-zero or vice versa
// - Changing `edgeColor[3]` from 0 to non-zero or vice versa
// NOTE: For XR_FB_passthrough, all color values are treated as linear.
XRAPI_ATTR XrResult XRAPI_CALL
xrPassthroughLayerSetStyleFB(XrPassthroughLayerFB layer, const XrPassthroughStyleFB* style);

// Parts that connect passthrough and meshes depend on the mesh extension.
#if defined(XR_FB_triangle_mesh)
// Create a geometry instance to be used as a projection surface for passthrough.
// A geometry instance assigns a triangle mesh as part of the specified layer's
// projection surface.
// The operation is only valid if the passthrough layer's purpose has been set to
// `XR_PASSTHROUGH_LAYER_PURPOSE_PROJECTED_FB`. Otherwise, the call this function will
// result in an error. In the specified layer, Passthrough will be visible where the view
// is covered by the user-specified geometries.
//
// A triangle mesh object can be instantiated multiple times - in the same or different layers'
// projection surface. Each instantiation has its own transformation, which
// can be updated using `xrGeometryInstanceSetTransformFB`.
XRAPI_ATTR XrResult XRAPI_CALL xrCreateGeometryInstanceFB(
    XrSession session,
    const XrGeometryInstanceCreateInfoFB* createInfo,
    XrGeometryInstanceFB* outGeomtetryInstance);

// Destroys a previously created geometry instance from passthrough rendering.
// This removes the geometry instance from passthrough rendering.
// The operation has no effect on other instances or the underlying mesh.
XRAPI_ATTR XrResult XRAPI_CALL xrDestroyGeometryInstanceFB(XrGeometryInstanceFB instance);

// Update the transformation of a passthrough geometry instance.
XRAPI_ATTR XrResult XRAPI_CALL xrGeometryInstanceSetTransformFB(
    XrGeometryInstanceFB instance,
    const XrGeometryInstanceTransformFB* transformation);
#endif // defined(XR_FB_triangle_mesh)

#endif /* XR_EXTENSION_PROTOTYPES */
#endif /* !XR_NO_PROTOTYPES */

#endif // XR_FB_passthrough

#ifdef __cplusplus
}
#endif
