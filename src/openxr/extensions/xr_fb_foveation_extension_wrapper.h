#ifndef XR_FB_FOVEATION_EXTENSION_WRAPPER_H
#define XR_FB_FOVEATION_EXTENSION_WRAPPER_H

#include "openxr/OpenXRApi.h"
#include "openxr/extensions/xr_extension_wrapper.h"
#include "openxr/extensions/xr_fb_swapchain_update_state_extension_wrapper.h"
#include "openxr/include/util.h"

// Wrapper for the XR_FB_foveation_extension* extensions.
class XRFbFoveationExtensionWrapper : public XRExtensionWrapper {
public:
	static XRFbFoveationExtensionWrapper *get_singleton();

	void on_instance_initialized(const XrInstance instance) override;

	void on_instance_destroyed() override;

	void set_foveation_level(XrFoveationLevelFB level, XrFoveationDynamicFB dynamic);

	void **set_swapchain_create_info_and_get_next_pointer(void **swapchain_create_info) override;

protected:
	XRFbFoveationExtensionWrapper();
	~XRFbFoveationExtensionWrapper();

private:
	EXT_PROTO_XRRESULT_FUNC3(xrCreateFoveationProfileFB,
			(XrSession), session,
			(const XrFoveationProfileCreateInfoFB *), create_info,
			(XrFoveationProfileFB *), profile);

	EXT_PROTO_XRRESULT_FUNC1(xrDestroyFoveationProfileFB, (XrFoveationProfileFB), profile);

	XrResult initialize_fb_foveation_extension(XrInstance instance);

	void cleanup();

	bool is_enabled();

	static XRFbFoveationExtensionWrapper *singleton;

	// Enable foveation on this swapchain
	XrSwapchainCreateInfoFoveationFB swapchain_create_info_foveation_fb = {
		.type = XR_TYPE_SWAPCHAIN_CREATE_INFO_FOVEATION_FB
	};

	OpenXRApi *openxr_api = nullptr;
	XRFbSwapchainUpdateStateExtensionWrapper *swapchain_update_state_wrapper = nullptr;
	bool fb_foveation_ext = false;
	bool fb_foveation_configuration_ext = false;
};

#endif // XR_FB_FOVEATION_EXTENSION_WRAPPER_H
