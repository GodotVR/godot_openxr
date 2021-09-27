////////////////////////////////////////////////////////////////////////////////////////////////
// Wrapper class for interacting with OpenXR
//
// Initial implementation thanks Christoph Haag

#ifndef OPENXR_API_H
#define OPENXR_API_H

#include <Godot.hpp>
#include <OS.hpp>
#include <Transform.hpp>
#include <Vector2.hpp>

#ifdef WIN32
#include <windows.h>
#endif

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <set>
#include <vector>

#include "xrmath.h"

#ifdef WIN32
#include <glad/glad.h>
#elif ANDROID
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#else
// linux
#define GL_GLEXT_PROTOTYPES 1
#define GL3_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glext.h>

#include <GL/glx.h>
#include <X11/Xlib.h>
#endif

#include "openxr/extensions/xr_composition_layer_provider.h"
#include "openxr/extensions/xr_extension_wrapper.h"
#include "openxr/include/openxr_inc.h"
#include <openxr/openxr_platform.h>

// forward declare this
class OpenXRApi;

#include "openxr/actions/action.h"
#include "openxr/actions/actionset.h"

#define USER_INPUT_MAX 2

using namespace godot;

class OpenXRApi {
	friend class Action;
	friend class ActionSet;

public:
	// These are hardcoded and meant for our backwards compatibility layer
	// If not configured in our action sets they will be defunct

	struct InputMap {
		const char *name;
		XrPath toplevel_path;
		godot_int godot_controller;
		XrPath active_profile; // note, this can be a profile added in the OpenXR runtime unknown to our default mappings
	};

	InputMap inputmaps[USER_INPUT_MAX] = {
		{ "/user/hand/left", XR_NULL_PATH, -1, XR_NULL_PATH },
		{ "/user/hand/right", XR_NULL_PATH, -1, XR_NULL_PATH },
		// gamepad is already supported in Godots own joystick handling, head we're using directly
		// { "/user/foot/left", XR_NULL_PATH, -1, XR_NULL_PATH },
		// { "/user/foot/right", XR_NULL_PATH, -1, XR_NULL_PATH },
		// { "/user/treadmill", XR_NULL_PATH, -1, XR_NULL_PATH },
	};

	// Default actions we support so we can mimic our old ARVRController handling
	enum DefaultActions {
		// Poses
		ACTION_AIM_POSE, // we are not using this ourselves
		ACTION_GRIP_POSE,

		// Analog
		ACTION_FRONT_TRIGGER, // front trigger (Axis 2)
		ACTION_SIDE_TRIGGER, // side trigger/grip (Axis 4)
		ACTION_JOYSTICK, // primary joystick (Axis 0/1)

		// Buttons
		ACTION_AX_BUTTON, // A/X button (button 7)
		ACTION_BYM_BUTTON, // B/Y/Menu button (button 1)
		ACTION_FRONT_BUTTON, // front trigger as button (button 15)
		ACTION_SIDE_BUTTON, // side trigger/grip as button (button 2)
		ACTION_JOYSTICK_BUTTON, // Click on joystick (button 14)

		// Output
		ACTION_HAPTIC, // Haptic output

		ACTION_MAX
	};

	struct DefaultAction {
		const char *name;
		const XrActionType type;
		Action *action;
	};

	DefaultAction default_actions[ACTION_MAX] = {
		{ "aim_pose", XR_ACTION_TYPE_POSE_INPUT, NULL },
		{ "grip_pose", XR_ACTION_TYPE_POSE_INPUT, NULL },
		{ "front_trigger", XR_ACTION_TYPE_FLOAT_INPUT, NULL },
		{ "side_trigger", XR_ACTION_TYPE_FLOAT_INPUT, NULL },
		{ "joystick", XR_ACTION_TYPE_VECTOR2F_INPUT, NULL },
		{ "ax_buttons", XR_ACTION_TYPE_BOOLEAN_INPUT, NULL },
		{ "bym_button", XR_ACTION_TYPE_BOOLEAN_INPUT, NULL },
		{ "front_button", XR_ACTION_TYPE_BOOLEAN_INPUT, NULL },
		{ "side_button", XR_ACTION_TYPE_BOOLEAN_INPUT, NULL },
		{ "joystick_button", XR_ACTION_TYPE_BOOLEAN_INPUT, NULL },
		{ "haptic", XR_ACTION_TYPE_VIBRATION_OUTPUT, NULL },
	};

private:
	enum ActionSetStatus {
		ACTION_SET_UNINITIALISED,
		ACTION_SET_INITIALISED,
		ACTION_SET_FAILED
	};

	static OpenXRApi *singleton;
	bool initialised = false;
	bool running = false;
	int use_count = 1;
	godot::OS::VideoDriver video_driver = godot::OS::VIDEO_DRIVER_GLES3;

	// extensions
	bool monado_stick_on_ball_ext = false;

	std::vector<const char *> enabled_extensions;
	std::set<XRExtensionWrapper *> registered_extension_wrappers;
	std::set<XRCompositionLayerProvider *> composition_layer_providers;

	// feature flags
	XrViewConfigurationType view_config_type = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
	XrInstance instance = XR_NULL_HANDLE;
	XrSystemId systemId;
	XrSession session = XR_NULL_HANDLE;
	XrSessionState state = XR_SESSION_STATE_UNKNOWN;
	String system_name;
	uint32_t vendor_id = 0;

	bool is_steamvr = false;

	bool keep_3d_linear = true;
#ifdef WIN32
	XrGraphicsBindingOpenGLWin32KHR graphics_binding_gl;
	XrSwapchainImageOpenGLKHR **images = NULL;
#elif ANDROID
	XrGraphicsBindingOpenGLESAndroidKHR graphics_binding_gl;
	XrSwapchainImageOpenGLESKHR **images = NULL;
#else
	XrGraphicsBindingOpenGLXlibKHR graphics_binding_gl;
	XrSwapchainImageOpenGLKHR **images = NULL;
#endif
	XrSwapchain *swapchains = NULL;
	uint32_t view_count;
	XrViewConfigurationView *configuration_views = NULL;

	XrCompositionLayerProjection *projectionLayer = NULL;
	XrFrameState frameState = {};

	uint32_t *buffer_index = NULL;

	XrView *views = NULL;
	XrCompositionLayerProjectionView *projection_views = NULL;
	XrSpace play_space = XR_NULL_HANDLE;
	XrSpace view_space = XR_NULL_HANDLE;
	bool view_pose_valid = false;
	bool head_pose_valid = false;

	// config
	/*
	 * XR_REFERENCE_SPACE_TYPE_LOCAL: head pose on startup/recenter is coordinate system origin.
	 * XR_REFERENCE_SPACE_TYPE_STAGE: origin is externally calibrated to be on play space floor.
	 *
	 * Note that Godot has it's own implementation to support localise the headset, but we could expose this through our config
	 */
	XrReferenceSpaceType play_space_type = XR_REFERENCE_SPACE_TYPE_STAGE;

	/*
	 * XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY - wearable displays, usually stereoscopic
	 * XR_FORM_FACTOR_HANDHELD_DISPLAY - handheld devices, phones, tablets, etc.
	 */
	XrFormFactor form_factor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

	godot::String action_sets_json;
	godot::String interaction_profiles_json;

	std::vector<ActionSet *> action_sets;

	bool isExtensionSupported(const char *extensionName, XrExtensionProperties *instanceExtensionProperties, uint32_t instanceExtensionCount);
	bool isViewConfigSupported(XrViewConfigurationType type, XrSystemId systemId);
	bool isReferenceSpaceSupported(XrReferenceSpaceType type);

	bool initialiseInstance();
	bool initialiseSession();
	bool initialiseSpaces();
	void cleanupSpaces();
	bool initialiseSwapChains();
	void cleanupSwapChains();

	bool loadActionSets();
	bool bindActionSets();
	void unbindActionSets();
	void cleanupActionSets();

	bool poll_events();
	bool on_state_idle();
	bool on_state_ready();
	bool on_state_synchronized();
	bool on_state_visible();
	bool on_state_focused();
	bool on_state_stopping();
	bool on_state_loss_pending();
	bool on_state_exiting();

	bool check_graphics_requirements_gl(XrSystemId system_id);
	XrResult acquire_image(int eye);
	void update_actions();
	void transform_from_matrix(godot_transform *p_dest, XrMatrix4x4f *matrix, float p_world_scale);

	bool parse_action_sets(const godot::String &p_json);
	bool parse_interaction_profiles(const godot::String &p_json);

public:
	static OpenXRApi *openxr_get_api();
	static void openxr_release_api();

	OpenXRApi();
	~OpenXRApi();

	template <class T>
	void register_extension_wrapper() {
		register_extension_wrapper(T::get_singleton());
	}

	void register_extension_wrapper(XRExtensionWrapper *wrapper) {
		if (initialised) {
			Godot::print_error("Extension wrappers must be registered prior to initialization.", __FUNCTION__, __FILE__, __LINE__);
			return;
		}
		registered_extension_wrappers.insert(wrapper);
	}

	void register_composition_layer_provider(XRCompositionLayerProvider *provider) {
		composition_layer_providers.insert(provider);
	}

	void unregister_composition_layer_provider(XRCompositionLayerProvider *provider) {
		composition_layer_providers.erase(provider);
	}

	bool is_initialised();
	bool initialize();
	void uninitialize();
	bool is_running();

	void on_resume();
	void on_pause();

	XrInstance get_instance() { return instance; };
	XrSession get_session() { return session; };
	XrSystemId get_system_id() { return systemId; }
	XrSpace get_play_space() { return play_space; }
	XrFrameState get_frame_state() { return frameState; }
	String get_system_name() const { return system_name; }
	uint32_t get_vendor_id() const { return vendor_id; }

	uint32_t get_view_count() const { return view_count; }
	const XrSwapchain &get_swapchain(uint32_t eye) { return swapchains[eye]; }

	bool get_keep_3d_linear() { return keep_3d_linear; };

	template <class... Args>
	bool xr_result(XrResult result, const char *format, Args... values) const {
		if (XR_SUCCEEDED(result))
			return true;

		char resultString[XR_MAX_RESULT_STRING_SIZE];
		xrResultToString(instance, result, resultString);

		godot::Godot::print_error(
				godot::String("OpenXR ") + godot::String(format).format(godot::Array::make(values...)) + godot::String(" [") + godot::String(resultString) + godot::String("]"),
				__FUNCTION__,
				__FILE__,
				__LINE__);

		return false;
	};

	// config
	XrViewConfigurationType get_view_configuration_type() const;
	void set_view_configuration_type(const XrViewConfigurationType p_view_configuration_type);

	XrFormFactor get_form_factor() const;
	void set_form_factor(const XrFormFactor p_form_factor);

	godot::Array get_enabled_extensions() const;

	static const char *default_action_sets_json;
	godot::String get_action_sets_json() const;
	void set_action_sets_json(const godot::String &p_action_sets_json);

	static const char *default_interaction_profiles_json;
	godot::String get_interaction_profiles_json() const;
	void set_interaction_profiles_json(const godot::String &p_interaction_profiles_json);

	bool has_action_sets() { return action_sets.size() > 0; };
	ActionSet *get_action_set(const godot::String &p_name);
	Action *get_action(const char *p_name);

	/* render_openxr() should be called once per eye.
	 *
	 * If has_external_texture_support it assumes godot has finished rendering into
	 * the external texture and ignores texid. If false, it copies content from
	 * texid to the OpenXR swapchain. Then the image is released.
	 * If eye == 1, ends the frame.
	 */
	void render_openxr(int eye, uint32_t texid, bool has_external_texture_support);

	// fill_projection_matrix() should be called after process_openxr()
	void fill_projection_matrix(int eye, godot_real p_z_near, godot_real p_z_far, godot_real *p_projection);

	// recommended_rendertarget_size() returns required size of our image buffers
	void recommended_rendertarget_size(uint32_t *width, uint32_t *height);

	// get_view_transform() should be called after fill_projection_matrix()
	bool get_view_transform(int eye, float world_scale, godot_transform *transform_for_eye);

	// get_head_center() can be called at any time after init
	bool get_head_center(float world_scale, godot_transform *transform);

	// get_external_texture_for_eye() acquires images and sets has_support to true
	int get_external_texture_for_eye(int eye, bool *has_support);

	// process_openxr() should be called FIRST in the frame loop
	void process_openxr();

	// helper method to get a transform from an openxr pose
	godot::Transform transform_from_pose(const XrPosef &p_pose, float p_world_scale);

	// helper method to get a valid transform from an openxr space location
	godot::Transform transform_from_space_location(const XrSpaceLocation &p_location, float p_world_scale);
	godot::Transform transform_from_space_location(const XrHandJointLocationEXT &p_location, float p_world_scale);
};

#endif /* !OPENXR_API_H */
