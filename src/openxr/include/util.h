#ifndef UTIL_H
#define UTIL_H

#include <gen/Engine.hpp>
#include <gen/MainLoop.hpp>
#include <gen/Object.hpp>
#include <gen/SceneTree.hpp>
#include <gen/Viewport.hpp>

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

#define ENUM_TO_STRING_CASE(e) \
	case e: {                  \
		return String(#e);     \
	} break;

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
		Godot::print_error("Unable to retrieve main loop", __FUNCTION__, __FILE__, __LINE__);
		return nullptr;
	}

	auto *scene_tree = Object::cast_to<SceneTree>(main_loop);
	if (!scene_tree) {
		Godot::print_error("Unable to retrieve scene tree", __FUNCTION__, __FILE__, __LINE__);
		return nullptr;
	}

	Viewport *viewport = scene_tree->get_root();
	return viewport;
}

static inline bool check_bit(uint64_t in, uint64_t bits) {
	return (in & bits) != 0;
}

#endif // UTIL_H
