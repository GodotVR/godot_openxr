#ifndef UTIL_H
#define UTIL_H

#include <gen/Engine.hpp>
#include <gen/MainLoop.hpp>
#include <gen/Object.hpp>
#include <gen/SceneTree.hpp>
#include <gen/Viewport.hpp>
#include <dlfcn.h>

using namespace godot;

#define UNPACK(...) __VA_ARGS__

#define EXT_PROTO_XRRESULT_FUNC1(func_name, arg1_type, arg1)                    \
	PFN_##func_name func_name##_ptr = nullptr;                                  \
	XRAPI_ATTR XrResult XRAPI_CALL func_name(UNPACK arg1_type p_##arg1) const { \
		if (!func_name##_ptr) {                                                 \
			return XR_ERROR_HANDLE_INVALID;                                     \
		}                                                                       \
		return (*func_name##_ptr)(p_##arg1);                                    \
	}

#define EXT_PROTO_XRRESULT_FUNC2(func_name, arg1_type, arg1, arg2_type, arg2)                              \
	PFN_##func_name func_name##_ptr = nullptr;                                                             \
	XRAPI_ATTR XrResult XRAPI_CALL func_name(UNPACK arg1_type p_##arg1, UNPACK arg2_type p_##arg2) const { \
		if (!func_name##_ptr) {                                                                            \
			return XR_ERROR_HANDLE_INVALID;                                                                \
		}                                                                                                  \
		return (*func_name##_ptr)(p_##arg1, p_##arg2);                                                     \
	}

#define EXT_PROTO_XRRESULT_FUNC3(func_name, arg1_type, arg1, arg2_type, arg2, arg3_type, arg3)                                        \
	PFN_##func_name func_name##_ptr = nullptr;                                                                                        \
	XRAPI_ATTR XrResult XRAPI_CALL func_name(UNPACK arg1_type p_##arg1, UNPACK arg2_type p_##arg2, UNPACK arg3_type p_##arg3) const { \
		if (!func_name##_ptr) {                                                                                                       \
			return XR_ERROR_HANDLE_INVALID;                                                                                           \
		}                                                                                                                             \
		return (*func_name##_ptr)(p_##arg1, p_##arg2, p_##arg3);                                                                      \
	}

#define EXT_PROTO_XRRESULT_FUNC4(func_name, arg1_type, arg1, arg2_type, arg2, arg3_type, arg3, arg4_type, arg4)                                                  \
	PFN_##func_name func_name##_ptr = nullptr;                                                                                                                   \
	XRAPI_ATTR XrResult XRAPI_CALL func_name(UNPACK arg1_type p_##arg1, UNPACK arg2_type p_##arg2, UNPACK arg3_type p_##arg3, UNPACK arg4_type p_##arg4) const { \
		if (!func_name##_ptr) {                                                                                                                                  \
			return XR_ERROR_HANDLE_INVALID;                                                                                                                      \
		}                                                                                                                                                        \
		return (*func_name##_ptr)(p_##arg1, p_##arg2, p_##arg3, p_##arg4);                                                                                       \
	}

#define EXT_PROTO_XRRESULT_FUNC5(func_name, arg1_type, arg1, arg2_type, arg2, arg3_type, arg3, arg4_type, arg4, arg5_type, arg5)                                                            \
	PFN_##func_name func_name##_ptr = nullptr;                                                                                                                                              \
	XRAPI_ATTR XrResult XRAPI_CALL func_name(UNPACK arg1_type p_##arg1, UNPACK arg2_type p_##arg2, UNPACK arg3_type p_##arg3, UNPACK arg4_type p_##arg4, UNPACK arg5_type p_##arg5) const { \
		if (!func_name##_ptr) {                                                                                                                                                             \
			return XR_ERROR_HANDLE_INVALID;                                                                                                                                                 \
		}                                                                                                                                                                                   \
		return (*func_name##_ptr)(p_##arg1, p_##arg2, p_##arg3, p_##arg4, p_##arg5);                                                                                                        \
	}

#define EXT_PROTO_XRRESULT_FUNC6(func_name, arg1_type, arg1, arg2_type, arg2, arg3_type, arg3, arg4_type, arg4, arg5_type, arg5, arg6_type, arg6)                                                                      \
	PFN_##func_name func_name##_ptr = nullptr;                                                                                                                                                                         \
	XRAPI_ATTR XrResult XRAPI_CALL func_name(UNPACK arg1_type p_##arg1, UNPACK arg2_type p_##arg2, UNPACK arg3_type p_##arg3, UNPACK arg4_type p_##arg4, UNPACK arg5_type p_##arg5, UNPACK arg6_type p_##arg6) const { \
		if (!func_name##_ptr) {                                                                                                                                                                                        \
			return XR_ERROR_HANDLE_INVALID;                                                                                                                                                                            \
		}                                                                                                                                                                                                              \
		return (*func_name##_ptr)(p_##arg1, p_##arg2, p_##arg3, p_##arg4, p_##arg5, p_##arg6);                                                                                                                         \
	}

#define ENUM_TO_STRING_CASE(e) \
	case e: {                  \
		return String(#e);     \
	} break;

#define INIT_XR_FUNC_V(openxr_api, name)                                                                              \
	do {                                                                                                              \
		XrResult get_instance_proc_addr_result;                                                                       \
		get_instance_proc_addr_result = openxr_api->get_instance_proc_addr(#name, (PFN_xrVoidFunction *)&name##_ptr); \
		ERR_FAIL_COND_V(XR_FAILED(get_instance_proc_addr_result), false);                                             \
	} while (0)

#define EXT_INIT_XR_FUNC_V(name) INIT_XR_FUNC_V(openxr_api, name)
#define OPENXR_API_INIT_XR_FUNC_V(name) INIT_XR_FUNC_V(this, name)

#define INIT_XR_FUNC(openxr_api, name)                                                                                \
	do {                                                                                                              \
		XrResult get_instance_proc_addr_result;                                                                       \
		get_instance_proc_addr_result = openxr_api->get_instance_proc_addr(#name, (PFN_xrVoidFunction *)&name##_ptr); \
		ERR_FAIL_COND(XR_FAILED(get_instance_proc_addr_result));                                                      \
	} while (0)

#define EXT_INIT_XR_FUNC(name) INIT_XR_FUNC(openxr_api, name)
#define OPENXR_API_INIT_XR_FUNC(name) INIT_XR_FUNC(this, name)

static Error open_dynamic_library(const String p_path, void *&p_library_handle) {
	String path = p_path;

	p_library_handle = dlopen(path.utf8().get_data(), RTLD_NOW);
	if (!p_library_handle) {
		Godot::print_error("Can't open dynamic library: " + p_path + ", error: " + dlerror() + ".", __FUNCTION__, __FILE__, __LINE__);
		return Error::FAILED;
	}

	return Error::OK;
}

static Error close_dynamic_library(void *p_library_handle) {
	if (dlclose(p_library_handle)) {
		return Error::FAILED;
	}
	return Error::OK;
}

static Error get_dynamic_library_symbol_handle(void *p_library_handle, const String p_name, void *&p_symbol_handle) {
	const char *error;
	dlerror(); // Clear existing errors

	p_symbol_handle = dlsym(p_library_handle, p_name.utf8().get_data());
	error = dlerror();
	if (error != nullptr) {
		Godot::print_error("Can't resolve symbol: " + p_name + ", error: " + error + ".", __FUNCTION__, __FILE__, __LINE__);
		return Error::FAILED;
	}

	return Error::OK;
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
