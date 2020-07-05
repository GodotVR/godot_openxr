////////////////////////////////////////////////////////////////////////////////////////////////
// GodotCalls is a utility for storing shared data required to call back into
// Godot

#ifndef GODOT_CALLS_H
#define GODOT_CALLS_H

#ifdef WIN32
#include <windows.h>
#endif

#include <stdint.h>

#ifdef __cplusplus
// fully define these, don't waste time with needless callbacks for access
#define GODOT_CORE_API_GODOT_VECTOR2_TYPE_DEFINED
typedef union godot_vector2 {
	// Force struct to be classified as INTEGER by System V AMD64 ABI.
	uint8_t _dont_touch_that[8];
	struct
	{
		float x;
		float y;
	};

	inline void
	set(float p_x, float p_y) {
		x = p_x;
		y = p_y;
	};
} godot_vector2;

#define GODOT_CORE_API_GODOT_VECTOR3_TYPE_DEFINED
typedef union godot_vector3 {
	// Force struct to be classified as INTEGER by System V AMD64 ABI.
	uint8_t _dont_touch_that[12];
	struct
	{
		float x;
		float y;
		float z;
	};

	inline void
	set(float p_x, float p_y, float p_z) {
		x = p_x;
		y = p_y;
		z = p_z;
	};
} godot_vector3;

#define GODOT_CORE_API_GODOT_RECT2_TYPE_DEFINED
typedef union godot_rect2 {
	// Force struct to be classified as INTEGER by System V AMD64 ABI.
	uint8_t _dont_touch_that[16];
	struct
	{
		godot_vector2 position;
		godot_vector2 size;
	};
} godot_rect2;
#endif

#include <gdnative_api_struct.gen.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// forward declarations
extern const godot_gdnative_core_api_struct *api;
extern const godot_gdnative_ext_arvr_api_struct *arvr_api;
extern const godot_gdnative_ext_nativescript_api_struct *nativescript_api;

#ifdef __cplusplus
extern "C" {
#endif
void GDN_EXPORT godot_openxr_gdnative_init(godot_gdnative_init_options *p_options);
void GDN_EXPORT godot_openxr_gdnative_terminate(godot_gdnative_terminate_options *p_options);
#ifdef __cplusplus
}
#endif

#endif /* !GODOT_CALLS_H */
