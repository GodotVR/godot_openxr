#include <ARVRServer.hpp>

#include "xr_htc_vive_tracker_extension_wrapper.h"

XRHTCViveTrackerExtensionWrapper *XRHTCViveTrackerExtensionWrapper::singleton = nullptr;

XRHTCViveTrackerExtensionWrapper *XRHTCViveTrackerExtensionWrapper::get_singleton() {
	if (!singleton) {
		singleton = new XRHTCViveTrackerExtensionWrapper();
	}

	return singleton;
}

XRHTCViveTrackerExtensionWrapper::XRHTCViveTrackerExtensionWrapper() {
	openxr_api = OpenXRApi::openxr_get_api();

	request_extensions[XR_HTCX_VIVE_TRACKER_INTERACTION_EXTENSION_NAME] = &available;
}

XRHTCViveTrackerExtensionWrapper::~XRHTCViveTrackerExtensionWrapper() {
	OpenXRApi::openxr_release_api();
}

bool XRHTCViveTrackerExtensionWrapper::is_available() {
	return available;
}

void XRHTCViveTrackerExtensionWrapper::add_input_maps() {
	if (available) {
		openxr_api->add_input_map(new OpenXRInputHTCTracker("/user/vive_tracker_htcx/role/left_foot"));
		openxr_api->add_input_map(new OpenXRInputHTCTracker("/user/vive_tracker_htcx/role/right_foot"));
		openxr_api->add_input_map(new OpenXRInputHTCTracker("/user/vive_tracker_htcx/role/left_shoulder"));
		openxr_api->add_input_map(new OpenXRInputHTCTracker("/user/vive_tracker_htcx/role/right_shoulder"));
		openxr_api->add_input_map(new OpenXRInputHTCTracker("/user/vive_tracker_htcx/role/left_elbow"));
		openxr_api->add_input_map(new OpenXRInputHTCTracker("/user/vive_tracker_htcx/role/right_elbow"));
		openxr_api->add_input_map(new OpenXRInputHTCTracker("/user/vive_tracker_htcx/role/left_knee"));
		openxr_api->add_input_map(new OpenXRInputHTCTracker("/user/vive_tracker_htcx/role/right_knee"));
		openxr_api->add_input_map(new OpenXRInputHTCTracker("/user/vive_tracker_htcx/role/waist"));
		openxr_api->add_input_map(new OpenXRInputHTCTracker("/user/vive_tracker_htcx/role/chest"));
		openxr_api->add_input_map(new OpenXRInputHTCTracker("/user/vive_tracker_htcx/role/camera"));
		openxr_api->add_input_map(new OpenXRInputHTCTracker("/user/vive_tracker_htcx/role/keyboard"));
	}
}

bool XRHTCViveTrackerExtensionWrapper::on_event_polled(const XrEventDataBuffer &event) {
	switch (event.type) {
		case XR_TYPE_EVENT_DATA_VIVE_TRACKER_CONNECTED_HTCX: {
			// Investigate if we need to do more here
			printf("OpenXR EVENT: VIVE tracker connected");

			return true;
		} break;
		default: {
			return false;
		} break;
	}
}

OpenXRInputHTCTracker::OpenXRInputHTCTracker(const char *p_name) :
		OpenXRInputBase(p_name, 0) {
	// nothing to do here for now...
}

void OpenXRInputHTCTracker::update(OpenXRApi *p_openxr_api) {
	if (toplevel_path == XR_NULL_PATH) {
		// no path, skip this
	} else if (godot_controller == -1) {
		// not Godot controller registered
	} else {
		const float ws = ARVRServer::get_singleton()->get_world_scale();
		OpenXRApi::DefaultAction *default_actions = &p_openxr_api->default_actions[0]; // Copy for easy access
		bool is_active = false;

		// If our aim pose is active, our controller is active
		// note, if the user has removed this action then our old controller approach becomes defunct
		if (default_actions[OpenXRApi::ACTION_AIM_POSE].action != NULL) {
			is_active = default_actions[OpenXRApi::ACTION_AIM_POSE].action->is_pose_active(toplevel_path);
		}

		if (is_active) {
			// Start with our pose, we put our ARVRController on our aim pose (may need to change this to our grip pose...)
			godot_transform controller_transform;
			Transform *t = (Transform *)&controller_transform;
			tracking_confidence = default_actions[OpenXRApi::ACTION_AIM_POSE].action->get_as_pose(toplevel_path, ws, *t);

			if (tracking_confidence != TRACKING_CONFIDENCE_NONE) {
				arvr_api->godot_arvr_set_controller_transform(godot_controller, &controller_transform, true, true);
			}

			// For HTC trackers we have limited inputs

			// Button and axis are hardcoded..
			// Axis
			if (default_actions[OpenXRApi::ACTION_FRONT_TRIGGER].action != NULL) {
				arvr_api->godot_arvr_set_controller_axis(godot_controller, 2, default_actions[OpenXRApi::ACTION_FRONT_TRIGGER].action->get_as_float(toplevel_path), true); // 0.0 -> 1.0
			}
			if (default_actions[OpenXRApi::ACTION_SIDE_TRIGGER].action != NULL) {
				arvr_api->godot_arvr_set_controller_axis(godot_controller, 4, default_actions[OpenXRApi::ACTION_SIDE_TRIGGER].action->get_as_float(toplevel_path), true); // 0.0 -> 1.0
			}
			if (default_actions[OpenXRApi::ACTION_PRIMARY].action != NULL) {
				Vector2 v = default_actions[OpenXRApi::ACTION_PRIMARY].action->get_as_vector(toplevel_path);
				arvr_api->godot_arvr_set_controller_axis(godot_controller, 0, v.x, true); // -1.0 -> 1.0
				arvr_api->godot_arvr_set_controller_axis(godot_controller, 1, v.y, true); // -1.0 -> 1.0
			}

			// Buttons
			if (default_actions[OpenXRApi::ACTION_MENU_BUTTON].action != NULL) {
				arvr_api->godot_arvr_set_controller_button(godot_controller, 3, default_actions[OpenXRApi::ACTION_MENU_BUTTON].action->get_as_bool(toplevel_path));
			}
			if (default_actions[OpenXRApi::ACTION_FRONT_BUTTON].action != NULL) {
				arvr_api->godot_arvr_set_controller_button(godot_controller, 15, default_actions[OpenXRApi::ACTION_FRONT_BUTTON].action->get_as_bool(toplevel_path));
			}
			if (default_actions[OpenXRApi::ACTION_SIDE_BUTTON].action != NULL) {
				arvr_api->godot_arvr_set_controller_button(godot_controller, 2, default_actions[OpenXRApi::ACTION_SIDE_BUTTON].action->get_as_bool(toplevel_path));
			}
			if (default_actions[OpenXRApi::ACTION_PRIMARY_BUTTON].action != NULL) {
				arvr_api->godot_arvr_set_controller_button(godot_controller, 14, default_actions[OpenXRApi::ACTION_PRIMARY_BUTTON].action->get_as_bool(toplevel_path));
			}
			if (default_actions[OpenXRApi::ACTION_PRIMARY_TOUCH].action != NULL) {
				arvr_api->godot_arvr_set_controller_button(godot_controller, 12, default_actions[OpenXRApi::ACTION_PRIMARY_TOUCH].action->get_as_bool(toplevel_path));
			}

			if (default_actions[OpenXRApi::ACTION_HAPTIC].action != NULL) {
				// Godot currently only gives us a float between 0.0 and 1.0 for rumble strength.
				// Full haptic control will be offered through another object
				float haptic = arvr_api->godot_arvr_get_controller_rumble(godot_controller);
				if (haptic > 0.0) {
					// 17,000,000.0 nanoseconds (17ms) is slightly more then the duration of one frame if we're outputting at 60fps
					// so if we sustain our pulse we should be issuing a new pulse before the old one ends
					default_actions[OpenXRApi::ACTION_HAPTIC].action->do_haptic_pulse(toplevel_path, 17.0 * 1000 * 1000, XR_FREQUENCY_UNSPECIFIED, haptic);
				}
			}
		} else {
			// In Godot 4 this will also mark the tracker as inactive but in Godot 3 we'll need the user to check this manually
			// But our old approach of removing and re-adding controllers means that IDs get messed up so...
			tracking_confidence = TRACKING_CONFIDENCE_NONE;
		}
	}
}
