#include <ARVRServer.hpp>

#include "xr_ext_hand_tracking_extension_wrapper.h"

#include "openxr/include/util.h"

HandTracker::HandTracker(XRExtHandTrackingExtensionWrapper *p_hand_tracking_api, const char *p_name, XrHandEXT p_hand_ext) :
		OpenXRInputBase(p_name, 0) {
	is_initialised = false;
	hand_tracker = XR_NULL_HANDLE;
	hand_tracking_api = p_hand_tracking_api;
	hand_ext = p_hand_ext;
}

void HandTracker::update(OpenXRApi *p_openxr_api) {
	if (!hand_tracking_api) {
		return;
	}
	if (!hand_tracking_api->is_supported()) {
		return;
	}

	const XrTime time = p_openxr_api->get_next_frame_time(); // This data will be used for the next frame we render

	XrResult result;

	if (hand_tracker == XR_NULL_HANDLE) {
		XrHandTrackerCreateInfoEXT createInfo = {
			.type = XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT,
			.next = nullptr,
			.hand = hand_ext,
			.handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT,
		};

		result = hand_tracking_api->xrCreateHandTrackerEXT(p_openxr_api->get_session(), &createInfo, &hand_tracker);
		if (!p_openxr_api->xr_result(result, "Failed to obtain hand tracking information")) {
			// not successful? then we do nothing.
			is_initialised = false;
		} else {
			velocities = {
				.type = XR_TYPE_HAND_JOINT_VELOCITIES_EXT,
				.jointCount = XR_HAND_JOINT_COUNT_EXT,
				.jointVelocities = joint_velocities,
			};

			if (hand_tracking_api->tracking_aim_state_supported()) {
				aimState = {
					.type = XR_TYPE_HAND_TRACKING_AIM_STATE_FB,
					.next = nullptr,
				};

				velocities.next = &aimState;
			}

			locations = {
				.type = XR_TYPE_HAND_JOINT_LOCATIONS_EXT,
				.next = &velocities,
				.isActive = false,
				.jointCount = XR_HAND_JOINT_COUNT_EXT,
				.jointLocations = joint_locations,
			};

			is_initialised = true;
		}
	}

	if (is_initialised) {
		void *next_pointer = nullptr;

		XrHandJointsMotionRangeInfoEXT motionRangeInfo;

		if (hand_tracking_api->hand_motion_supported()) {
			motionRangeInfo = {
				.type = XR_TYPE_HAND_JOINTS_MOTION_RANGE_INFO_EXT,
				.next = next_pointer,
				.handJointsMotionRange = motion_range,
			};

			next_pointer = &motionRangeInfo;
		}

		XrHandJointsLocateInfoEXT locateInfo = {
			.type = XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT,
			.next = next_pointer,
			.baseSpace = p_openxr_api->get_play_space(),
			.time = time,
		};

		result = hand_tracking_api->xrLocateHandJointsEXT(hand_tracker, &locateInfo, &locations);
		if (!p_openxr_api->xr_result(result, "failed to get tracking for hand {0}!", name)) {
			return;
		}

		// For some reason an inactive controller isn't coming back as inactive but has coordinates either as NAN or very large
		const XrPosef &palm = joint_locations[XR_HAND_JOINT_PALM_EXT].pose;
		if (!locations.isActive || isnan(palm.position.x) || palm.position.x < -1000000.00 || palm.position.x > 1000000.00) {
			locations.isActive = false; // workaround, make sure its inactive
		}

		if (hand_tracking_api->tracking_aim_state_supported() && locations.isActive && godot_controller != -1) {
			tracking_confidence = check_bit(XR_HAND_TRACKING_AIM_VALID_BIT_FB, aimState.status) ? TrackingConfidence::TRACKING_CONFIDENCE_HIGH : TrackingConfidence::TRACKING_CONFIDENCE_NONE;

			const float ws = ARVRServer::get_singleton()->get_world_scale();
			godot_transform controller_transform;
			auto *t = (Transform *)&controller_transform;

			*t = p_openxr_api->transform_from_pose(aimState.aimPose, ws);
			arvr_api->godot_arvr_set_controller_transform(
					godot_controller,
					&controller_transform,
					true,
					true);

			// Index pinch is mapped to the A/X button
			arvr_api->godot_arvr_set_controller_button(
					godot_controller,
					7,
					check_bit(XR_HAND_TRACKING_AIM_INDEX_PINCHING_BIT_FB, aimState.status));
			// Middle pinch is mapped to the B/Y button
			arvr_api->godot_arvr_set_controller_button(
					godot_controller,
					1,
					check_bit(XR_HAND_TRACKING_AIM_MIDDLE_PINCHING_BIT_FB, aimState.status));
			// Ring pinch is mapped to the front trigger
			arvr_api->godot_arvr_set_controller_button(
					godot_controller,
					15,
					check_bit(XR_HAND_TRACKING_AIM_RING_PINCHING_BIT_FB, aimState.status));
			// Little finger pinch is mapped to the side trigger / grip button
			arvr_api->godot_arvr_set_controller_button(
					godot_controller,
					2,
					check_bit(XR_HAND_TRACKING_AIM_LITTLE_PINCHING_BIT_FB, aimState.status));
			// Menu button
			arvr_api->godot_arvr_set_controller_button(
					godot_controller,
					3,
					check_bit(XR_HAND_TRACKING_AIM_MENU_PRESSED_BIT_FB, aimState.status));

			// To allow accessing the pinch state as provided by the API we map them here to
			// the joystick axis of the controller. This will give the ability to access the
			// basic hand tracking gestures without the need to query specific APIs.
			arvr_api->godot_arvr_set_controller_axis(
					godot_controller,
					7,
					aimState.pinchStrengthIndex,
					false);
			arvr_api->godot_arvr_set_controller_axis(
					godot_controller,
					6,
					aimState.pinchStrengthMiddle,
					false);
			arvr_api->godot_arvr_set_controller_axis(
					godot_controller,
					2,
					aimState.pinchStrengthRing,
					false);
			arvr_api->godot_arvr_set_controller_axis(
					godot_controller,
					4,
					aimState.pinchStrengthLittle,
					false);

		} else {
			tracking_confidence = TrackingConfidence::TRACKING_CONFIDENCE_NONE;
		}
	}
}

void HandTracker::cleanup() {
	// May more this into unbind, need to see if that is called at the right times..
	if (hand_tracker != XR_NULL_HANDLE) {
		hand_tracking_api->xrDestroyHandTrackerEXT(hand_tracker);
		hand_tracker = XR_NULL_HANDLE;
	}

	is_initialised = false;
}

XRExtHandTrackingExtensionWrapper::XRExtHandTrackingExtensionWrapper() {
	openxr_api = OpenXRApi::openxr_get_api();
	request_extensions[XR_EXT_HAND_TRACKING_EXTENSION_NAME] = &hand_tracking_ext;
	request_extensions[XR_EXT_HAND_JOINTS_MOTION_RANGE_EXTENSION_NAME] = &hand_motion_range_ext;
	request_extensions[XR_FB_HAND_TRACKING_AIM_EXTENSION_NAME] = &hand_tracking_aim_state_ext;

	for (int i = 0; i < MAX_TRACKED_HANDS; i++) {
		hand_trackers[i] = nullptr;
	}
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

void XRExtHandTrackingExtensionWrapper::add_input_maps() {
	// This should register our trackers in slots 3 and 4 as long as our extensions are registered in the correct order.
	for (int i = 0; i < MAX_TRACKED_HANDS; i++) {
		hand_trackers[i] = new HandTracker(this, hand_controller_names[i], i == 0 ? XR_HAND_LEFT_EXT : XR_HAND_RIGHT_EXT);
		openxr_api->add_input_map(hand_trackers[i]);
	}
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

#ifdef DEBUG
	Godot::print("Hand tracking is supported\n");
#endif

	hand_tracking_supported = true;
	return true;
}

void XRExtHandTrackingExtensionWrapper::cleanup_hand_tracking() {
	for (int i = 0; i < MAX_TRACKED_HANDS; i++) {
		hand_trackers[i]->cleanup();
	}
}

const HandTracker *XRExtHandTrackingExtensionWrapper::get_hand_tracker(uint32_t p_hand) const {
	if (p_hand < MAX_TRACKED_HANDS) {
		return hand_trackers[p_hand];
	} else {
		return nullptr;
	}
}

XrHandJointsMotionRangeEXT XRExtHandTrackingExtensionWrapper::get_motion_range(uint32_t p_hand) const {
	if (p_hand < MAX_TRACKED_HANDS && hand_trackers[p_hand]) {
		return hand_trackers[p_hand]->motion_range;
	} else {
		// just return this as the default
		return XR_HAND_JOINTS_MOTION_RANGE_UNOBSTRUCTED_EXT;
	}
}

void XRExtHandTrackingExtensionWrapper::set_motion_range(uint32_t p_hand, XrHandJointsMotionRangeEXT p_motion_range) {
	if (p_hand < MAX_TRACKED_HANDS && hand_trackers[p_hand]) {
		hand_trackers[p_hand]->motion_range = p_motion_range;
	}
}

bool XRExtHandTrackingExtensionWrapper::is_hand_tracker_controller(const int p_godot_controller) {
	for (const auto &hand_tracker : hand_trackers) {
		if (hand_tracker && hand_tracker->get_godot_controller() == p_godot_controller) {
			return true;
		}
	}
	return false;
}

TrackingConfidence XRExtHandTrackingExtensionWrapper::get_hand_tracker_tracking_confidence(const int p_godot_controller) {
	for (const auto &hand_tracker : hand_trackers) {
		if (hand_tracker && hand_tracker->get_godot_controller() == p_godot_controller) {
			return hand_tracker->get_tracking_confidence();
		}
	}
	return TRACKING_CONFIDENCE_NONE;
}
