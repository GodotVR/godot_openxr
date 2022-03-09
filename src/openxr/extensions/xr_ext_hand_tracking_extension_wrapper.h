#ifndef XR_EXT_HAND_TRACKING_EXTENSION_WRAPPER_H
#define XR_EXT_HAND_TRACKING_EXTENSION_WRAPPER_H

#include "openxr/OpenXRApi.h"
#include "openxr/include/openxr_inc.h"
#include "xr_extension_wrapper.h"

#include <map>

#define MAX_TRACKED_HANDS 2
#define HAND_CONTROLLER_ID_OFFSET 3

class XRExtHandTrackingExtensionWrapper;

class HandTracker : public OpenXRInputBase {
private:
	XRExtHandTrackingExtensionWrapper *hand_tracking_api = nullptr;

public:
	bool is_initialised = false;
	XrHandEXT hand_ext;
	XrHandJointsMotionRangeEXT motion_range = XR_HAND_JOINTS_MOTION_RANGE_UNOBSTRUCTED_EXT;

	XrHandTrackerEXT hand_tracker = XR_NULL_HANDLE;
	XrHandJointLocationEXT joint_locations[XR_HAND_JOINT_COUNT_EXT];
	XrHandJointVelocityEXT joint_velocities[XR_HAND_JOINT_COUNT_EXT];

	XrHandTrackingAimStateFB aimState;
	XrHandJointVelocitiesEXT velocities;
	XrHandJointLocationsEXT locations;

	HandTracker(XRExtHandTrackingExtensionWrapper *p_hand_tracking_api, const char *p_name, XrHandEXT p_hand_ext);

	virtual void update(OpenXRApi *p_openxr_api) override;
	void cleanup();
};

// Wrapper for the XR hand tracking related extensions.
class XRExtHandTrackingExtensionWrapper : public XRExtensionWrapper {
public:
	const char *hand_controller_names[MAX_TRACKED_HANDS] = {
		"Tracked Left Hand",
		"Tracked Right Hand"
	};

	static XRExtHandTrackingExtensionWrapper *get_singleton();

	virtual void add_input_maps() override;

	void on_instance_initialized(const XrInstance instance) override;

	void on_state_ready() override;

	void on_state_stopping() override;

	void on_session_destroyed() override;

	bool is_supported() { return hand_tracking_supported; }
	bool tracking_aim_state_supported() { return hand_tracking_aim_state_ext; }
	bool hand_motion_supported() { return hand_motion_range_ext; }

	const HandTracker *get_hand_tracker(uint32_t p_hand) const;

	XrHandJointsMotionRangeEXT get_motion_range(uint32_t p_hand) const;

	void set_motion_range(uint32_t p_hand, XrHandJointsMotionRangeEXT p_motion_range);

	bool is_hand_tracker_controller(const int p_godot_controller);

	TrackingConfidence get_hand_tracker_tracking_confidence(const int p_godot_controller);

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

protected:
	XRExtHandTrackingExtensionWrapper();
	~XRExtHandTrackingExtensionWrapper();

private:
	static XrResult initialise_ext_hand_tracking_extension(XrInstance instance);

	bool initialize_hand_tracking();

	void cleanup_hand_tracking();

	void cleanup();

	static XRExtHandTrackingExtensionWrapper *singleton;

	OpenXRApi *openxr_api = nullptr;
	bool hand_tracking_ext = false;
	bool hand_motion_range_ext = false;
	bool hand_tracking_aim_state_ext = false;
	bool hand_tracking_supported = false;

	HandTracker *hand_trackers[MAX_TRACKED_HANDS]; // Fixed for left and right hand
};

#endif // XR_EXT_HAND_TRACKING_EXTENSION_WRAPPER_H
