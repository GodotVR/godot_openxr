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
	Array get_action_sets() const;
	void set_action_sets(Array p_action_sets);

	void clear_interaction_profiles();
	void add_interaction_profile(Ref<OpenXRInteractionProfile> p_interaction_profile);
	void remove_interaction_profile(Ref<OpenXRInteractionProfile> p_interaction_profile);
	Array get_interaction_profiles() const;
	void set_interaction_profiles(Array p_interaction_profiles);

	OpenXRActionSets();
	~OpenXRActionSets();
};
} // namespace godot

#endif // !OPENXR_ACTIONSETS_H
