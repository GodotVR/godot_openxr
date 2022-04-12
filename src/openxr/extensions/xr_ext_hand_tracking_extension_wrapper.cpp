#include <ARVRServer.hpp>

#include "xr_ext_hand_tracking_extension_wrapper.h"

#include "openxr/include/util.h"

XRExtHandTrackingExtensionWrapper::XRExtHandTrackingExtensionWrapper() {
	openxr_api = OpenXRApi::openxr_get_api();
	request_extensions[XR_EXT_HAND_TRACKING_EXTENSION_NAME] = &hand_tracking_ext;
	request_extensions[XR_EXT_HAND_JOINTS_MOTION_RANGE_EXTENSION_NAME] = &hand_motion_range_ext;
	request_extensions[XR_FB_HAND_TRACKING_AIM_EXTENSION_NAME] = &hand_tracking_aim_state_ext;
}

XRExtHandTrackingExtensionWrapper::~XRExtHandTrackingExtensionWrapper() {
	cleanup();
	OpenXRApi::openxr_release_api();
}

void XRExtHandTrackingExtensionWrapper::cleanup() {
	cleanup_hand_tracking();

	hand_tracking_ext = false;
	hand_motion_range_ext = false;
	hand_tracking_aim_state_ext = false;
	hand_tracking_supported = false;
}

XRExtHandTrackingExtensionWrapper *XRExtHandTrackingExtensionWrapper::singleton = nullptr;

XRExtHandTrackingExtensionWrapper *XRExtHandTrackingExtensionWrapper::get_singleton() {
	if (!singleton) {
		singleton = new XRExtHandTrackingExtensionWrapper();
	}

	return singleton;
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
	update_handtracking();
}

void XRExtHandTrackingExtensionWrapper::on_state_stopping() {
	cleanup_hand_tracking();
}

void XRExtHandTrackingExtensionWrapper::on_session_destroyed() {
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
		hand_trackers[i].aim_state_godot_controller = -1;
		hand_trackers[i].tracking_confidence = TrackingConfidence::TRACKING_CONFIDENCE_NONE;
	}

#ifdef DEBUG
	Godot::print("Hand tracking is supported\n");
#endif

	hand_tracking_supported = true;
	return true;
}

void XRExtHandTrackingExtensionWrapper::cleanup_hand_tracking() {
	for (int i = 0; i < 2; i++) {
		if (hand_trackers[i].hand_tracker != XR_NULL_HANDLE) {
			xrDestroyHandTrackerEXT(hand_trackers[i].hand_tracker);

			hand_trackers[i].is_initialised = false;
			hand_trackers[i].hand_tracker = XR_NULL_HANDLE;
			hand_trackers[i].aim_state_godot_controller = -1;
			hand_trackers[i].tracking_confidence = TrackingConfidence::TRACKING_CONFIDENCE_NONE;
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

bool XRExtHandTrackingExtensionWrapper::is_hand_tracker_controller(const int p_godot_controller) {
	for (const auto &hand_tracker : hand_trackers) {
		if (hand_tracker.aim_state_godot_controller == p_godot_controller) {
			return true;
		}
	}
	return false;
}

TrackingConfidence XRExtHandTrackingExtensionWrapper::get_hand_tracker_tracking_confidence(const int p_godot_controller) {
	for (const auto &hand_tracker : hand_trackers) {
		if (hand_tracker.aim_state_godot_controller == p_godot_controller) {
			return hand_tracker.tracking_confidence;
		}
	}
	return TRACKING_CONFIDENCE_NONE;
}

void XRExtHandTrackingExtensionWrapper::update_handtracking() {
	if (!hand_tracking_supported) {
		return;
	}

	const XrTime time = openxr_api->get_next_frame_time(); // This data will be used for the next frame we render

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
				hand_trackers[i].velocities = {
					.type = XR_TYPE_HAND_JOINT_VELOCITIES_EXT,
					.jointCount = XR_HAND_JOINT_COUNT_EXT,
					.jointVelocities = hand_trackers[i].joint_velocities,
				};

				if (hand_tracking_aim_state_ext) {
					hand_trackers[i].aimState = {
						.type = XR_TYPE_HAND_TRACKING_AIM_STATE_FB,
						.next = nullptr,
					};

					hand_trackers[i].velocities.next = &hand_trackers[i].aimState;
				}

				hand_trackers[i].locations = {
					.type = XR_TYPE_HAND_JOINT_LOCATIONS_EXT,
					.next = &hand_trackers[i].velocities,
					.isActive = false,
					.jointCount = XR_HAND_JOINT_COUNT_EXT,
					.jointLocations = hand_trackers[i].joint_locations,
				};

				hand_trackers[i].is_initialised = true;
			}
		}

		if (hand_trackers[i].is_initialised) {
			void *next_pointer = nullptr;

			XrHandJointsMotionRangeInfoEXT motionRangeInfo;

			if (hand_motion_range_ext) {
				motionRangeInfo = {
					.type = XR_TYPE_HAND_JOINTS_MOTION_RANGE_INFO_EXT,
					.next = next_pointer,
					.handJointsMotionRange = hand_trackers[i].motion_range,
				};

				next_pointer = &motionRangeInfo;
			}

			XrHandJointsLocateInfoEXT locateInfo = {
				.type = XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT,
				.next = next_pointer,
				.baseSpace = openxr_api->get_play_space(),
				.time = time,
			};

			result = xrLocateHandJointsEXT(hand_trackers[i].hand_tracker, &locateInfo, &hand_trackers[i].locations);
			if (!openxr_api->xr_result(result, "failed to get tracking for hand {0}!", i)) {
				continue;
			}

			// For some reason an inactive controller isn't coming back as inactive but has coordinates either as NAN or very large
			const XrPosef &palm = hand_trackers[i].joint_locations[XR_HAND_JOINT_PALM_EXT].pose;
			if (
					!hand_trackers[i].locations.isActive || isnan(palm.position.x) || palm.position.x < -1000000.00 || palm.position.x > 1000000.00) {
				hand_trackers[i].locations.isActive = false; // workaround, make sure its inactive
			}

			if (hand_tracking_aim_state_ext && hand_trackers[i].locations.isActive) {
				// Controllers are updated based on the aim state's pose and pinches' strength
				if (hand_trackers[i].aim_state_godot_controller == -1) {
					hand_trackers[i].aim_state_godot_controller =
							arvr_api->godot_arvr_add_controller(
									const_cast<char *>(hand_controller_names[i]),
									i + HAND_CONTROLLER_ID_OFFSET,
									true,
									true);
				}

				hand_trackers[i].tracking_confidence = check_bit(XR_HAND_TRACKING_AIM_VALID_BIT_FB, hand_trackers[i].aimState.status) ? TrackingConfidence::TRACKING_CONFIDENCE_HIGH : TrackingConfidence::TRACKING_CONFIDENCE_NONE;

				int controller = hand_trackers[i].aim_state_godot_controller;

				const float ws = ARVRServer::get_singleton()->get_world_scale();
				godot_transform controller_transform;
				auto *t = (Transform *)&controller_transform;

				*t = openxr_api->transform_from_pose(hand_trackers[i].aimState.aimPose, ws);
				arvr_api->godot_arvr_set_controller_transform(
						controller,
						&controller_transform,
						true,
						true);

				// Index pinch is mapped to the A/X button
				arvr_api->godot_arvr_set_controller_button(
						controller,
						7,
						check_bit(XR_HAND_TRACKING_AIM_INDEX_PINCHING_BIT_FB, hand_trackers[i].aimState.status));
				// Middle pinch is mapped to the B/Y button
				arvr_api->godot_arvr_set_controller_button(
						controller,
						1,
						check_bit(XR_HAND_TRACKING_AIM_MIDDLE_PINCHING_BIT_FB, hand_trackers[i].aimState.status));
				// Ring pinch is mapped to the front trigger
				arvr_api->godot_arvr_set_controller_button(
						controller,
						15,
						check_bit(XR_HAND_TRACKING_AIM_RING_PINCHING_BIT_FB, hand_trackers[i].aimState.status));
				// Little finger pinch is mapped to the side trigger / grip button
				arvr_api->godot_arvr_set_controller_button(
						controller,
						2,
						check_bit(XR_HAND_TRACKING_AIM_LITTLE_PINCHING_BIT_FB, hand_trackers[i].aimState.status));
				// Menu button
				arvr_api->godot_arvr_set_controller_button(
						controller,
						3,
						check_bit(XR_HAND_TRACKING_AIM_MENU_PRESSED_BIT_FB, hand_trackers[i].aimState.status));

				// To allow accessing the pinch state as provided by the API we map them here to
				// the joystick axis of the controller. This will give the ability to access the
				// basic hand tracking gestures without the need to query specific APIs.
				arvr_api->godot_arvr_set_controller_axis(
						controller,
						7,
						hand_trackers[i].aimState.pinchStrengthIndex,
						false);
				arvr_api->godot_arvr_set_controller_axis(
						controller,
						6,
						hand_trackers[i].aimState.pinchStrengthMiddle,
						false);
				arvr_api->godot_arvr_set_controller_axis(
						controller,
						2,
						hand_trackers[i].aimState.pinchStrengthRing,
						false);
				arvr_api->godot_arvr_set_controller_axis(
						controller,
						4,
						hand_trackers[i].aimState.pinchStrengthLittle,
						false);

			} else if (hand_trackers[i].aim_state_godot_controller != -1) {
				// Remove the controller, it's no longer active
				arvr_api->godot_arvr_remove_controller(hand_trackers[i].aim_state_godot_controller);
				hand_trackers[i].aim_state_godot_controller = -1;
				hand_trackers[i].tracking_confidence = TrackingConfidence::TRACKING_CONFIDENCE_NONE;
			}
		}
	}
}
