#include "xr_fb_display_refresh_rate_extension.h"

PFN_xrEnumerateDisplayRefreshRatesFB xrEnumerateDisplayRefreshRatesFB_ptr = nullptr;

XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateDisplayRefreshRatesFB(
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

XRAPI_ATTR XrResult XRAPI_CALL xrGetDisplayRefreshRateFB(
		XrSession session,
		float *displayRefreshRate) {
	if (xrGetDisplayRefreshRateFB_ptr == nullptr) {
		return XR_ERROR_HANDLE_INVALID;
	}

	return (*xrGetDisplayRefreshRateFB_ptr)(session, displayRefreshRate);
}

PFN_xrRequestDisplayRefreshRateFB xrRequestDisplayRefreshRateFB_ptr = nullptr;

XRAPI_ATTR XrResult XRAPI_CALL xrRequestDisplayRefreshRateFB(
		XrSession session,
		float displayRefreshRate) {
	if (xrRequestDisplayRefreshRateFB_ptr == nullptr) {
		return XR_ERROR_HANDLE_INVALID;
	}

	return (*xrRequestDisplayRefreshRateFB_ptr)(session, displayRefreshRate);
}

XrResult initialise_fb_display_refresh_rate_extension(XrInstance instance) {
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
