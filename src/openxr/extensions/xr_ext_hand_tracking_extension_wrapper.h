#ifndef XR_EXT_HAND_TRACKING_EXTENSION_WRAPPER_H
#define XR_EXT_HAND_TRACKING_EXTENSION_WRAPPER_H

#include "openxr/OpenXRApi.h"
#include "openxr/include/openxr_inc.h"
#include "xr_extension_wrapper.h"

#include <map>

#define MAX_TRACKED_HANDS 2
#define HAND_CONTROLLER_ID_OFFSET 3

class HandTracker {
public:
	bool is_initialised = false;
	XrHandJointsMotionRangeEXT motion_range = XR_HAND_JOINTS_MOTION_RANGE_UNOBSTRUCTED_EXT;

	XrHandTrackerEXT hand_tracker = XR_NULL_HANDLE;
	XrHandJointLocationEXT joint_locations[XR_HAND_JOINT_COUNT_EXT];
	XrHandJointVelocityEXT joint_velocities[XR_HAND_JOINT_COUNT_EXT];

	XrHandTrackingAimStateFB aimState;
	XrHandJointVelocitiesEXT velocities;
	XrHandJointLocationsEXT locations;

	godot_int aim_state_godot_controller = -1;
	TrackingConfidence tracking_confidence = TrackingConfidence::TRACKING_CONFIDENCE_NONE;
};

// Wrapper for the XR hand tracking related extensions.
class XRExtHandTrackingExtensionWrapper : public XRExtensionWrapper {
public:
	const char *hand_controller_names[MAX_TRACKED_HANDS] = {
		"Tracked Left Hand",
		"Tracked Right Hand"
	};

	static XRExtHandTrackingExtensionWrapper *get_singleton();

	void on_instance_initialized(const XrInstance instance) override;

	void on_state_ready() override;

	void on_process_openxr() override;

	void on_state_stopping() override;

	void on_session_destroyed() override;

	const HandTracker *get_hand_tracker(uint32_t p_hand) const;

	XrHandJointsMotionRangeEXT get_motion_range(uint32_t p_hand) const;

	void set_motion_range(uint32_t p_hand, XrHandJointsMotionRangeEXT p_motion_range);

	bool is_hand_tracker_controller(const int p_godot_controller);

	TrackingConfidence get_hand_tracker_tracking_confidence(const int p_godot_controller);

protected:
	XRExtHandTrackingExtensionWrapper();
	~XRExtHandTrackingExtensionWrapper();

private:
	static XRAPI_ATTR XrResult XRAPI_CALL xrCreateHandTrackerEXT(
			XrSession session,
			const XrHandTrackerCreateInfoEXT *createInfo,
			XrHandTrackerEXT *handTracker);

	static XRAPI_ATTR XrResult XRAPI_CALL xrDestroyHandTrackerEXT(
			XrHandTrackerEXT handTracker);

	static XRAPI_ATTR XrResult XRAPI_CALL xrLocateHandJointsEXT(
			XrHandTrackerEXT handTracker,
			const XrHandJointsLocateInfoEXT *locateInfo,
			XrHandJointLocationsEXT *locations);

	static XrResult initialise_ext_hand_tracking_extension(XrInstance instance);

	bool initialize_hand_tracking();

	void update_handtracking();

	void cleanup_hand_tracking();

	void cleanup();

	static XRExtHandTrackingExtensionWrapper *singleton;

	OpenXRApi *openxr_api = nullptr;
	bool hand_tracking_ext = false;
	bool hand_motion_range_ext = false;
	bool hand_tracking_aim_state_ext = false;
	bool hand_tracking_supported = false;

	HandTracker hand_trackers[MAX_TRACKED_HANDS]; // Fixed for left and right hand
};

#endif // XR_EXT_HAND_TRACKING_EXTENSION_WRAPPER_H
