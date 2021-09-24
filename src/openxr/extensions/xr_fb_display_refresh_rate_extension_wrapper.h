#ifndef XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_WRAPPER_H
#define XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_WRAPPER_H

#include "openxr/OpenXRApi.h"
#include "openxr/include/openxr_inc.h"
#include "xr_extension_wrapper.h"

#include <map>

// Wrapper for the XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME extension.
class XRFbDisplayRefreshRateExtensionWrapper : public XRExtensionWrapper {
public:
	static XRFbDisplayRefreshRateExtensionWrapper *get_singleton();

	void on_instance_initialized(const XrInstance instance) override;

	void on_instance_destroyed() override;

	double get_refresh_rate() const;

	void set_refresh_rate(const double p_refresh_rate);

	godot::Array get_available_refresh_rates() const;

protected:
	XRFbDisplayRefreshRateExtensionWrapper();
	~XRFbDisplayRefreshRateExtensionWrapper();

private:
	static XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateDisplayRefreshRatesFB(
			XrSession session,
			uint32_t displayRefreshRateCapacityInput,
			uint32_t *displayRefreshRateCountOutput,
			float *displayRefreshRates);

	static XRAPI_ATTR XrResult XRAPI_CALL xrGetDisplayRefreshRateFB(
			XrSession session,
			float *displayRefreshRate);

	static XRAPI_ATTR XrResult XRAPI_CALL xrRequestDisplayRefreshRateFB(
			XrSession session,
			float displayRefreshRate);

	static XrResult initialise_fb_display_refresh_rate_extension(XrInstance instance);

	void cleanup();

	static XRFbDisplayRefreshRateExtensionWrapper *singleton;

	OpenXRApi *openxr_api = nullptr;
	bool fb_display_refresh_rate_ext = false;
};

#endif // XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_WRAPPER_H
