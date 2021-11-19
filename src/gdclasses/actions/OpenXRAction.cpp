/////////////////////////////////////////////////////////////////////////////////////
// Our OpenXR Action resource

#include "gdclasses/actions/OpenXRAction.h"

using namespace godot;

void OpenXRAction::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_name", "name"), &OpenXRAction::set_name);
	ClassDB::bind_method(D_METHOD("get_name"), &OpenXRAction::get_name);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "name"), "set_name", "get_name");

	ClassDB::bind_method(D_METHOD("set_action_type", "action_type"), &OpenXRAction::set_action_type);
	ClassDB::bind_method(D_METHOD("get_action_type"), &OpenXRAction::get_action_type);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "action_type"), "set_action_type", "get_action_type");

	BIND_ENUM_CONSTANT(ACTION_BOOL);
	BIND_ENUM_CONSTANT(ACTION_FLOAT);
	BIND_ENUM_CONSTANT(ACTION_VECTOR2);
	BIND_ENUM_CONSTANT(ACTION_POSE);
}

void OpenXRAction::set_name(const String &p_name) {
	name = p_name;
}

String OpenXRAction::get_name() const {
	return name;
}

void OpenXRAction::set_action_type(int p_type) {
	action_type = ActionType(p_type);
}

int OpenXRAction::get_action_type() const {
	return int(action_type);
}

OpenXRAction::OpenXRAction() {

}

OpenXRAction::~OpenXRAction() {

}
