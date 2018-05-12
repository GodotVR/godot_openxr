////////////////////////////////////////////////////////////////////////////////////////////////
// GodotCalls is a utility for storing shared data required to call back into Godot

#include "GodotCalls.h"

const godot_gdnative_core_api_struct *api = NULL;
const godot_gdnative_ext_arvr_api_struct *arvr_api = NULL;
const godot_gdnative_ext_nativescript_api_struct *nativescript_api = NULL;

// we don't use anything more for OpenHMD at the moment
