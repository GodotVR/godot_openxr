/////////////////////////////////////////////////////////////////////////////////////
// Our OpenXR config GDNative object

#ifndef OPENXR_CONFIG_H
#define OPENXR_CONFIG_H

#include "openxr/OpenXRApi.h"
#include <Node.hpp>

namespace godot {
class OpenXRConfig : public Node {
	GODOT_CLASS(OpenXRConfig, Node)

private:
	OpenXRApi *openxr_api;

public:
	static void _register_methods();

	void _init();

	OpenXRConfig();
	~OpenXRConfig();

	bool keep_3d_linear() const;

	int get_view_config_type() const;
	void set_view_config_type(const int p_config_type);

	int get_form_factor() const;
	void set_form_factor(const int p_form_factor);

	godot::Array get_enabled_extensions() const;

	String get_action_sets() const;
	void set_action_sets(const String p_action_sets);

	String get_interaction_profiles() const;
	void set_interaction_profiles(const String p_interaction_profiles);
};
} // namespace godot

#endif // !OPENXR_CONFIG_H
