/////////////////////////////////////////////////////////////////////////////////////
// Our OpenXR hand GDNative object implemented through meshes

#ifndef OPENXR_HAND_H
#define OPENXR_HAND_H

#include "OpenXRApi.h"
#include <Ref.hpp>
#include <Spatial.hpp>

namespace godot {
class OpenXRHand : public Spatial {
	GODOT_CLASS(OpenXRHand, Spatial)

private:
	OpenXRApi *openxr_api;
	int hand;

	Spatial *joints[XR_HAND_JOINT_COUNT_EXT];

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
};
} // namespace godot

#endif // !OPENXR_HAND_H
