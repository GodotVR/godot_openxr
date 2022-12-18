#include "xr_fb_passthrough_extension_wrapper.h"

#include <gen/Viewport.hpp>

using namespace godot;

XRFbPassthroughExtensionWrapper *XRFbPassthroughExtensionWrapper::singleton = nullptr;

XRFbPassthroughExtensionWrapper *XRFbPassthroughExtensionWrapper::get_singleton() {
	if (!singleton) {
		singleton = new XRFbPassthroughExtensionWrapper();
	}

	return singleton;
}

XRFbPassthroughExtensionWrapper::XRFbPassthroughExtensionWrapper() {
	openxr_api = OpenXRApi::openxr_get_api();
	request_extensions[XR_FB_PASSTHROUGH_EXTENSION_NAME] = &fb_passthrough_ext;
	request_extensions[XR_FB_TRIANGLE_MESH_EXTENSION_NAME] = &fb_triangle_mesh_ext;
}

XRFbPassthroughExtensionWrapper::~XRFbPassthroughExtensionWrapper() {
	cleanup();
	OpenXRApi::openxr_release_api();
}

void XRFbPassthroughExtensionWrapper::cleanup() {
	fb_passthrough_ext = false;
	fb_triangle_mesh_ext = false;
}

void XRFbPassthroughExtensionWrapper::on_instance_initialized(const XrInstance instance) {
	if (fb_passthrough_ext) {
#ifdef DEBUG
		Godot::print("OpenXR: Initializing passthrough extension...");
#endif
		bool result = initialize_fb_passthrough_extension(instance);
		if (!result) {
			Godot::print("Failed to initialize fb_passthrough extension");
			fb_passthrough_ext = false;
		}
	}

	if (fb_triangle_mesh_ext) {
		bool result = initialize_fb_triangle_mesh_extension(instance);
		if (!result) {
			Godot::print("Failed to initialize fb_triangle_mesh extension");
			fb_triangle_mesh_ext = false;
		}
	}

	if (fb_passthrough_ext) {
#ifdef DEBUG
		Godot::print("OpenXR: Registering as composition layer provider...");
#endif
		openxr_api->register_composition_layer_provider(this);
	}
}

bool XRFbPassthroughExtensionWrapper::is_passthrough_enabled() {
	return fb_passthrough_ext && passthrough_handle != XR_NULL_HANDLE && passthrough_layer != XR_NULL_HANDLE;
}

bool XRFbPassthroughExtensionWrapper::is_composition_passthrough_layer_ready() {
	return fb_passthrough_ext && passthrough_handle != XR_NULL_HANDLE && composition_passthrough_layer.layerHandle != XR_NULL_HANDLE;
}

bool XRFbPassthroughExtensionWrapper::start_passthrough() {
	if (passthrough_handle == XR_NULL_HANDLE) {
		return false;
	}

	if (is_passthrough_enabled()) {
		return true;
	}

	// Start the passthrough feature
#ifdef DEBUG
	Godot::print("OpenXR: Starting passthrough feature...");
#endif
	XrResult result = xrPassthroughStartFB(passthrough_handle);
	if (!is_valid_passthrough_result(result, "Failed to start passthrough")) {
		stop_passthrough();
		return false;
	}

	// Create the passthrough layer
#ifdef DEBUG
	Godot::print("OpenXR: Creating passthrough layer...");
#endif
	result = xrCreatePassthroughLayerFB(openxr_api->get_session(), &passthrough_layer_config, &passthrough_layer);
	if (!is_valid_passthrough_result(result, "Failed to create the passthrough layer")) {
		stop_passthrough();
		return false;
	}

	// Check if the the viewport has transparent background
	Viewport *viewport = get_main_viewport();
	if (viewport && !viewport->has_transparent_background()) {
		Godot::print_warning("Main viewport doesn't have transparent background! Passthrough may not properly render.", __FUNCTION__, __FILE__, __LINE__);
	}

	composition_passthrough_layer.layerHandle = passthrough_layer;

	return true;
}

void XRFbPassthroughExtensionWrapper::on_session_initialized(const XrSession session) {
	if (fb_passthrough_ext) {
		// Create the passthrough feature and start it.
#ifdef DEBUG
		Godot::print("OpenXR: Creating passthrough feature...");
#endif
		XrResult result = xrCreatePassthroughFB(openxr_api->get_session(), &passthrough_create_info, &passthrough_handle);
		if (!openxr_api->xr_result(result, "Failed to create passthrough")) {
			passthrough_handle = XR_NULL_HANDLE;
			return;
		}
	}
}

XrCompositionLayerBaseHeader *XRFbPassthroughExtensionWrapper::get_composition_layer() {
	if (is_composition_passthrough_layer_ready()) {
		return (XrCompositionLayerBaseHeader *)&composition_passthrough_layer;
	} else {
		return nullptr;
	}
}

void XRFbPassthroughExtensionWrapper::stop_passthrough() {
	if (!fb_passthrough_ext) {
		return;
	}

	composition_passthrough_layer.layerHandle = XR_NULL_HANDLE;

	XrResult result;
	if (passthrough_layer != XR_NULL_HANDLE) {
		// Destroy the layer
#ifdef DEBUG
		Godot::print("OpenXR: Destroying passthrough layer...");
#endif
		result = xrDestroyPassthroughLayerFB(passthrough_layer);
		openxr_api->xr_result(result, "Unable to destroy passthrough layer");
		passthrough_layer = XR_NULL_HANDLE;
	}

	if (passthrough_handle != XR_NULL_HANDLE) {
#ifdef DEBUG
		Godot::print("OpenXR: Stopping passthrough feature...");
#endif
		result = xrPassthroughPauseFB(passthrough_handle);
		openxr_api->xr_result(result, "Unable to stop passthrough feature");
	}
}

void XRFbPassthroughExtensionWrapper::on_session_destroyed() {
	if (fb_passthrough_ext) {
		stop_passthrough();

		XrResult result;
		if (passthrough_handle != XR_NULL_HANDLE) {
#ifdef DEBUG
			Godot::print("OpenXR: Destroying passthrough feature...");
#endif
			result = xrDestroyPassthroughFB(passthrough_handle);
			openxr_api->xr_result(result, "Unable to destroy passthrough feature");
			passthrough_handle = XR_NULL_HANDLE;
		}
	}
}

void XRFbPassthroughExtensionWrapper::on_instance_destroyed() {
	if (fb_passthrough_ext) {
		openxr_api->unregister_composition_layer_provider(this);
	}
	cleanup();
}

bool XRFbPassthroughExtensionWrapper::initialize_fb_passthrough_extension(const XrInstance p_instance) {
	ERR_FAIL_NULL_V(openxr_api, false);

	EXT_INIT_XR_FUNC_V(xrCreatePassthroughFB);
	EXT_INIT_XR_FUNC_V(xrDestroyPassthroughFB);
	EXT_INIT_XR_FUNC_V(xrPassthroughStartFB);
	EXT_INIT_XR_FUNC_V(xrPassthroughPauseFB);
	EXT_INIT_XR_FUNC_V(xrCreatePassthroughLayerFB);
	EXT_INIT_XR_FUNC_V(xrDestroyPassthroughLayerFB);
	EXT_INIT_XR_FUNC_V(xrPassthroughLayerPauseFB);
	EXT_INIT_XR_FUNC_V(xrPassthroughLayerResumeFB);
	EXT_INIT_XR_FUNC_V(xrPassthroughLayerSetStyleFB);
	EXT_INIT_XR_FUNC_V(xrCreateGeometryInstanceFB);
	EXT_INIT_XR_FUNC_V(xrDestroyGeometryInstanceFB);
	EXT_INIT_XR_FUNC_V(xrGeometryInstanceSetTransformFB);

	return true;
}

bool XRFbPassthroughExtensionWrapper::initialize_fb_triangle_mesh_extension(const XrInstance p_instance) {
	ERR_FAIL_NULL_V(openxr_api, false);

	EXT_INIT_XR_FUNC_V(xrCreateTriangleMeshFB);
	EXT_INIT_XR_FUNC_V(xrDestroyTriangleMeshFB);
	EXT_INIT_XR_FUNC_V(xrTriangleMeshGetVertexBufferFB);
	EXT_INIT_XR_FUNC_V(xrTriangleMeshGetIndexBufferFB);
	EXT_INIT_XR_FUNC_V(xrTriangleMeshBeginUpdateFB);
	EXT_INIT_XR_FUNC_V(xrTriangleMeshEndUpdateFB);
	EXT_INIT_XR_FUNC_V(xrTriangleMeshBeginVertexBufferUpdateFB);
	EXT_INIT_XR_FUNC_V(xrTriangleMeshEndVertexBufferUpdateFB);

	return true;
}
