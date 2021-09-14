#ifndef UTIL_H
#define UTIL_H

#include <godot/gdnative_interface.h>

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/main_loop.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

#define UNPACK(...) __VA_ARGS__

#define EXT_PROTO_XRRESULT_FUNC1(func_name, arg1_type, arg1)          \
	PFN_##func_name func_name##_ptr = nullptr;                        \
	XRAPI_ATTR XrResult XRAPI_CALL func_name(UNPACK arg1_type arg1) { \
		if (!func_name##_ptr) {                                       \
			return XR_ERROR_HANDLE_INVALID;                           \
		}                                                             \
		return (*func_name##_ptr)(arg1);                              \
	}

#define EXT_PROTO_XRRESULT_FUNC2(func_name, arg1_type, arg1, arg2_type, arg2)                \
	PFN_##func_name func_name##_ptr = nullptr;                                               \
	XRAPI_ATTR XrResult XRAPI_CALL func_name(UNPACK arg1_type arg1, UNPACK arg2_type arg2) { \
		if (!func_name##_ptr) {                                                              \
			return XR_ERROR_HANDLE_INVALID;                                                  \
		}                                                                                    \
		return (*func_name##_ptr)(arg1, arg2);                                               \
	}

#define EXT_PROTO_XRRESULT_FUNC3(func_name, arg1_type, arg1, arg2_type, arg2, arg3_type, arg3)                      \
	PFN_##func_name func_name##_ptr = nullptr;                                                                      \
	XRAPI_ATTR XrResult XRAPI_CALL func_name(UNPACK arg1_type arg1, UNPACK arg2_type arg2, UNPACK arg3_type arg3) { \
		if (!func_name##_ptr) {                                                                                     \
			return XR_ERROR_HANDLE_INVALID;                                                                         \
		}                                                                                                           \
		return (*func_name##_ptr)(arg1, arg2, arg3);                                                                \
	}

// The map should be of type std::map<const char *, PFN_xrVoidFunction*>
#define LOAD_FUNC_POINTER_IN_MAP(map_name, func_name) map_name[#func_name] = (PFN_xrVoidFunction *)&func_name##_ptr

static XrResult initialize_function_pointer_map(XrInstance instance, std::map<const char *, PFN_xrVoidFunction *> function_pointer_map) {
	XrResult result;
	for (auto const &entry : function_pointer_map) {
		result = xrGetInstanceProcAddr(instance, entry.first, entry.second);
		if (result != XR_SUCCESS) {
			return result;
		}
	}

	return XR_SUCCESS;
}

static Viewport *get_main_viewport() {
	MainLoop *main_loop = Engine::get_singleton()->get_main_loop();
	if (!main_loop) {
		UtilityFunctions::print("Unable to retrieve main loop");
		return nullptr;
	}

	auto *scene_tree = Object::cast_to<SceneTree>(main_loop);
	if (!scene_tree) {
		UtilityFunctions::print("Unable to retrieve scene tree");
		return nullptr;
	}

	Window *window = scene_tree->get_root();
	Viewport *viewport = Object::cast_to<Viewport>((Object *)window);

	return viewport;
}

#endif // UTIL_H
