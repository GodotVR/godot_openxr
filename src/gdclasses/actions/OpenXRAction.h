/////////////////////////////////////////////////////////////////////////////////////
// Our OpenXR Action resource

#ifndef OPENXR_ACTION_H
#define OPENXR_ACTION_H

#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {
class OpenXRAction : public Resource {
	GDCLASS(OpenXRAction, Resource)

public:
	enum ActionType {
		ACTION_BOOL,
		ACTION_FLOAT,
		ACTION_VECTOR2,
		ACTION_POSE
	};

private:
	String name;
	String localized_name;
	ActionType action_type;

protected:
	static void _bind_methods();

public:
	void set_name(const String &p_name);
	String get_name() const;

	OpenXRAction();
	~OpenXRAction();
};

} // namespace godot

VARIANT_ENUM_CAST(godot::OpenXRAction, ActionType);

#endif // !OPENXR_ACTION_H
