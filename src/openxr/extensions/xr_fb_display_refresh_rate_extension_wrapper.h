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
	void cleanup();

	static XRFbDisplayRefreshRateExtensionWrapper *singleton;

	OpenXRApi *openxr_api = nullptr;
	bool fb_display_refresh_rate_ext = false;

	// OpenXR API call wrappers
	EXT_PROTO_XRRESULT_FUNC4(xrEnumerateDisplayRefreshRatesFB, (XrSession), session, (uint32_t), displayRefreshRateCapacityInput, (uint32_t *), displayRefreshRateCountOutput, (float *), displayRefreshRates);
	EXT_PROTO_XRRESULT_FUNC2(xrGetDisplayRefreshRateFB, (XrSession), session, (float *), display_refresh_rate);
	EXT_PROTO_XRRESULT_FUNC2(xrRequestDisplayRefreshRateFB, (XrSession), session, (float), display_refresh_rate);
};

#endif // XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_WRAPPER_H
