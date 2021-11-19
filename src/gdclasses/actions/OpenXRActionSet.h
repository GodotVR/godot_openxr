/////////////////////////////////////////////////////////////////////////////////////
// Our OpenXR ActionSet resource

#ifndef OPENXR_ACTIONSET_H
#define OPENXR_ACTIONSET_H

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/string.hpp>

#include "gdclasses/actions/OpenXRAction.h"

#include <vector>

namespace godot {
class OpenXRActionSet : public Resource {
	GDCLASS(OpenXRActionSet, Resource)

private:
	String name;
	int priority = 0;

	std::vector<Ref<OpenXRAction>> actions;

protected:
	static void _bind_methods();

public:
	void set_name(const String &p_name);
	String get_name() const;

	void set_priority(int p_priority);
	int get_priority() const;

	void clear_actions();
	void add_action(Ref<OpenXRAction> p_action);
	void remove_action(Ref<OpenXRAction> p_action);
	Array get_actions() const;
	void set_actions(Array p_actions);

	OpenXRActionSet();
	~OpenXRActionSet();
};
} // namespace godot

#endif // !OPENXR_ACTIONSET_H
