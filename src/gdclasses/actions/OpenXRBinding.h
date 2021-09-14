/////////////////////////////////////////////////////////////////////////////////////
// Our OpenXR Binding resource

#ifndef OPENXR_BINDING_H
#define OPENXR_BINDING_H

#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/string.hpp>

#include "gdclasses/actions/OpenXRAction.h"

namespace godot {
class OpenXRBinding : public Resource {
	GDCLASS(OpenXRBinding, Resource)

private:
	Ref<OpenXRAction> action;
	String path;

protected:
	static void _bind_methods();

public:
	void set_action(const Ref<OpenXRAction> p_action);
	Ref<OpenXRAction> get_action() const;

	void set_path(const String p_path);
	String get_path() const;

	OpenXRBinding();
	~OpenXRBinding();
};
} // namespace godot

#endif // !OPENXR_BINDING_H
