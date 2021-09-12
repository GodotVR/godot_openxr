/////////////////////////////////////////////////////////////////////////////////////
// Our OpenXR hand GDNative object implemented through meshes

#ifndef OPENXR_HAND_H
#define OPENXR_HAND_H

#include "openxr/OpenXRApi.h"
#include "openxr/extensions/xr_ext_hand_tracking_extension_wrapper.h"
#include <Ref.hpp>
#include <Spatial.hpp>

namespace godot {
class OpenXRHand : public Spatial {
	GODOT_CLASS(OpenXRHand, Spatial)

private:
	OpenXRApi *openxr_api;
	XRExtHandTrackingExtensionWrapper *hand_tracking_wrapper = nullptr;
	int hand;
	int motion_range;

	Spatial *joints[XR_HAND_JOINT_COUNT_EXT];
	void _set_motion_range();

public:
	static void _register_methods();

	void _init();
	void _ready();
	void _physics_process(float delta);

	OpenXRHand();
	~OpenXRHand();

	bool is_active() const;

	int get_hand() const;
	void set_hand(int p_hand);

	int get_motion_range() const;
	void set_motion_range(int p_motion_range);
};
} // namespace godot

#endif // !OPENXR_HAND_H
