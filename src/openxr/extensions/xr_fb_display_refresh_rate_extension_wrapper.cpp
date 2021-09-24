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
		XrResult result = initialise_fb_display_refresh_rate_extension(instance);
		if (!openxr_api->xr_result(result, "Failed to initialise display refresh rate extension")) {
			fb_display_refresh_rate_ext = false; // I guess we don't support it...
		}
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

PFN_xrEnumerateDisplayRefreshRatesFB xrEnumerateDisplayRefreshRatesFB_ptr = nullptr;

XRAPI_ATTR XrResult XRAPI_CALL XRFbDisplayRefreshRateExtensionWrapper::xrEnumerateDisplayRefreshRatesFB(
		XrSession session,
		uint32_t displayRefreshRateCapacityInput,
		uint32_t *displayRefreshRateCountOutput,
		float *displayRefreshRates) {
	if (xrEnumerateDisplayRefreshRatesFB_ptr == nullptr) {
		return XR_ERROR_HANDLE_INVALID;
	}

	return (*xrEnumerateDisplayRefreshRatesFB_ptr)(session, displayRefreshRateCapacityInput, displayRefreshRateCountOutput, displayRefreshRates);
}

PFN_xrGetDisplayRefreshRateFB xrGetDisplayRefreshRateFB_ptr = nullptr;

XRAPI_ATTR XrResult XRAPI_CALL XRFbDisplayRefreshRateExtensionWrapper::xrGetDisplayRefreshRateFB(
		XrSession session,
		float *displayRefreshRate) {
	if (xrGetDisplayRefreshRateFB_ptr == nullptr) {
		return XR_ERROR_HANDLE_INVALID;
	}

	return (*xrGetDisplayRefreshRateFB_ptr)(session, displayRefreshRate);
}

PFN_xrRequestDisplayRefreshRateFB xrRequestDisplayRefreshRateFB_ptr = nullptr;

XRAPI_ATTR XrResult XRAPI_CALL XRFbDisplayRefreshRateExtensionWrapper::xrRequestDisplayRefreshRateFB(
		XrSession session,
		float displayRefreshRate) {
	if (xrRequestDisplayRefreshRateFB_ptr == nullptr) {
		return XR_ERROR_HANDLE_INVALID;
	}

	return (*xrRequestDisplayRefreshRateFB_ptr)(session, displayRefreshRate);
}

XrResult XRFbDisplayRefreshRateExtensionWrapper::initialise_fb_display_refresh_rate_extension(XrInstance instance) {
	XrResult result;

	result = xrGetInstanceProcAddr(instance, "xrEnumerateDisplayRefreshRatesFB", (PFN_xrVoidFunction *)&xrEnumerateDisplayRefreshRatesFB_ptr);
	if (result != XR_SUCCESS) {
		return result;
	}
	result = xrGetInstanceProcAddr(instance, "xrGetDisplayRefreshRateFB", (PFN_xrVoidFunction *)&xrGetDisplayRefreshRateFB_ptr);
	if (result != XR_SUCCESS) {
		return result;
	}
	result = xrGetInstanceProcAddr(instance, "xrRequestDisplayRefreshRateFB", (PFN_xrVoidFunction *)&xrRequestDisplayRefreshRateFB_ptr);
	if (result != XR_SUCCESS) {
		return result;
	}

	return XR_SUCCESS;
}
