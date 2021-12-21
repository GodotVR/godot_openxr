#ifndef XR_FB_PASSTHROUGH_EXTENSION_WRAPPER_H_
#define XR_FB_PASSTHROUGH_EXTENSION_WRAPPER_H_

#include "openxr/OpenXRApi.h"
#include "openxr/include/openxr_inc.h"
#include "xr_composition_layer_provider.h"
#include "xr_extension_wrapper.h"

#include <map>

#include "openxr/include/util.h"

// Wrapper for the set of Facebook XR passthrough extensions.
class XRFbPassthroughExtensionWrapper : public XRExtensionWrapper, public XRCompositionLayerProvider {
public:
	static XRFbPassthroughExtensionWrapper *get_singleton();

	void on_instance_initialized(const XrInstance instance) override;

	void on_session_initialized(const XrSession session) override;

	void on_session_destroyed() override;

	void on_instance_destroyed() override;

	XrCompositionLayerBaseHeader *get_composition_layer() override;

	bool start_passthrough();

	void stop_passthrough();

protected:
	XRFbPassthroughExtensionWrapper();
	~XRFbPassthroughExtensionWrapper();

private:
	// Create a passthrough feature
	EXT_PROTO_XRRESULT_FUNC3(xrCreatePassthroughFB,
			(XrSession), session,
			(const XrPassthroughCreateInfoFB *), create_info,
			(XrPassthroughFB *), feature_out)

	// Destroy a previously created passthrough feature
	EXT_PROTO_XRRESULT_FUNC1(xrDestroyPassthroughFB, (XrPassthroughFB), feature)

	//*** Passthrough feature state management functions *********
	// Start the passthrough feature
	EXT_PROTO_XRRESULT_FUNC1(xrPassthroughStartFB, (XrPassthroughFB), passthrough)
	// Pause the passthrough feature
	EXT_PROTO_XRRESULT_FUNC1(xrPassthroughPauseFB, (XrPassthroughFB), passthrough)

	EXT_PROTO_XRRESULT_FUNC3(xrCreatePassthroughLayerFB, (XrSession), session,
			(const XrPassthroughLayerCreateInfoFB *), config,
			(XrPassthroughLayerFB *), layer_out)

	EXT_PROTO_XRRESULT_FUNC1(xrDestroyPassthroughLayerFB, (XrPassthroughLayerFB), layer)

	EXT_PROTO_XRRESULT_FUNC1(xrPassthroughLayerPauseFB, (XrPassthroughLayerFB), layer)
	EXT_PROTO_XRRESULT_FUNC1(xrPassthroughLayerResumeFB, (XrPassthroughLayerFB), layer)

	// Set the style of an existing passthrough layer. If the enabled feature set
	// doesn’t change, this is a lightweight operation that can be called in every
	// frame to animate the style. Changes that may incur a bigger cost:
	// - Enabling/disabling the color mapping, or changing the type of mapping
	//   (monochromatic to RGBA or back).
	// - Changing `textureOpacityFactor` from 0 to non-zero or vice versa
	// - Changing `edgeColor[3]` from 0 to non-zero or vice versa
	// NOTE: For XR_FB_passthrough, all color values are treated as linear.
	EXT_PROTO_XRRESULT_FUNC2(xrPassthroughLayerSetStyleFB,
			(XrPassthroughLayerFB), layer,
			(const XrPassthroughStyleFB *), style)

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
	EXT_PROTO_XRRESULT_FUNC3(xrCreateGeometryInstanceFB,
			(XrSession), session,
			(const XrGeometryInstanceCreateInfoFB *), create_info,
			(XrGeometryInstanceFB *), out_geometry_instance)

	// Destroys a previously created geometry instance from passthrough rendering.
	// This removes the geometry instance from passthrough rendering.
	// The operation has no effect on other instances or the underlying mesh.
	EXT_PROTO_XRRESULT_FUNC1(xrDestroyGeometryInstanceFB, (XrGeometryInstanceFB), instance)

	// Update the transformation of a passthrough geometry instance.
	EXT_PROTO_XRRESULT_FUNC2(xrGeometryInstanceSetTransformFB,
			(XrGeometryInstanceFB), instance,
			(const XrGeometryInstanceTransformFB *), transformation)

	// Create a triangle mesh geometry object.
	// Depending on the behavior flags, the mesh could be created immutable (data is assigned
	// at creation and cannot be changed) or mutable (the mesh is created empty and can be updated
	// by calling begin/end update functions).
	EXT_PROTO_XRRESULT_FUNC3(xrCreateTriangleMeshFB,
			(XrSession), session,
			(const XrTriangleMeshCreateInfoFB *), create_info,
			(XrTriangleMeshFB *), out_triangle_mesh)

	// Destroy an `XrTriangleMeshFB` object along with its data. The mesh buffers must not be
	// accessed anymore after their parent mesh object has been destroyed.
	EXT_PROTO_XRRESULT_FUNC1(xrDestroyTriangleMeshFB, (XrTriangleMeshFB), mesh)

	// Retrieve a pointer to the vertex buffer. The vertex buffer is structured as an array of 3 floats
	// per vertex representing x, y, and z: `[x0, y0, z0, x1, y1, z1, ...]`. The size of the buffer is
	// `maxVertexCount * 3` floats. The application must call `xrTriangleMeshBeginUpdateFB` or
	// `xrTriangleMeshBeginVertexBufferUpdateFB` before making modifications to the vertex
	// buffer. The buffer location is guaranteed to remain constant over the lifecycle of the mesh
	// object.
	EXT_PROTO_XRRESULT_FUNC2(xrTriangleMeshGetVertexBufferFB,
			(XrTriangleMeshFB), mesh,
			(XrVector3f **), out_vertex_buffer)

	// Retrieve the index buffer that defines the topology of the triangle mesh. Each triplet of
	// consecutive elements point to three vertices in the vertex buffer and thus form a triangle. The
	// size of each element is `indexElementSize` bytes, and thus the size of the buffer is
	// `maxTriangleCount * 3 * indexElementSize` bytes. The application must call
	// `xrTriangleMeshBeginUpdateFB` before making modifications to the index buffer. The buffer
	// location is guaranteed to remain constant over the lifecycle of the mesh object.
	EXT_PROTO_XRRESULT_FUNC2(xrTriangleMeshGetIndexBufferFB,
			(XrTriangleMeshFB), mesh,
			(uint32_t **), out_index_buffer)

	// Begin updating the mesh buffer data. The application must call this function before it makes any
	// modifications to the buffers retrieved by `xrTriangleMeshGetVertexBufferFB` and
	// `xrTriangleMeshGetIndexBufferFB`. If only the vertex buffer needs to be updated,
	// `xrTriangleMeshBeginVertexBufferUpdateFB` can be used instead. To commit the
	// modifications, the application must call `xrTriangleMeshEndUpdateFB`.
	EXT_PROTO_XRRESULT_FUNC1(xrTriangleMeshBeginUpdateFB, (XrTriangleMeshFB), mesh)

	// Signal the API that the application has finished updating the mesh buffers after a call to
	// `xrTriangleMeshBeginUpdateFB`. `vertexCount` and `triangleCount` specify the actual
	// number of primitives that make up the mesh after the update. They must be larger than zero but
	// smaller or equal to the maximum counts defined at create time. Buffer data beyond these counts
	// is ignored.
	EXT_PROTO_XRRESULT_FUNC3(xrTriangleMeshEndUpdateFB,
			(XrTriangleMeshFB), mesh,
			(uint32_t), vertexCount,
			(uint32_t), triangle_count)

	// Update the vertex positions of a triangle mesh. Can only be called once the mesh topology has
	// been set using `xrTriangleMeshBeginUpdateFB`/`xrTriangleMeshEndUpdateFB`. The
	// vertex count is defined by the last invocation to `xrTriangleMeshEndUpdateFB`. Once the
	// modification is done, `xrTriangleMeshEndVertexBufferUpdateFB` must be called.
	EXT_PROTO_XRRESULT_FUNC2(xrTriangleMeshBeginVertexBufferUpdateFB,
			(XrTriangleMeshFB), mesh,
			(uint32_t *), out_vertex_count)

	// Signal the API that the contents of the vertex buffer data has been updated
	// after a call to `xrTriangleMeshBeginVertexBufferUpdateFB`.
	EXT_PROTO_XRRESULT_FUNC1(xrTriangleMeshEndVertexBufferUpdateFB, (XrTriangleMeshFB), mesh)

	XrResult initialize_fb_passthrough_extension(XrInstance instance);

	XrResult initialize_fb_triangle_mesh_extension(XrInstance instance);

	void cleanup();

	bool is_passthrough_valid();

	// TODO: Temporary workaround (https://github.com/GodotVR/godot_openxr/issues/138)
	//  Address a bug in the passthrough api where XR_ERROR_UNEXPECTED_STATE_PASSTHROUGH_FB is
	//  returned even when the operation is valid on Meta Quest devices.
	//  The issue should be addressed on that platform in OS release v37.
	inline bool is_valid_passthrough_result(XrResult result, const char *format) {
		return openxr_api->xr_result(result, format) || result == XR_ERROR_UNEXPECTED_STATE_PASSTHROUGH_FB;
	}

	bool is_composition_passthrough_layer_ready();

	static XRFbPassthroughExtensionWrapper *singleton;

	OpenXRApi *openxr_api = nullptr;
	bool fb_passthrough_ext = false; // required for any passthrough functionality
	bool fb_triangle_mesh_ext = false; // only use for projected passthrough

	XrPassthroughCreateInfoFB passthrough_create_info = {
		.type = XR_TYPE_PASSTHROUGH_CREATE_INFO_FB,
		.next = nullptr,
	};
	XrPassthroughFB passthrough_handle = XR_NULL_HANDLE;

	XrPassthroughLayerCreateInfoFB passthrough_layer_config = {
		.type = XR_TYPE_PASSTHROUGH_LAYER_CREATE_INFO_FB,
		.next = nullptr,
		.passthrough = passthrough_handle,
		.flags = XR_PASSTHROUGH_IS_RUNNING_AT_CREATION_BIT_FB,
		.purpose = XR_PASSTHROUGH_LAYER_PURPOSE_RECONSTRUCTION_FB,
	};
	XrPassthroughStyleFB passthrough_layer_style = {
		.type = XR_TYPE_PASSTHROUGH_STYLE_FB,
		.next = nullptr,
		.textureOpacityFactor = 1,
		.edgeColor = { 0, 0, 0, 0 },
	};
	XrPassthroughLayerFB passthrough_layer = XR_NULL_HANDLE;

	XrCompositionLayerPassthroughFB composition_passthrough_layer = {
		.type = XR_TYPE_COMPOSITION_LAYER_PASSTHROUGH_FB,
		.flags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT,
	};
};

#endif // XR_FB_PASSTHROUGH_EXTENSION_WRAPPER_H_
