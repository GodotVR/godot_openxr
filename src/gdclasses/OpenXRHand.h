/////////////////////////////////////////////////////////////////////////////////////
// Our OpenXR hand GDNative object implemented through meshes

#ifndef OPENXR_HAND_H
#define OPENXR_HAND_H

#include "openxr/OpenXRApi.h"
#include "openxr/extensions/xr_ext_hand_tracking_extension_wrapper.h"
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/node3d.hpp>

namespace godot {
class OpenXRHand : public Node3D {
	GDCLASS(OpenXRHand, Node3D)

private:
	OpenXRApi *openxr_api;
	XRExtHandTrackingExtensionWrapper *hand_tracking_wrapper = nullptr;
	int hand;
	int motion_range;

	Node3D *joints[XR_HAND_JOINT_COUNT_EXT];
	void _set_motion_range();

protected:
	static void _bind_methods();

public:
	virtual void _ready() override;
	virtual void _physics_process(double delta) override;

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
