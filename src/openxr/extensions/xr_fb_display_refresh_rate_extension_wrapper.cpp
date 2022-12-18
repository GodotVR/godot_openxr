#include "xr_fb_display_refresh_rate_extension_wrapper.h"

#include <core/Variant.hpp>

using namespace godot;

XRFbDisplayRefreshRateExtensionWrapper *XRFbDisplayRefreshRateExtensionWrapper::singleton = nullptr;

XRFbDisplayRefreshRateExtensionWrapper *XRFbDisplayRefreshRateExtensionWrapper::get_singleton() {
	if (!singleton) {
		singleton = new XRFbDisplayRefreshRateExtensionWrapper();
	}

	return singleton;
}

XRFbDisplayRefreshRateExtensionWrapper::XRFbDisplayRefreshRateExtensionWrapper() {
	openxr_api = OpenXRApi::openxr_get_api();
	request_extensions[XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME] = &fb_display_refresh_rate_ext;
}

XRFbDisplayRefreshRateExtensionWrapper::~XRFbDisplayRefreshRateExtensionWrapper() {
	cleanup();
	OpenXRApi::openxr_release_api();
}

void XRFbDisplayRefreshRateExtensionWrapper::cleanup() {
	fb_display_refresh_rate_ext = false;
}

void XRFbDisplayRefreshRateExtensionWrapper::on_instance_initialized(const XrInstance instance) {
	if (fb_display_refresh_rate_ext) {
		EXT_INIT_XR_FUNC(xrEnumerateDisplayRefreshRatesFB);
		EXT_INIT_XR_FUNC(xrGetDisplayRefreshRateFB);
		EXT_INIT_XR_FUNC(xrRequestDisplayRefreshRateFB);
	}
}

void XRFbDisplayRefreshRateExtensionWrapper::on_instance_destroyed() {
	cleanup();
}

double XRFbDisplayRefreshRateExtensionWrapper::get_refresh_rate() const {
	double refresh_rate = 0.0;

	// Currently only supported through FB's display refresh rate extension
	if (fb_display_refresh_rate_ext) {
		float rate;
		XrResult result = xrGetDisplayRefreshRateFB(openxr_api->get_session(), &rate);
		if (!openxr_api->xr_result(result, "Failed to obtain refresh rate")) {
			return 0.0;
		}
		refresh_rate = rate;
	}
	return refresh_rate;
}

void XRFbDisplayRefreshRateExtensionWrapper::set_refresh_rate(const double p_refresh_rate) {
	// Currently only supported through FB's display refresh rate extension
	if (fb_display_refresh_rate_ext) {
		XrResult result = xrRequestDisplayRefreshRateFB(openxr_api->get_session(), p_refresh_rate);
		if (!openxr_api->xr_result(result, "Failed to set refresh rate")) {
			return;
		}
	}
}

godot::Array XRFbDisplayRefreshRateExtensionWrapper::get_available_refresh_rates() const {
	godot::Array arr;
	XrResult result;

	// Currently only supported through FB's display refresh rate extension
	if (fb_display_refresh_rate_ext) {
		uint32_t display_refresh_rate_count;

		// figure out how many entries we have...
		result = xrEnumerateDisplayRefreshRatesFB(openxr_api->get_session(), 0, &display_refresh_rate_count, nullptr);
		if (!openxr_api->xr_result(result, "Failed to obtain refresh rate count")) {
			return arr;
		}

		if (display_refresh_rate_count > 0) {
			float *display_refresh_rates = (float *)malloc(sizeof(float) * display_refresh_rate_count);
			if (display_refresh_rates == nullptr) {
				return arr;
			}

			result = xrEnumerateDisplayRefreshRatesFB(openxr_api->get_session(), display_refresh_rate_count, &display_refresh_rate_count, display_refresh_rates);
			if (!openxr_api->xr_result(result, "Failed to obtain refresh rate count")) {
				free(display_refresh_rates);
				return arr;
			}
			for (int i = 0; i < display_refresh_rate_count; i++) {
				// and add to our rate array as a double
				double refresh_rate = display_refresh_rates[i];
				arr.push_back(Variant(refresh_rate));
			}

			free(display_refresh_rates);
		}
	}

	return arr;
}
