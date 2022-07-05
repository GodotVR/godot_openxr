#include "xr_ext_palm_pose_extension_wrapper.h"

#include <core/Variant.hpp>

using namespace godot;

XRExtPalmPoseExtensionWrapper *XRExtPalmPoseExtensionWrapper::singleton = nullptr;

XRExtPalmPoseExtensionWrapper *XRExtPalmPoseExtensionWrapper::get_singleton() {
	if (!singleton) {
		singleton = new XRExtPalmPoseExtensionWrapper();
	}

	return singleton;
}

XRExtPalmPoseExtensionWrapper::XRExtPalmPoseExtensionWrapper() {
	openxr_api = OpenXRApi::openxr_get_api();
	request_extensions[XR_EXT_PALM_POSE_EXTENSION_NAME] = &palm_pose_ext;
}

XRExtPalmPoseExtensionWrapper::~XRExtPalmPoseExtensionWrapper() {
	OpenXRApi::openxr_release_api();
}

bool XRExtPalmPoseExtensionWrapper::path_is_supported(const String &p_io_path) {
	if (p_io_path == "/user/hand/left/input/palm_ext/pose") {
		return palm_pose_ext;
	}

	if (p_io_path == "/user/hand/right/input/palm_ext/pose") {
		return palm_pose_ext;
	}

	// Not a path under this extensions control, so we return true;
	return true;
}
