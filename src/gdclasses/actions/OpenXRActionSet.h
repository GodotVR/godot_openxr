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
	String localized_name;
	int priority;

	std::vector<Ref<OpenXRAction>> actions;

protected:
	static void _bind_methods();

public:
	void set_name(const String &p_name);
	String get_name() const;

	void set_localized_name(const String &p_name);
	String get_localized_name() const;

	void set_priority(int p_priority);
	int get_priority() const;

	void clear_actions();
	void add_action(Ref<OpenXRAction> p_action);
	void remove_action(Ref<OpenXRAction> p_action);
	int number_of_actions() const;
	Ref<OpenXRAction> get_action(int p_index) const;

	/* this needs https://github.com/godotengine/godot-cpp/pull/656
	Array _get_property_list();
	Variant _get(const String p_name) const;
	bool _set(const String p_name, Variant p_value);
	*/

	OpenXRActionSet();
	~OpenXRActionSet();
};
} // namespace godot

#endif // !OPENXR_ACTIONSET_H
