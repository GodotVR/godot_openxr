#include "xr_fb_swapchain_update_state_extension_wrapper.h"

XRFbSwapchainUpdateStateExtensionWrapper *XRFbSwapchainUpdateStateExtensionWrapper::singleton = nullptr;

XRFbSwapchainUpdateStateExtensionWrapper *XRFbSwapchainUpdateStateExtensionWrapper::get_singleton() {
	if (!singleton) {
		singleton = new XRFbSwapchainUpdateStateExtensionWrapper();
	}
	return singleton;
}

XRFbSwapchainUpdateStateExtensionWrapper::XRFbSwapchainUpdateStateExtensionWrapper() {
	openxr_api = OpenXRApi::openxr_get_api();
	// These might be FB extensions but other vendors may implement them in due time as well.
	request_extensions[XR_FB_SWAPCHAIN_UPDATE_STATE_EXTENSION_NAME] = &fb_swapchain_update_state_ext;
#ifdef XR_USE_GRAPHICS_API_OPENGL_ES
	request_extensions[XR_FB_SWAPCHAIN_UPDATE_STATE_OPENGL_ES_EXTENSION_NAME] = &fb_swapchain_update_state_opengles_ext;
#endif
}

XRFbSwapchainUpdateStateExtensionWrapper::~XRFbSwapchainUpdateStateExtensionWrapper() {
	cleanup();
	OpenXRApi::openxr_release_api();
}

void XRFbSwapchainUpdateStateExtensionWrapper::cleanup() {
	fb_swapchain_update_state_ext = false;
#ifdef XR_USE_GRAPHICS_API_OPENGL_ES
	fb_swapchain_update_state_opengles_ext = false;
#endif
}

void XRFbSwapchainUpdateStateExtensionWrapper::on_instance_initialized(const XrInstance instance) {
	if (fb_swapchain_update_state_ext) {
		EXT_INIT_XR_FUNC(xrUpdateSwapchainFB);
		EXT_INIT_XR_FUNC(xrGetSwapchainStateFB);
	}
}

void XRFbSwapchainUpdateStateExtensionWrapper::on_instance_destroyed() {
	cleanup();
}
