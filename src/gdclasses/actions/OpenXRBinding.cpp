/////////////////////////////////////////////////////////////////////////////////////
// Our OpenXR Binding resource

#include "gdclasses/actions/OpenXRBinding.h"

using namespace godot;

void OpenXRBinding::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_action", "action"), &OpenXRBinding::set_action);
	ClassDB::bind_method(D_METHOD("get_action"), &OpenXRBinding::get_action);

	ClassDB::bind_method(D_METHOD("set_path", "path"), &OpenXRBinding::set_path);
	ClassDB::bind_method(D_METHOD("get_path"), &OpenXRBinding::get_path);

	// this crashes:
	// ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "action", PROPERTY_HINT_RESOURCE_TYPE, "OpenXRAction"), "set_action", "get_action");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "path"), "set_path", "get_path");
}

void OpenXRBinding::set_action(const Ref<OpenXRAction> p_action) {
	action = p_action;

	// Should register this binding with the action..
}

Ref<OpenXRAction> OpenXRBinding::get_action() const {
	return action;
}

void OpenXRBinding::set_path(const String p_path) {
	path = p_path;
}

String OpenXRBinding::get_path() const {
	return path;
}

OpenXRBinding::OpenXRBinding() {

}

OpenXRBinding::~OpenXRBinding() {
	// should unregister the binding with our action
}
