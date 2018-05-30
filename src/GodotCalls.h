////////////////////////////////////////////////////////////////////////////////////////////////
// GodotCalls is a utility for storing shared data required to call back into Godot

#ifndef GODOT_CALLS_H
#define GODOT_CALLS_H

#include <gdnative_api_struct.gen.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// forward declarations
extern const godot_gdnative_core_api_struct *api;
extern const godot_gdnative_ext_arvr_api_struct *arvr_api;
extern const godot_gdnative_ext_nativescript_api_struct *nativescript_api;

void GDN_EXPORT godot_openvr_gdnative_init(godot_gdnative_init_options *p_options);
void GDN_EXPORT godot_openvr_gdnative_terminate(godot_gdnative_terminate_options *p_options);

#endif /* !GODOT_CALLS_H */
