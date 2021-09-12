#include "xr_ext_hand_tracking_extension_wrapper.h"

XRExtHandTrackingExtensionWrapper::XRExtHandTrackingExtensionWrapper() {
	openxr_api = OpenXRApi::openxr_get_api();
	request_extensions[XR_EXT_HAND_TRACKING_EXTENSION_NAME] = &hand_tracking_ext;
	request_extensions[XR_EXT_HAND_JOINTS_MOTION_RANGE_EXTENSION_NAME] = &hand_motion_range_ext;
}

XRExtHandTrackingExtensionWrapper::~XRExtHandTrackingExtensionWrapper() {
	cleanup();
	OpenXRApi::openxr_release_api();
}

void XRExtHandTrackingExtensionWrapper::cleanup() {
	cleanup_hand_tracking();

	hand_tracking_ext = false;
	hand_motion_range_ext = false;
	hand_tracking_supported = false;
}

XRExtHandTrackingExtensionWrapper *XRExtHandTrackingExtensionWrapper::singleton = nullptr;

XRExtHandTrackingExtensionWrapper *XRExtHandTrackingExtensionWrapper::get_singleton() {
	if (!singleton) {
		singleton = new XRExtHandTrackingExtensionWrapper();
	}

	return singleton;
}

std::map<const char *, bool *> XRExtHandTrackingExtensionWrapper::get_request_extensions() {
	return request_extensions;
}

void XRExtHandTrackingExtensionWrapper::on_instance_initialized(const XrInstance instance) {
	if (hand_tracking_ext) {
		XrResult result = initialise_ext_hand_tracking_extension(instance);
		if (!openxr_api->xr_result(result, "Failed to initialise hand tracking extension")) {
			hand_tracking_ext = false; // I guess we don't support it...
		}
	}
}

void XRExtHandTrackingExtensionWrapper::on_state_ready() {
	initialize_hand_tracking();
}

void XRExtHandTrackingExtensionWrapper::on_process_openxr() {
	// TODO: Tends to crash randomly on Quest, needs to investigate.
#ifndef ANDROID
	update_handtracking();
#endif
}

void XRExtHandTrackingExtensionWrapper::on_state_stopping() {
	cleanup_hand_tracking();
}

void XRExtHandTrackingExtensionWrapper::on_instance_destroyed() {
	cleanup();
}

PFN_xrCreateHandTrackerEXT xrCreateHandTrackerEXT_ptr = nullptr;

XRAPI_ATTR XrResult XRAPI_CALL XRExtHandTrackingExtensionWrapper::xrCreateHandTrackerEXT(
		XrSession session,
		const XrHandTrackerCreateInfoEXT *createInfo,
		XrHandTrackerEXT *handTracker) {
	if (xrCreateHandTrackerEXT_ptr == nullptr) {
		return XR_ERROR_HANDLE_INVALID;
	}

	return (*xrCreateHandTrackerEXT_ptr)(session, createInfo, handTracker);
}

PFN_xrDestroyHandTrackerEXT xrDestroyHandTrackerEXT_ptr = nullptr;

XRAPI_ATTR XrResult XRAPI_CALL XRExtHandTrackingExtensionWrapper::xrDestroyHandTrackerEXT(
		XrHandTrackerEXT handTracker) {
	if (xrDestroyHandTrackerEXT_ptr == nullptr) {
		return XR_ERROR_HANDLE_INVALID;
	}

	return (*xrDestroyHandTrackerEXT_ptr)(handTracker);
}

PFN_xrLocateHandJointsEXT xrLocateHandJointsEXT_ptr = nullptr;

XRAPI_ATTR XrResult XRAPI_CALL XRExtHandTrackingExtensionWrapper::xrLocateHandJointsEXT(
		XrHandTrackerEXT handTracker,
		const XrHandJointsLocateInfoEXT *locateInfo,
		XrHandJointLocationsEXT *locations) {
	if (xrLocateHandJointsEXT_ptr == nullptr) {
		return XR_ERROR_HANDLE_INVALID;
	}

	return (*xrLocateHandJointsEXT_ptr)(handTracker, locateInfo, locations);
}

XrResult XRExtHandTrackingExtensionWrapper::initialise_ext_hand_tracking_extension(XrInstance instance) {
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

bool XRExtHandTrackingExtensionWrapper::initialize_hand_tracking() {
	XrResult result;

	if (!hand_tracking_ext) {
		return false;
	}

#ifdef DEBUG
	Godot::print("OpenXR initialise hand tracking");
#endif

	XrSystemHandTrackingPropertiesEXT handTrackingSystemProperties = {
		.type = XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT,
	};

	XrSystemProperties systemProperties = {
		.type = XR_TYPE_SYSTEM_PROPERTIES,
		.next = &handTrackingSystemProperties,
	};

	result = xrGetSystemProperties(openxr_api->get_instance(), openxr_api->get_system_id(), &systemProperties);
	if (!openxr_api->xr_result(result, "Failed to obtain hand tracking information")) {
		return false;
	}

	if (!handTrackingSystemProperties.supportsHandTracking) {
		// The system does not support hand tracking
		Godot::print("Hand tracking is not supported\n");
		return false;
	}

	for (int i = 0; i < 2; i++) {
		// we'll do this later
		hand_trackers[i].is_initialised = false;
		hand_trackers[i].hand_tracker = XR_NULL_HANDLE;
	}

	Godot::print("Hand tracking is supported\n");

	hand_tracking_supported = true;
	return true;
}

void XRExtHandTrackingExtensionWrapper::cleanup_hand_tracking() {
	for (int i = 0; i < 2; i++) {
		if (hand_trackers[i].hand_tracker != XR_NULL_HANDLE) {
			xrDestroyHandTrackerEXT(hand_trackers[i].hand_tracker);

			hand_trackers[i].is_initialised = false;
			hand_trackers[i].hand_tracker = XR_NULL_HANDLE;
		}
	}
}

const HandTracker *XRExtHandTrackingExtensionWrapper::get_hand_tracker(uint32_t p_hand) const {
	if (p_hand < MAX_TRACKED_HANDS) {
		return &hand_trackers[p_hand];
	} else {
		return nullptr;
	}
}

XrHandJointsMotionRangeEXT XRExtHandTrackingExtensionWrapper::get_motion_range(uint32_t p_hand) const {
	if (p_hand < MAX_TRACKED_HANDS) {
		return hand_trackers[p_hand].motion_range;
	} else {
		// just return this as the default
		return XR_HAND_JOINTS_MOTION_RANGE_UNOBSTRUCTED_EXT;
	}
}

void XRExtHandTrackingExtensionWrapper::set_motion_range(uint32_t p_hand, XrHandJointsMotionRangeEXT p_motion_range) {
	if (p_hand < MAX_TRACKED_HANDS) {
		hand_trackers[p_hand].motion_range = p_motion_range;
	}
}

void XRExtHandTrackingExtensionWrapper::update_handtracking() {
	if (!hand_tracking_supported) {
		return;
	}

	const XrTime time = openxr_api->get_frame_state().predictedDisplayTime;
	XrResult result;

	for (int i = 0; i < 2; i++) {
		if (hand_trackers[i].hand_tracker == XR_NULL_HANDLE) {
			XrHandTrackerCreateInfoEXT createInfo = {
				.type = XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT,
				.next = nullptr,
				.hand = i == 0 ? XR_HAND_LEFT_EXT : XR_HAND_RIGHT_EXT,
				.handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT,
			};

			result = xrCreateHandTrackerEXT(openxr_api->get_session(), &createInfo, &hand_trackers[i].hand_tracker);
			if (!openxr_api->xr_result(result, "Failed to obtain hand tracking information")) {
				// not successful? then we do nothing.
				hand_trackers[i].is_initialised = false;
			} else {
				hand_trackers[i].velocities.type = XR_TYPE_HAND_JOINT_VELOCITIES_EXT;
				hand_trackers[i].velocities.jointCount = XR_HAND_JOINT_COUNT_EXT;
				hand_trackers[i].velocities.jointVelocities = hand_trackers[i].joint_velocities;

				hand_trackers[i].locations.type = XR_TYPE_HAND_JOINT_LOCATIONS_EXT;
				hand_trackers[i].locations.next = &hand_trackers[i].velocities;
				hand_trackers[i].locations.isActive = false;
				hand_trackers[i].locations.jointCount = XR_HAND_JOINT_COUNT_EXT;
				hand_trackers[i].locations.jointLocations = hand_trackers[i].joint_locations;

				hand_trackers[i].is_initialised = true;
			}
		}

		if (hand_trackers[i].is_initialised) {
			XrHandJointsLocateInfoEXT locateInfo = {
				.type = XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT,
				.next = nullptr,
				.baseSpace = openxr_api->get_play_space(),
				.time = time,
			};
			XrHandJointsMotionRangeInfoEXT motionRangeInfo;

			if (hand_motion_range_ext) {
				motionRangeInfo.type = XR_TYPE_HAND_JOINTS_MOTION_RANGE_INFO_EXT;
				motionRangeInfo.next = nullptr;
				motionRangeInfo.handJointsMotionRange = hand_trackers[i].motion_range;

				locateInfo.next = &motionRangeInfo;
			}

			// Godot::print("Obtaining hand joint info for {0}", i);

			result = xrLocateHandJointsEXT(hand_trackers[i].hand_tracker, &locateInfo, &hand_trackers[i].locations);
			if (openxr_api->xr_result(result, "failed to get tracking for hand {0}!", i)) {
				// For some reason an inactive controller isn't coming back as inactive but has coordinates either as NAN or very large
				const XrPosef &palm = hand_trackers[i].joint_locations[XR_HAND_JOINT_PALM_EXT].pose;
				if (
						!hand_trackers[i].locations.isActive || isnan(palm.position.x) || palm.position.x < -1000000.00 || palm.position.x > 1000000.00) {
					hand_trackers[i].locations.isActive = false; // workaround, make sure its inactive
					// printf("Hand %i inactive\n", i);
				} else {
					// we have our hand tracking info....

					// Godot::print("Hand {0}: ({1}, {2}, {3})\n", i, palm.position.x, palm.position.y, palm.position.z);
				}
			}
		}
	}
}
