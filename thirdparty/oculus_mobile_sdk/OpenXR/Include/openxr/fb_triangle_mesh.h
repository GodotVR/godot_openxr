/************************************************************************************

Filename    :   fb_triangle_mesh.h
Content     :   MR mesh API definitions.
Language    :   C99

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

*************************************************************************************/

#pragma once

#include <openxr/openxr_extension_helpers.h>

#if defined(__cplusplus)
extern "C" {
#endif

// Extension 118
#ifndef XR_FB_triangle_mesh

#if defined(XR_FB_mr_triangle_mesh_EXPERIMENTAL_VERSION)
#error "Experimental versions of XR_FB_triangle_mesh extension are no longer supported"
#endif

#define XR_FB_triangle_mesh 1
#define XR_FB_triangle_mesh_SPEC_VERSION 1
#define XR_FB_TRIANGLE_MESH_EXTENSION_NAME "XR_FB_triangle_mesh"

XR_DEFINE_HANDLE(XrTriangleMeshFB)

// Specifies whether the vertices of an outward-facing side of a triangle appear in clockwise or
// counter-clockwise order. See https://www.khronos.org/opengl/wiki/Face_Culling#Winding_order for
// more details. `XR_WINDING_ORDER_UNKNOWN_FB` specifies that the winding order of the
// triangles is unknown and that the SDK cannot make any assumptions on the triangle orientation
// (e.g. for backface culling).
typedef enum XrWindingOrderFB {
    XR_WINDING_ORDER_UNKNOWN_FB = 0,
    XR_WINDING_ORDER_CW_FB = 1,
    XR_WINDING_ORDER_CCW_FB = 2,
    XR_WINDING_ORDER_MAX_ENUM_FB = 0x7FFFFFFF
} XrWindingOrderFB;

typedef XrFlags64 XrTriangleMeshFlagsFB;

// Flag bits for XrTriangleMeshFlagsFB

// Indicates the triangle mesh can be mutated after creation
static const XrTriangleMeshFlagsFB XR_TRIANGLE_MESH_MUTABLE_BIT_FB = 0x00000001;

XR_STRUCT_ENUM(XR_TYPE_TRIANGLE_MESH_CREATE_INFO_FB, 1000117001);
typedef struct XrTriangleMeshCreateInfoFB {
    XrStructureType type;
    const void* XR_MAY_ALIAS next;
    // Behavior flags. Accepted flags include:
    // - XR_TRIANGLE_MESH_MUTABLE_BIT_FB: defines the mesh as mutable (updateable).
    //   Mesh buffers can be updated between `xrTriangleMeshBeginUpdateFB` and
    //   `xrTriangleMeshEndUpdateFB` calls.
    XrTriangleMeshFlagsFB flags;
    // The winding order of the triangles, see `XrWindingOrderFB` for details.
    XrWindingOrderFB windingOrder;
    // Number of vertices in the mesh.
    // In the case of the mutable mesh, the value is treated as the maximum number of vertices
    // the mesh will be able to represent at any time in its lifecycle.
    // The actual number of vertices can vary and is defined in `xrTriangleMeshEndUpdateFB`.
    uint32_t vertexCount;
    // Pointer to the vertex data.
    // Must be null when the mesh is mutable - mesh data must be populated separately.
    // Otherwise must be a valid list of 3D vertex coordinates.
    const XrVector3f* vertexBuffer;
    // Number of triangles in the mesh.
    // In the case of the mutable mesh, the value is treated as the maximum number of maximum number
    // of triangles the mesh will be able to represent at any time in its lifecycle.
    // The actual number of triangles can vary and is defined in
    // `xrTriangleMeshEndUpdateFB`.
    uint32_t triangleCount;
    // Pointer to an array of triangle indices. Each triplet of consecutive elements represents
    // three indices into the vertex buffer. The resulting vertices form a triangle.
    // Must be null when the mesh is mutable - mesh data must be populated separately.
    const uint32_t* indexBuffer;
} XrTriangleMeshCreateInfoFB;

typedef XrResult(XRAPI_PTR* PFN_xrCreateTriangleMeshFB)(
    XrSession session,
    const XrTriangleMeshCreateInfoFB* createInfo,
    XrTriangleMeshFB* outTriangleMesh);
typedef XrResult(XRAPI_PTR* PFN_xrDestroyTriangleMeshFB)(XrTriangleMeshFB mesh);
typedef XrResult(XRAPI_PTR* PFN_xrTriangleMeshGetVertexBufferFB)(
    XrTriangleMeshFB mesh,
    XrVector3f** outVertexBuffer);
typedef XrResult(XRAPI_PTR* PFN_xrTriangleMeshGetIndexBufferFB)(
    XrTriangleMeshFB mesh,
    uint32_t** outIndexBuffer);
typedef XrResult(XRAPI_PTR* PFN_xrTriangleMeshBeginUpdateFB)(XrTriangleMeshFB mesh);
typedef XrResult(XRAPI_PTR* PFN_xrTriangleMeshEndUpdateFB)(
    XrTriangleMeshFB mesh,
    uint32_t vertexCount,
    uint32_t triangleCount);
typedef XrResult(XRAPI_PTR* PFN_xrTriangleMeshBeginVertexBufferUpdateFB)(
    XrTriangleMeshFB mesh,
    uint32_t* outVertexCount);
typedef XrResult(XRAPI_PTR* PFN_xrTriangleMeshEndVertexBufferUpdateFB)(XrTriangleMeshFB mesh);

#ifndef XR_NO_PROTOTYPES
#ifdef XR_EXTENSION_PROTOTYPES

// Create a triangle mesh geometry object.
// Depending on the behavior flags, the mesh could be created immutable (data is assigned
// at creation and cannot be changed) or mutable (the mesh is created empty and can be updated
// by calling begin/end update functions).
XRAPI_ATTR XrResult XRAPI_CALL xrCreateTriangleMeshFB(
    XrSession session,
    const XrTriangleMeshCreateInfoFB* createInfo,
    XrTriangleMeshFB* outTriangleMesh);

// Destroy an `XrTriangleMeshFB` object along with its data. The mesh buffers must not be
// accessed anymore after their parent mesh object has been destroyed.
XRAPI_ATTR XrResult XRAPI_CALL xrDestroyTriangleMeshFB(XrTriangleMeshFB mesh);

// Retrieve a pointer to the vertex buffer. The vertex buffer is structured as an array of 3 floats
// per vertex representing x, y, and z: `[x0, y0, z0, x1, y1, z1, ...]`. The size of the buffer is
// `maxVertexCount * 3` floats. The application must call `xrTriangleMeshBeginUpdateFB` or
// `xrTriangleMeshBeginVertexBufferUpdateFB` before making modifications to the vertex
// buffer. The buffer location is guaranteed to remain constant over the lifecycle of the mesh
// object.
XRAPI_ATTR XrResult XRAPI_CALL
xrTriangleMeshGetVertexBufferFB(XrTriangleMeshFB mesh, XrVector3f** outVertexBuffer);

// Retrieve the index buffer that defines the topology of the triangle mesh. Each triplet of
// consecutive elements point to three vertices in the vertex buffer and thus form a triangle. The
// size of each element is `indexElementSize` bytes, and thus the size of the buffer is
// `maxTriangleCount * 3 * indexElementSize` bytes. The application must call
// `xrTriangleMeshBeginUpdateFB` before making modifications to the index buffer. The buffer
// location is guaranteed to remain constant over the lifecycle of the mesh object.
XRAPI_ATTR XrResult XRAPI_CALL
xrTriangleMeshGetIndexBufferFB(XrTriangleMeshFB mesh, uint32_t** outIndexBuffer);

// Begin updating the mesh buffer data. The application must call this function before it makes any
// modifications to the buffers retrieved by `xrTriangleMeshGetVertexBufferFB` and
// `xrTriangleMeshGetIndexBufferFB`. If only the vertex buffer needs to be updated,
// `xrTriangleMeshBeginVertexBufferUpdateFB` can be used instead. To commit the
// modifications, the application must call `xrTriangleMeshEndUpdateFB`.
XRAPI_ATTR XrResult XRAPI_CALL xrTriangleMeshBeginUpdateFB(XrTriangleMeshFB mesh);

// Signal the API that the application has finished updating the mesh buffers after a call to
// `xrTriangleMeshBeginUpdateFB`. `vertexCount` and `triangleCount` specify the actual
// number of primitives that make up the mesh after the update. They must be larger than zero but
// smaller or equal to the maximum counts defined at create time. Buffer data beyond these counts
// is ignored.
XRAPI_ATTR XrResult XRAPI_CALL
xrTriangleMeshEndUpdateFB(XrTriangleMeshFB mesh, uint32_t vertexCount, uint32_t triangleCount);

// Update the vertex positions of a triangle mesh. Can only be called once the mesh topology has
// been set using `xrTriangleMeshBeginUpdateFB`/`xrTriangleMeshEndUpdateFB`. The
// vertex count is defined by the last invocation to `xrTriangleMeshEndUpdateFB`. Once the
// modification is done, `xrTriangleMeshEndVertexBufferUpdateFB` must be called.
XRAPI_ATTR XrResult XRAPI_CALL
xrTriangleMeshBeginVertexBufferUpdateFB(XrTriangleMeshFB mesh, uint32_t* outVertexCount);

// Signal the API that the contents of the vertex buffer data has been updated
// after a call to `xrTriangleMeshBeginVertexBufferUpdateFB`.
XRAPI_ATTR XrResult XRAPI_CALL xrTriangleMeshEndVertexBufferUpdateFB(XrTriangleMeshFB mesh);

#endif /* XR_EXTENSION_PROTOTYPES */
#endif /* !XR_NO_PROTOTYPES */

#endif // XR_FB_triangle_mesh

#ifdef __cplusplus
}
#endif
