#ifndef XR_FB_SWAPCHAIN_UPDATE_STATE_EXTENSION_WRAPPER_H
#define XR_FB_SWAPCHAIN_UPDATE_STATE_EXTENSION_WRAPPER_H

#include "openxr/OpenXRApi.h"
#include "openxr/extensions/xr_extension_wrapper.h"
#include "openxr/include/util.h"

// Wrapper for the XR_FB_swapchain_update_state_extension* extensions.
class XRFbSwapchainUpdateStateExtensionWrapper : public XRExtensionWrapper {
public:
	static XRFbSwapchainUpdateStateExtensionWrapper *get_singleton();

	void on_instance_initialized(const XrInstance instance) override;

	void on_instance_destroyed() override;

	bool is_enabled() {
		return fb_swapchain_update_state_ext;
	}

	EXT_PROTO_XRRESULT_FUNC2(xrUpdateSwapchainFB,
			(XrSwapchain), swapchain,
			(const XrSwapchainStateBaseHeaderFB *), state);

	EXT_PROTO_XRRESULT_FUNC2(xrGetSwapchainStateFB,
			(XrSwapchain), swapchain,
			(XrSwapchainStateBaseHeaderFB *), state);

protected:
	XRFbSwapchainUpdateStateExtensionWrapper();
	~XRFbSwapchainUpdateStateExtensionWrapper();

private:
	XrResult initialize_fb_swapchain_update_state_extension(XrInstance instance);

	void cleanup();

	static XRFbSwapchainUpdateStateExtensionWrapper *singleton;

	OpenXRApi *openxr_api = nullptr;
	bool fb_swapchain_update_state_ext = false;
#ifdef XR_USE_GRAPHICS_API_OPENGL_ES
	bool fb_swapchain_update_state_opengles_ext = false;
#endif
};

#endif // XR_FB_SWAPCHAIN_UPDATE_STATE_EXTENSION_WRAPPER_H
