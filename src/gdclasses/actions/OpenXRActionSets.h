/////////////////////////////////////////////////////////////////////////////////////
// Our OpenXR ActionSets resource

#ifndef OPENXR_ACTIONSETS_H
#define OPENXR_ACTIONSETS_H

#include <godot_cpp/classes/resource.hpp>

#include "gdclasses/actions/OpenXRActionSet.h"
#include "gdclasses/actions/OpenXRInteractionProfile.h"

#include <vector>

namespace godot {
class OpenXRActionSets : public Resource {
	GDCLASS(OpenXRActionSets, Resource)

private:
	std::vector<Ref<OpenXRActionSet>> action_sets;
	std::vector<Ref<OpenXRInteractionProfile>> interaction_profiles;

protected:
	static void _bind_methods();

public:
	void clear_action_sets();
	void add_action_set(Ref<OpenXRActionSet> p_action_set);
	void remove_action_set(Ref<OpenXRActionSet> p_action_set);
	int number_of_action_sets() const;
	Ref<OpenXRActionSet> get_action_set(int p_index) const;

	void clear_interaction_profiles();
	void add_interaction_profile(Ref<OpenXRInteractionProfile> p_interaction_profile);
	void remove_interaction_profile(Ref<OpenXRInteractionProfile> p_interaction_profile);
	int number_of_interaction_profiles() const;
	Ref<OpenXRInteractionProfile> get_interaction_profile(int p_index) const;

	/* this needs https://github.com/godotengine/godot-cpp/pull/656
	Array _get_property_list();
	Variant _get(const String p_name) const;
	bool _set(const String p_name, Variant p_value);
	*/

	OpenXRActionSets();
	~OpenXRActionSets();
};
} // namespace godot

#endif // !OPENXR_ACTIONSETS_H
