/////////////////////////////////////////////////////////////////////////////////////
// Our OpenXR Interaction profile resource

#ifndef OPENXR_INTERACTION_PROFILE_H
#define OPENXR_INTERACTION_PROFILE_H

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/string.hpp>

#include "gdclasses/actions/OpenXRBinding.h"

#include <vector>

namespace godot {
class OpenXRInteractionProfile : public Resource {
	GDCLASS(OpenXRInteractionProfile, Resource)

private:
	String path;

	std::vector<Ref<OpenXRBinding>> bindings;

protected:
	static void _bind_methods();

public:
	void set_path(const String &p_path);
	String get_path() const;

	void clear_bindings();
	void add_binding(Ref<OpenXRBinding> p_binding);
	void remove_binding(Ref<OpenXRBinding> p_binding);
	int number_of_bindings() const;
	Ref<OpenXRBinding> get_binding(int p_index) const;

	/* this needs https://github.com/godotengine/godot-cpp/pull/656
	Array _get_property_list();
	Variant _get(const String p_name) const;
	bool _set(const String p_name, Variant p_value);
	*/

	OpenXRInteractionProfile();
	~OpenXRInteractionProfile();
};
} // namespace godot

#endif // !OPENXR_INTERACTION_PROFILE_H
