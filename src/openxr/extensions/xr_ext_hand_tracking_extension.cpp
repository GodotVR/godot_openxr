#include "xr_ext_hand_tracking_extension.h"

PFN_xrCreateHandTrackerEXT xrCreateHandTrackerEXT_ptr = nullptr;

XRAPI_ATTR XrResult XRAPI_CALL xrCreateHandTrackerEXT(
		XrSession session,
		const XrHandTrackerCreateInfoEXT *createInfo,
		XrHandTrackerEXT *handTracker) {
	if (xrCreateHandTrackerEXT_ptr == nullptr) {
		return XR_ERROR_HANDLE_INVALID;
	}

	return (*xrCreateHandTrackerEXT_ptr)(session, createInfo, handTracker);
}

PFN_xrDestroyHandTrackerEXT xrDestroyHandTrackerEXT_ptr = nullptr;

XRAPI_ATTR XrResult XRAPI_CALL xrDestroyHandTrackerEXT(
		XrHandTrackerEXT handTracker) {
	if (xrDestroyHandTrackerEXT_ptr == nullptr) {
		return XR_ERROR_HANDLE_INVALID;
	}

	return (*xrDestroyHandTrackerEXT_ptr)(handTracker);
}

PFN_xrLocateHandJointsEXT xrLocateHandJointsEXT_ptr = nullptr;

XRAPI_ATTR XrResult XRAPI_CALL xrLocateHandJointsEXT(
		XrHandTrackerEXT handTracker,
		const XrHandJointsLocateInfoEXT *locateInfo,
		XrHandJointLocationsEXT *locations) {
	if (xrLocateHandJointsEXT_ptr == nullptr) {
		return XR_ERROR_HANDLE_INVALID;
	}

	return (*xrLocateHandJointsEXT_ptr)(handTracker, locateInfo, locations);
}

XrResult initialise_ext_hand_tracking_extension(XrInstance instance) {
	XrResult result;

	result = xrGetInstanceProcAddr(instance, "xrCreateHandTrackerEXT", (PFN_xrVoidFunction *)&xrCreateHandTrackerEXT_ptr);
	if (result != XR_SUCCESS) {
		return result;
	}

	result = xrGetInstanceProcAddr(instance, "xrDestroyHandTrackerEXT", (PFN_xrVoidFunction *)&xrDestroyHandTrackerEXT_ptr);
	if (result != XR_SUCCESS) {
		return result;
	}

	result = xrGetInstanceProcAddr(instance, "xrLocateHandJointsEXT", (PFN_xrVoidFunction *)&xrLocateHandJointsEXT_ptr);
	if (result != XR_SUCCESS) {
		return result;
	}

	return XR_SUCCESS;
}
