#include "xr_fb_foveation_extension_wrapper.h"

XRFbFoveationExtensionWrapper *XRFbFoveationExtensionWrapper::singleton = nullptr;

XRFbFoveationExtensionWrapper *XRFbFoveationExtensionWrapper::get_singleton() {
	if (!singleton) {
		singleton = new XRFbFoveationExtensionWrapper();
	}
	return singleton;
}

XRFbFoveationExtensionWrapper::XRFbFoveationExtensionWrapper() {
	openxr_api = OpenXRApi::openxr_get_api();
	swapchain_update_state_wrapper = XRFbSwapchainUpdateStateExtensionWrapper::get_singleton();
	request_extensions[XR_FB_FOVEATION_EXTENSION_NAME] = &fb_foveation_ext;
	request_extensions[XR_FB_FOVEATION_CONFIGURATION_EXTENSION_NAME] = &fb_foveation_configuration_ext;
}

XRFbFoveationExtensionWrapper::~XRFbFoveationExtensionWrapper() {
	cleanup();
	OpenXRApi::openxr_release_api();
}

void XRFbFoveationExtensionWrapper::cleanup() {
	fb_foveation_ext = false;
	fb_foveation_configuration_ext = false;
}

bool XRFbFoveationExtensionWrapper::is_enabled() {
	return swapchain_update_state_wrapper != nullptr && swapchain_update_state_wrapper->is_enabled() && fb_foveation_ext && fb_foveation_configuration_ext;
}

XrResult XRFbFoveationExtensionWrapper::initialize_fb_foveation_extension(XrInstance instance) {
	std::map<const char *, PFN_xrVoidFunction *> func_pointer_map;
	LOAD_FUNC_POINTER_IN_MAP(func_pointer_map, xrCreateFoveationProfileFB);
	LOAD_FUNC_POINTER_IN_MAP(func_pointer_map, xrDestroyFoveationProfileFB);

	return initialize_function_pointer_map(instance, func_pointer_map);
}

void XRFbFoveationExtensionWrapper::on_instance_initialized(const XrInstance instance) {
	if (is_enabled()) {
		XrResult result = initialize_fb_foveation_extension(instance);
		if (!openxr_api->xr_result(result, "Failed to initialize foveation extension")) {
			fb_foveation_ext = false;
			return;
		}
	}
}

void XRFbFoveationExtensionWrapper::on_instance_destroyed() {
	cleanup();
}

void **XRFbFoveationExtensionWrapper::set_swapchain_create_info_and_get_next_pointer(void **swapchain_create_info) {
	if (is_enabled()) {
		*swapchain_create_info = &swapchain_create_info_foveation_fb;
		return &swapchain_create_info_foveation_fb.next;
	} else {
		return nullptr;
	}
}

void XRFbFoveationExtensionWrapper::set_foveation_level(XrFoveationLevelFB level,
		XrFoveationDynamicFB dynamic) {
	if (!is_enabled()) {
		return;
	}

	for (uint32_t eye = 0; eye < openxr_api->get_view_count(); eye++) {
		XrFoveationLevelProfileCreateInfoFB level_profile_create_info = {
			.type = XR_TYPE_FOVEATION_LEVEL_PROFILE_CREATE_INFO_FB,
			.next = nullptr,
			.level = level,
			.verticalOffset = 0.0f,
			.dynamic = dynamic,
		};

		XrFoveationProfileCreateInfoFB profile_create_info = {
			.type = XR_TYPE_FOVEATION_PROFILE_CREATE_INFO_FB,
			.next = &level_profile_create_info,
		};

		XrFoveationProfileFB foveation_profile;
		XrResult result = xrCreateFoveationProfileFB(openxr_api->get_session(), &profile_create_info, &foveation_profile);
		if (!openxr_api->xr_result(result, "Unable to create the foveation profile for eye {0}", eye)) {
			return;
		}

		XrSwapchainStateFoveationFB foveation_update_state = {
			.type = XR_TYPE_SWAPCHAIN_STATE_FOVEATION_FB,
			.profile = foveation_profile,
		};

		result = swapchain_update_state_wrapper->xrUpdateSwapchainFB(openxr_api->get_swapchain(eye), (XrSwapchainStateBaseHeaderFB *)&foveation_update_state);
		if (!openxr_api->xr_result(result, "Unable to update the swapchain for eye {0}", eye)) {
			return;
		}

		result = xrDestroyFoveationProfileFB(foveation_profile);
		openxr_api->xr_result(result, "Unable to destroy the foveation profile for eye {0}", eye);
	}
}
