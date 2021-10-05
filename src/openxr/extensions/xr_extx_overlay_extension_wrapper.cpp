#include "xr_extx_overlay_extension_wrapper.h"

#include <core/Variant.hpp>

using namespace godot;

XRExtxOverlayExtensionWrapper *XRExtxOverlayExtensionWrapper::singleton = nullptr;

XRExtxOverlayExtensionWrapper *XRExtxOverlayExtensionWrapper::get_singleton() {
	if (!singleton) {
		singleton = new XRExtxOverlayExtensionWrapper();
	}

	return singleton;
}

XRExtxOverlayExtensionWrapper::XRExtxOverlayExtensionWrapper() {
	openxr_api = OpenXRApi::openxr_get_api();
	request_extensions[XR_EXTX_OVERLAY_EXTENSION_NAME] = &extx_overlay_ext;
}

XRExtxOverlayExtensionWrapper::~XRExtxOverlayExtensionWrapper() {
	cleanup();
	OpenXRApi::openxr_release_api();
}

void XRExtxOverlayExtensionWrapper::cleanup() {
	extx_overlay_ext = false;
}

void XRExtxOverlayExtensionWrapper::on_instance_initialized(const XrInstance instance) {
	if (extx_overlay_ext) {
		XrResult result = initialise_extx_overlay_extension(instance);
		if (!openxr_api->xr_result(result, "Failed to initialise overlay extension")) {
			extx_overlay_ext = false; // I guess we don't support it...
		}
	}
}

void XRExtxOverlayExtensionWrapper::on_instance_destroyed() {
	cleanup();
}

XrResult XRExtxOverlayExtensionWrapper::initialise_extx_overlay_extension(XrInstance instance) {
	return XR_SUCCESS;
}
