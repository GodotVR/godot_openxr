////////////////////////////////////////////////////////////////////////////////////////////////
// Wrapper class for interacting with OpenXR

#ifndef OPENXR_API_H
#define OPENXR_API_H

#include <Godot.hpp>

#ifdef WIN32
#include <windows.h>
#endif

#include <stdint.h>
#include <stdio.h>

#include "xrmath.h"
#include <openxr/openxr.h>

#define XR_MND_BALL_ON_STICK_EXTENSION_NAME "XR_MNDX_ball_on_a_stick_controller"

#ifdef WIN32
#define XR_USE_PLATFORM_WIN32
#else
#define XR_USE_PLATFORM_XLIB
#endif
#define XR_USE_GRAPHICS_API_OPENGL

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#include <glad/glad.h>
#else
// linux
#define GL_GLEXT_PROTOTYPES 1
#define GL3_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glext.h>

#include <GL/glx.h>
#include <X11/Xlib.h>
#endif

#include <gdnative/gdnative.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

class OpenXRApi {
public:
	enum Hands {
		HAND_LEFT,
		HAND_RIGHT,
		HANDCOUNT
	};

	enum Actions {
		POSE_ACTION_INDEX,
		TRIGGER_ACTION_INDEX,
		GRAB_ACTION_INDEX,
		MENU_ACTION_INDEX,
		LAST_ACTION_INDEX
	};

private:
	static OpenXRApi *singleton;
	bool successful_init;
	int use_count;

	XrInstance instance = XR_NULL_HANDLE;
	XrSession session = XR_NULL_HANDLE;

	/* XR_REFERENCE_SPACE_TYPE_LOCAL: head pose on startup/recenter is coordinate system origin.
	 * XR_REFERENCE_SPACE_TYPE_STAGE: origin is externally calibrated to be on play space floor. */
	XrReferenceSpaceType play_space_type = XR_REFERENCE_SPACE_TYPE_STAGE;

	XrSpace play_space = XR_NULL_HANDLE;
	XrSpace view_space = XR_NULL_HANDLE;
#ifdef WIN32
	XrGraphicsBindingOpenGLWin32KHR graphics_binding_gl;
#else
	XrGraphicsBindingOpenGLXlibKHR graphics_binding_gl;
#endif
	XrSwapchainImageOpenGLKHR **images = NULL;
	XrSwapchain *swapchains = NULL;
	uint32_t view_count;
	XrViewConfigurationView *configuration_views = NULL;
	// GLuint** framebuffers;
	// GLuint depthbuffer;

	XrCompositionLayerProjection *projectionLayer = NULL;
	XrFrameState frameState = {};
	bool running;

	XrSessionState state;
	bool view_pose_valid = false;

	uint32_t *buffer_index = NULL;

	XrView *views = NULL;
	XrCompositionLayerProjectionView *projection_views = NULL;

	XrActionSet actionSet;
	XrAction actions[LAST_ACTION_INDEX];
	XrPath handPaths[HANDCOUNT];
	XrSpace handSpaces[HANDCOUNT];

	godot_int godot_controllers[2];

	bool monado_stick_on_ball_ext;

	bool is_successful_init();

	template <class... Args>
	bool xr_result(XrResult result, const char *format, Args... values);

	bool isExtensionSupported(const char *extensionName, XrExtensionProperties *instanceExtensionProperties, uint32_t instanceExtensionCount);
	bool isViewConfigSupported(XrViewConfigurationType type, XrSystemId systemId);
	bool isReferenceSpaceSupported(XrReferenceSpaceType type);
	bool check_graphics_requirements_gl(XrSystemId system_id);
	XrAction createAction(XrActionType actionType, const char *actionName, const char *localizedActionName);
	XrResult getActionStates(XrAction action, XrStructureType actionStateType, void *states);
	bool suggestActions(const char *interaction_profile, XrAction *actions, XrPath **paths, int num_actions);
	XrResult acquire_image(int eye);
	bool transform_from_pose(godot_transform *p_dest, XrPosef *pose, float p_world_scale);
	void update_controllers();
	void transform_from_matrix(godot_transform *p_dest, XrMatrix4x4f *matrix, float p_world_scale);

public:
	static OpenXRApi *openxr_get_api();
	static void openxr_release_api();

	OpenXRApi();
	~OpenXRApi();

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
};

#endif /* !OPENXR_API_H */
