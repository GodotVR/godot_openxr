////////////////////////////////////////////////////////////////////////////////////////////////
// Helper calls and singleton container for accessing openxr

#include "OXRCalls.h"
#include <stdint.h>

openxr_data_struct *openxr_data_singleton = NULL;

void
openxr_release_data()
{
	if (openxr_data_singleton == NULL) {
		// nothing to release
		printf(
		    "OpenXR: tried to release non-existent OpenXR context\n");
	} else if (openxr_data_singleton->use_count > 1) {
		// decrease use count
		openxr_data_singleton->use_count--;
		printf("OpenXR: decreased use count to %i\n",
		       openxr_data_singleton->use_count);
	} else {
		// cleanup openxr
		printf("OpenXR: releasing OpenXR context\n");

		deinit_openxr(openxr_data_singleton->api);

		api->godot_free(openxr_data_singleton);
		openxr_data_singleton = NULL;
	};
};

openxr_data_struct *
openxr_get_data()
{
	if (openxr_data_singleton != NULL) {
		// increase use count
		openxr_data_singleton->use_count++;
		printf("OpenXR: increased use count to %i\n",
		       openxr_data_singleton->use_count);
	} else {
		// init openxr
		printf("OpenXR: initialising OpenXR context\n");

		openxr_data_singleton = (openxr_data_struct *)api->godot_alloc(
		    sizeof(openxr_data_struct));
		if (openxr_data_singleton != NULL) {
			openxr_data_singleton->api = init_openxr();
			if (openxr_data_singleton->api == NULL) {
				printf("OpenXR init failed\n");
				api->godot_free(openxr_data_singleton);
				openxr_data_singleton = NULL;
			} else {
				printf("OpenXR init succeeded\n");
			}
		};
	}

	return openxr_data_singleton;
};

#define XR_MND_BALL_ON_STICK_EXTENSION_NAME "TODO_BALL_ON_STICK"

#define XR_USE_PLATFORM_XLIB
#define XR_USE_GRAPHICS_API_OPENGL

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GL_GLEXT_PROTOTYPES 1
#define GL3_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glext.h>

#include <GL/glx.h>
#include <X11/Xlib.h>

#include <gdnative/gdnative.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#define HANDCOUNT 2
#define HAND_LEFT 0
#define HAND_RIGHT 1

#define POSE_ACTION_INDEX 0
#define TRIGGER_ACTION_INDEX 1
#define GRAB_ACTION_INDEX 2
#define MENU_ACTION_INDEX 3
#define LAST_ACTION_INDEX 4 // array size

typedef struct xr_api
{
	XrInstance instance;
	XrSession session;
	XrSpace local_space;
	XrGraphicsBindingOpenGLXlibKHR graphics_binding_gl;
	XrSwapchainImageOpenGLKHR **images;
	XrSwapchain *swapchains;
	uint32_t view_count;
	XrViewConfigurationView *configuration_views;
	// GLuint** framebuffers;
	// GLuint depthbuffer;

	XrCompositionLayerProjection *projectionLayer;
	XrFrameState frameState;
	bool running;

	XrSessionState state;
	bool should_render;

	uint32_t *buffer_index;

	XrView *views;
	XrCompositionLayerProjectionView *projection_views;

	XrActionSet actionSet;
	XrAction actions[LAST_ACTION_INDEX];
	XrPath handPaths[HANDCOUNT];
	XrSpace handSpaces[HANDCOUNT];

	godot_int godot_controllers[2];

	bool monado_stick_on_ball_ext;
} xr_api;

bool
xr_result(XrInstance instance, XrResult result, const char *format, ...)
{
	if (XR_SUCCEEDED(result))
		return true;

	char resultString[XR_MAX_RESULT_STRING_SIZE];
	xrResultToString(instance, result, resultString);

	size_t len1 = strlen(format);
	size_t len2 = strlen(resultString) + 1;
	char formatRes[len1 + len2 + 5]; // + " []\n"
	sprintf(formatRes, "%s [%s]\n", format, resultString);

	va_list args;
	va_start(args, format);
	vprintf(formatRes, args);
	va_end(args);
	return false;
}

bool
isExtensionSupported(char *extensionName,
                     XrExtensionProperties *instanceExtensionProperties,
                     uint32_t instanceExtensionCount)
{
	for (uint32_t supportedIndex = 0;
	     supportedIndex < instanceExtensionCount; supportedIndex++) {
		if (!strcmp(extensionName,
		            instanceExtensionProperties[supportedIndex]
		                .extensionName)) {
			return true;
		}
	}
	return false;
}

bool
isViewConfigSupported(XrInstance instance,
                      XrViewConfigurationType type,
                      XrSystemId systemId)
{
	XrResult result;
	uint32_t viewConfigurationCount;
	result = xrEnumerateViewConfigurations(instance, systemId, 0,
	                                       &viewConfigurationCount, NULL);
	if (!xr_result(instance, result,
	               "Failed to get view configuration count"))
		return false;
	XrViewConfigurationType viewConfigurations[viewConfigurationCount];
	result = xrEnumerateViewConfigurations(
	    instance, systemId, viewConfigurationCount, &viewConfigurationCount,
	    viewConfigurations);
	if (!xr_result(instance, result,
	               "Failed to enumerate view configurations!"))
		return 1;

	for (uint32_t i = 0; i < viewConfigurationCount; ++i) {

		if (viewConfigurations[i] == type)
			return true;
	}
	return false;
}

bool
isReferenceSpaceSupported(XrInstance instance,
                          XrSession session,
                          XrReferenceSpaceType type)
{
	XrResult result;
	uint32_t referenceSpacesCount;
	result =
	    xrEnumerateReferenceSpaces(session, 0, &referenceSpacesCount, NULL);
	if (!xr_result(instance, result,
	               "Getting number of reference spaces failed!"))
		return 1;

	XrReferenceSpaceType referenceSpaces[referenceSpacesCount];
	result =
	    xrEnumerateReferenceSpaces(session, referenceSpacesCount,
	                               &referenceSpacesCount, referenceSpaces);
	if (!xr_result(instance, result,
	               "Enumerating reference spaces failed!"))
		return 1;

	for (uint32_t i = 0; i < referenceSpacesCount; i++) {
		if (referenceSpaces[i] == type)
			return true;
	}
	return false;
}

void
deinit_openxr(OPENXR_API_HANDLE _self)
{
	xr_api *self = (xr_api *)_self;
	free(self->projection_views);
	free(self->configuration_views);
	free(self->buffer_index);
	free(self->swapchains);
	if (self->images) {
		for (uint32_t i = 0; i < self->view_count; i++) {
			free(self->images[i]);
		}
	}
	free(self->images);
	free(self->projectionLayer);
	free(self->views);

	if (self->session) {
		xrDestroySession(self->session);
	}
	xrDestroyInstance(self->instance);

	free(self);
}

static XrAction
_createAction(xr_api *self,
              XrActionType actionType,
              char *actionName,
              char *localizedActionName)
{
	XrActionCreateInfo actionInfo = {.type = XR_TYPE_ACTION_CREATE_INFO,
	                                 .next = NULL,
	                                 .actionType = actionType,
	                                 .countSubactionPaths = HANDCOUNT,
	                                 .subactionPaths = self->handPaths};
	strcpy(actionInfo.actionName, actionName);
	strcpy(actionInfo.localizedActionName, localizedActionName);

	XrAction action;
	XrResult result = xrCreateAction(self->actionSet, &actionInfo, &action);
	if (!xr_result(self->instance, result, "failed to create %s action",
	               actionName))
		return NULL;

	return action;
}

static XrResult
_getActionStates(xr_api *self,
                 XrAction action,
                 XrStructureType actionStateType,
                 void *states)
{

	for (int i = 0; i < HANDCOUNT; i++) {
		XrActionStateGetInfo getInfo = {
		    .type = XR_TYPE_ACTION_STATE_GET_INFO,
		    .next = NULL,
		    .action = action,
		    .subactionPath = self->handPaths[i]};

		switch (actionStateType) {
		case XR_TYPE_ACTION_STATE_FLOAT: {
			XrActionStateFloat *resultStates = states;
			resultStates[i].type = XR_TYPE_ACTION_STATE_FLOAT,
			resultStates[i].next = NULL;
			XrResult result = xrGetActionStateFloat(
			    self->session, &getInfo, &resultStates[i]);
			if (!xr_result(self->instance, result,
			               "failed to get float value for hand %d!",
			               i))
				resultStates[i].isActive = false;
			break;
		}
		case XR_TYPE_ACTION_STATE_BOOLEAN: {
			XrActionStateBoolean *resultStates = states;
			resultStates[i].type = XR_TYPE_ACTION_STATE_BOOLEAN,
			resultStates[i].next = NULL;
			XrResult result = xrGetActionStateBoolean(
			    self->session, &getInfo, &resultStates[i]);
			if (!xr_result(
			        self->instance, result,
			        "failed to get boolean value for hand %d!", i))
				resultStates[i].isActive = false;
			break;
		}
		case XR_TYPE_ACTION_STATE_POSE: {
			XrActionStatePose *resultStates = states;
			resultStates[i].type = XR_TYPE_ACTION_STATE_POSE;
			resultStates[i].next = NULL;
			XrResult result = xrGetActionStatePose(
			    self->session, &getInfo, &resultStates[i]);
			if (!xr_result(self->instance, result,
			               "failed to get pose value for hand %d!",
			               i))
				resultStates[i].isActive = false;
			break;
		}

		default: return XR_ERROR_ACTION_TYPE_MISMATCH; // TOOD
		}
	}

	return XR_SUCCESS;
}

static bool
_suggestActions(xr_api *self,
                char *interaction_profile,
                XrAction *actions,
                XrPath **paths,
                int num_actions)
{
	XrPath interactionProfilePath;
	XrResult result = xrStringToPath(self->instance, interaction_profile,
	                                 &interactionProfilePath);
	if (!xr_result(self->instance, result,
	               "failed to get interaction profile path"))
		return false;

	int num_bindings = num_actions * HANDCOUNT;
	printf("Suggesting actions for %s, %d bindings\n", interaction_profile,
	       num_bindings);

	XrActionSuggestedBinding bindings[num_bindings];
	for (int action_count = 0; action_count < num_actions; action_count++) {
		for (int handCount = 0; handCount < HANDCOUNT; handCount++) {
			int binding_index =
			    action_count * HANDCOUNT + handCount;

			bindings[binding_index].action = actions[action_count];
			bindings[binding_index].binding =
			    paths[action_count][handCount];

#if 0
			for (int k = 0; k < LAST_ACTION_INDEX; k++) {
				if (self->actions[k] == actions[action_count]) {
					printf("Binding %d, Action %d => %d\n", binding_index, k, handCount);
				}
			}
#endif
		}
	}
	const XrInteractionProfileSuggestedBinding suggestedBindings = {
	    .type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING,
	    .next = NULL,
	    .interactionProfile = interactionProfilePath,
	    .countSuggestedBindings = num_bindings,
	    .suggestedBindings = bindings};

	xrSuggestInteractionProfileBindings(self->instance, &suggestedBindings);
	if (!xr_result(self->instance, result,
	               "failed to suggest simple bindings"))
		return false;

	return true;
}

static bool
_check_graphics_requirements_gl(xr_api *self, XrSystemId system_id)
{
	XrGraphicsRequirementsOpenGLKHR opengl_reqs = {
	    .type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR, .next = NULL};

	PFN_xrGetOpenGLGraphicsRequirementsKHR
	    pfnGetOpenGLGraphicsRequirementsKHR = NULL;
	XrResult result = xrGetInstanceProcAddr(
	    self->instance, "xrGetOpenGLGraphicsRequirementsKHR",
	    (PFN_xrVoidFunction *)&pfnGetOpenGLGraphicsRequirementsKHR);

	if (!xr_result(self->instance, result,
	               "Failed to get xrGetOpenGLGraphicsRequirementsKHR fp!"))
		return false;

	result = pfnGetOpenGLGraphicsRequirementsKHR(self->instance, system_id,
	                                             &opengl_reqs);

	if (!xr_result(self->instance, result,
	               "Failed to get OpenGL graphics requirements!"))
		return false;

	XrVersion desired_opengl_version = XR_MAKE_VERSION(3, 3, 0);
	if (desired_opengl_version > opengl_reqs.maxApiVersionSupported ||
	    desired_opengl_version < opengl_reqs.minApiVersionSupported) {
		printf(
		    "OpenXR Runtime only supports OpenGL version %d.%d "
		    "- %d.%d!\n",
		    XR_VERSION_MAJOR(opengl_reqs.minApiVersionSupported),
		    XR_VERSION_MINOR(opengl_reqs.minApiVersionSupported),
		    XR_VERSION_MAJOR(opengl_reqs.maxApiVersionSupported),
		    XR_VERSION_MINOR(opengl_reqs.maxApiVersionSupported));
		// it might still work
		return true;
	}
	return true;
}

OPENXR_API_HANDLE
init_openxr()
{
	xr_api *self = malloc(sizeof(xr_api));

	self->buffer_index = NULL;

	self->state = XR_SESSION_STATE_UNKNOWN;
	self->should_render = false;

	self->monado_stick_on_ball_ext = false;

	XrResult result;

	uint32_t extensionCount = 0;
	result = xrEnumerateInstanceExtensionProperties(NULL, 0,
	                                                &extensionCount, NULL);

	/* TODO: instance null will not be able to convert XrResult to string */
	if (!xr_result(NULL, result,
	               "Failed to enumerate number of extension properties"))
		return NULL;

	XrExtensionProperties extensionProperties[extensionCount];
	for (uint16_t i = 0; i < extensionCount; i++) {
		extensionProperties[i].type = XR_TYPE_EXTENSION_PROPERTIES;
		extensionProperties[i].next = NULL;
	}

	result = xrEnumerateInstanceExtensionProperties(
	    NULL, extensionCount, &extensionCount, extensionProperties);
	if (!xr_result(NULL, result,
	               "Failed to enumerate extension properties"))
		return NULL;

	if (!isExtensionSupported(XR_KHR_OPENGL_ENABLE_EXTENSION_NAME,
	                          extensionProperties, extensionCount)) {
		printf("Runtime does not support OpenGL extension!\n");
		return NULL;
	}

	if (isExtensionSupported(XR_MND_BALL_ON_STICK_EXTENSION_NAME,
	                         extensionProperties, extensionCount)) {
		self->monado_stick_on_ball_ext = true;
	}

	const char *enabledExtensions[extensionCount];

	int enabledExtensionCount = 0;
	enabledExtensions[enabledExtensionCount++] =
	    XR_KHR_OPENGL_ENABLE_EXTENSION_NAME;

	if (self->monado_stick_on_ball_ext) {
		enabledExtensions[enabledExtensionCount++] =
		    XR_MND_BALL_ON_STICK_EXTENSION_NAME;
	}

	XrInstanceCreateInfo instanceCreateInfo = {
	    .type = XR_TYPE_INSTANCE_CREATE_INFO,
	    .next = NULL,
	    .createFlags = 0,
	    .enabledExtensionCount = enabledExtensionCount,
	    .enabledExtensionNames = enabledExtensions,
	    .enabledApiLayerCount = 0,
	    .applicationInfo =
	        {
	            // TODO: get application name from godot
	            // TODO: establish godot version -> uint32_t versioning
	            .applicationName = "Godot OpenXR Plugin",
	            .engineName = "Godot Engine",
	            .applicationVersion = 1,
	            .engineVersion = 0,
	            .apiVersion = XR_CURRENT_API_VERSION,
	        },
	};
	result = xrCreateInstance(&instanceCreateInfo, &self->instance);
	if (!xr_result(NULL, result, "Failed to create XR instance."))
		return NULL;

	XrSystemGetInfo systemGetInfo = {
	    .type = XR_TYPE_SYSTEM_GET_INFO,
	    .formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY,
	    .next = NULL};

	XrSystemId systemId;
	result = xrGetSystem(self->instance, &systemGetInfo, &systemId);
	if (!xr_result(self->instance, result,
	               "Failed to get system for HMD form factor."))
		return NULL;

	XrSystemProperties systemProperties = {
	    .type = XR_TYPE_SYSTEM_PROPERTIES,
	    .next = NULL,
	    .graphicsProperties = {0},
	    .trackingProperties = {0},
	};
	result =
	    xrGetSystemProperties(self->instance, systemId, &systemProperties);
	if (!xr_result(self->instance, result,
	               "Failed to get System properties"))
		return NULL;

	XrViewConfigurationType viewConfigType =
	    XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
	if (!isViewConfigSupported(self->instance, viewConfigType, systemId)) {
		printf("Stereo View Configuration not supported!");
		return NULL;
	}

	result = xrEnumerateViewConfigurationViews(self->instance, systemId,
	                                           viewConfigType, 0,
	                                           &self->view_count, NULL);
	if (!xr_result(self->instance, result,
	               "Failed to get view configuration view count!"))
		return NULL;

	self->configuration_views =
	    malloc(sizeof(XrViewConfigurationView) * self->view_count);

	result = xrEnumerateViewConfigurationViews(
	    self->instance, systemId, viewConfigType, self->view_count,
	    &self->view_count, self->configuration_views);
	if (!xr_result(self->instance, result,
	               "Failed to enumerate view configuration views!"))
		return NULL;

	self->buffer_index = malloc(sizeof(uint32_t) * self->view_count);

	if (!_check_graphics_requirements_gl(self, systemId))
		return NULL;

	// TODO: support wayland
	// TODO: support windows
	// TODO: maybe support xcb separately?
	// TODO: support vulkan
	self->graphics_binding_gl = (XrGraphicsBindingOpenGLXlibKHR){
	    .type = XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR,
	};

	// TODO: get this from godot engine
	self->graphics_binding_gl.xDisplay = XOpenDisplay(NULL);
	self->graphics_binding_gl.glxContext = glXGetCurrentContext();
	self->graphics_binding_gl.glxDrawable = glXGetCurrentDrawable();

	printf("Graphics: Display %p, Context %" PRIxPTR ", Drawable %" PRIxPTR
	       "\n",
	       self->graphics_binding_gl.xDisplay,
	       (uintptr_t)self->graphics_binding_gl.glxContext,
	       (uintptr_t)self->graphics_binding_gl.glxDrawable);

	printf("Using OpenGL version: %s\n", glGetString(GL_VERSION));
	printf("Using OpenGL Renderer: %s\n", glGetString(GL_RENDERER));

	XrSessionCreateInfo session_create_info = {
	    .type = XR_TYPE_SESSION_CREATE_INFO,
	    .next = &self->graphics_binding_gl,
	    .systemId = systemId};

	result = xrCreateSession(self->instance, &session_create_info,
	                         &self->session);
	if (!xr_result(self->instance, result, "Failed to create session"))
		return NULL;

	XrReferenceSpaceType playSpace = XR_REFERENCE_SPACE_TYPE_LOCAL;
	if (!isReferenceSpaceSupported(self->instance, self->session,
	                               playSpace)) {
		printf("runtime does not support local space!\n");
		return NULL;
	}

	XrPosef identityPose = {
	    .orientation = {.x = 0, .y = 0, .z = 0, .w = 1.0},
	    .position = {.x = 0, .y = 0, .z = 0}};

	XrReferenceSpaceCreateInfo localSpaceCreateInfo = {
	    .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
	    .next = NULL,
	    .referenceSpaceType = playSpace,
	    .poseInReferenceSpace = identityPose};

	result = xrCreateReferenceSpace(self->session, &localSpaceCreateInfo,
	                                &self->local_space);
	if (!xr_result(self->instance, result, "Failed to create local space!"))
		return NULL;

	XrSessionBeginInfo sessionBeginInfo = {
	    .type = XR_TYPE_SESSION_BEGIN_INFO,
	    .next = NULL,
	    .primaryViewConfigurationType = viewConfigType};
	result = xrBeginSession(self->session, &sessionBeginInfo);
	if (!xr_result(self->instance, result, "Failed to begin session!"))
		return NULL;

	uint32_t swapchainFormatCount;
	result = xrEnumerateSwapchainFormats(self->session, 0,
	                                     &swapchainFormatCount, NULL);
	if (!xr_result(self->instance, result,
	               "Failed to get number of supported swapchain formats"))
		return NULL;

	int64_t swapchainFormats[swapchainFormatCount];
	result = xrEnumerateSwapchainFormats(
	    self->session, swapchainFormatCount, &swapchainFormatCount,
	    swapchainFormats);
	if (!xr_result(self->instance, result,
	               "Failed to enumerate swapchain formats"))
		return NULL;

	const bool SRGB_SWAPCHAIN = true;

	int64_t swapchainFormatToUse = swapchainFormats[0];

	printf("Swapchain Formats\n");
	for (int i = 0; i < swapchainFormatCount; i++) {
		printf("%lX\n", swapchainFormats[i]);
		if (SRGB_SWAPCHAIN &&
		    swapchainFormats[i] == GL_SRGB8_ALPHA8_EXT) {
			swapchainFormatToUse = swapchainFormats[i];
			printf("Using SRGB swapchain!\n");
		}
		if (!SRGB_SWAPCHAIN && swapchainFormats[i] == GL_RGBA8_EXT) {
			swapchainFormatToUse = swapchainFormats[i];
			printf("Using RGBA swapchain!\n");
		}
	}

	self->swapchains = malloc(sizeof(XrSwapchain) * self->view_count);
	uint32_t swapchainLength[self->view_count];
	for (uint32_t i = 0; i < self->view_count; i++) {
		XrSwapchainCreateInfo swapchainCreateInfo = {
		    .type = XR_TYPE_SWAPCHAIN_CREATE_INFO,
		    .usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT |
		                  XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT,
		    .createFlags = 0,
		    .format = swapchainFormatToUse,
		    .sampleCount = 1,
		    .width =
		        self->configuration_views[i].recommendedImageRectWidth,
		    .height =
		        self->configuration_views[i].recommendedImageRectHeight,
		    .faceCount = 1,
		    .arraySize = 1,
		    .mipCount = 1,
		    .next = NULL,
		};
		result = xrCreateSwapchain(self->session, &swapchainCreateInfo,
		                           &self->swapchains[i]);
		if (!xr_result(self->instance, result,
		               "Failed to create swapchain %d!", i))
			return NULL;
		result = xrEnumerateSwapchainImages(self->swapchains[i], 0,
		                                    &swapchainLength[i], NULL);
		if (!xr_result(self->instance, result,
		               "Failed to enumerate swapchains"))
			return NULL;
	}

	uint32_t maxSwapchainLength = 0;
	for (uint32_t i = 0; i < self->view_count; i++) {
		if (swapchainLength[i] > maxSwapchainLength) {
			maxSwapchainLength = swapchainLength[i];
		}
	}

	self->images =
	    malloc(sizeof(XrSwapchainImageOpenGLKHR *) * self->view_count);
	for (uint32_t i = 0; i < self->view_count; i++) {
		self->images[i] = malloc(sizeof(XrSwapchainImageOpenGLKHR) *
		                         maxSwapchainLength);

		for (int j = 0; j < maxSwapchainLength; j++) {
			self->images[i][j].type =
			    XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR;
			self->images[i][j].next = NULL;
		}
	}

	for (uint32_t i = 0; i < self->view_count; i++) {
		result = xrEnumerateSwapchainImages(
		    self->swapchains[i], swapchainLength[i],
		    &swapchainLength[i],
		    (XrSwapchainImageBaseHeader *)self->images[i]);
		if (!xr_result(self->instance, result,
		               "Failed to enumerate swapchain images"))
			return NULL;
	}

	// only used for OpenGL depth testing
	/*
	glGenTextures(1, &self->depthbuffer);
	glBindTexture(GL_TEXTURE_2D, self->depthbuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24,
	             self->configuration_views[0].recommendedImageRectWidth,
	             self->configuration_views[0].recommendedImageRectHeight, 0,
	             GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, 0);
      */

	self->projectionLayer = malloc(sizeof(XrCompositionLayerProjection));
	self->projectionLayer->type = XR_TYPE_COMPOSITION_LAYER_PROJECTION;
	self->projectionLayer->next = NULL;
	self->projectionLayer->layerFlags = 0;
	self->projectionLayer->space = self->local_space;
	self->projectionLayer->viewCount = self->view_count;
	self->projectionLayer->views = NULL;

	self->frameState.type = XR_TYPE_FRAME_STATE;
	self->frameState.next = NULL;

	self->running = true;

	self->views = malloc(sizeof(XrView) * self->view_count);
	self->projection_views =
	    malloc(sizeof(XrCompositionLayerProjectionView) * self->view_count);
	for (uint32_t i = 0; i < self->view_count; i++) {
		self->views[i].type = XR_TYPE_VIEW;
		self->views[i].next = NULL;

		self->projection_views[i].type =
		    XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
		self->projection_views[i].next = NULL;
		self->projection_views[i].subImage.swapchain =
		    self->swapchains[i];
		self->projection_views[i].subImage.imageArrayIndex = 0;
		self->projection_views[i].subImage.imageRect.offset.x = 0;
		self->projection_views[i].subImage.imageRect.offset.y = 0;
		self->projection_views[i].subImage.imageRect.extent.width =
		    self->configuration_views[i].recommendedImageRectWidth;
		self->projection_views[i].subImage.imageRect.extent.height =
		    self->configuration_views[i].recommendedImageRectHeight;
	};

	XrActionSetCreateInfo actionSetInfo = {
	    .type = XR_TYPE_ACTION_SET_CREATE_INFO,
	    .next = NULL,
	    .priority = 0};
	strcpy(actionSetInfo.actionSetName, "godotset");
	strcpy(actionSetInfo.localizedActionSetName,
	       "Action Set Used by Godot");

	result =
	    xrCreateActionSet(self->instance, &actionSetInfo, &self->actionSet);
	if (!xr_result(self->instance, result, "failed to create actionset"))
		return NULL;

	xrStringToPath(self->instance, "/user/hand/left",
	               &self->handPaths[HAND_LEFT]);
	xrStringToPath(self->instance, "/user/hand/right",
	               &self->handPaths[HAND_RIGHT]);

	// TODO: add action editor to godot and create actions dynamically
	self->actions[TRIGGER_ACTION_INDEX] = _createAction(
	    self, XR_ACTION_TYPE_FLOAT_INPUT, "trigger", "Trigger Button");
	if (self->actions[TRIGGER_ACTION_INDEX] == NULL)
		return NULL;

	self->actions[GRAB_ACTION_INDEX] = _createAction(
	    self, XR_ACTION_TYPE_BOOLEAN_INPUT, "grab", "Grab Button");
	if (self->actions[GRAB_ACTION_INDEX] == NULL)
		return NULL;

	self->actions[MENU_ACTION_INDEX] = _createAction(
	    self, XR_ACTION_TYPE_BOOLEAN_INPUT, "menu", "Menu Button");
	if (self->actions[GRAB_ACTION_INDEX] == NULL)
		return NULL;

	self->actions[POSE_ACTION_INDEX] = _createAction(
	    self, XR_ACTION_TYPE_POSE_INPUT, "handpose", "Hand Pose");
	if (self->actions[POSE_ACTION_INDEX] == NULL)
		return NULL;

	XrPath selectClickPath[HANDCOUNT];
	xrStringToPath(self->instance, "/user/hand/left/input/select/click",
	               &selectClickPath[HAND_LEFT]);
	xrStringToPath(self->instance, "/user/hand/right/input/select/click",
	               &selectClickPath[HAND_RIGHT]);

	XrPath aimPosePath[HANDCOUNT];
	xrStringToPath(self->instance, "/user/hand/left/input/aim/pose",
	               &aimPosePath[HAND_LEFT]);
	xrStringToPath(self->instance, "/user/hand/right/input/aim/pose",
	               &aimPosePath[HAND_RIGHT]);

	XrPath triggerPath[HANDCOUNT];
	xrStringToPath(self->instance, "/user/hand/left/input/trigger",
	               &triggerPath[HAND_LEFT]);
	xrStringToPath(self->instance, "/user/hand/right/input/trigger",
	               &triggerPath[HAND_RIGHT]);

	XrPath menuPath[HANDCOUNT];
	xrStringToPath(self->instance, "/user/hand/left/input/menu/click",
	               &menuPath[HAND_LEFT]);
	xrStringToPath(self->instance, "/user/hand/right/input/menu/click",
	               &menuPath[HAND_RIGHT]);

	XrPath aPath[HANDCOUNT];
	xrStringToPath(self->instance, "/user/hand/left/input/a/click",
	               &aPath[HAND_LEFT]);
	xrStringToPath(self->instance, "/user/hand/right/input/a/click",
	               &aPath[HAND_RIGHT]);

	XrPath bPath[HANDCOUNT];
	xrStringToPath(self->instance, "/user/hand/left/input/b/click",
	               &bPath[HAND_LEFT]);
	xrStringToPath(self->instance, "/user/hand/right/input/b/click",
	               &bPath[HAND_RIGHT]);

	// khr simple controller
	{
		XrAction actions[] = {self->actions[POSE_ACTION_INDEX],
		                      self->actions[TRIGGER_ACTION_INDEX]};
		XrPath *paths[] = {aimPosePath, selectClickPath};
		int num_actions = sizeof(actions) / sizeof(actions[0]);
		if (!_suggestActions(
		        self, "/interaction_profiles/khr/simple_controller",
		        actions, paths, num_actions))
			return NULL;
	}

	// valve index controller
	{
		XrAction actions[] = {
		    self->actions[POSE_ACTION_INDEX],
		    self->actions[TRIGGER_ACTION_INDEX],
		    self->actions[GRAB_ACTION_INDEX],
		    self->actions[MENU_ACTION_INDEX],
		};
		XrPath *paths[] = {aimPosePath, triggerPath, aPath, bPath};
		int num_actions = sizeof(actions) / sizeof(actions[0]);
		if (!_suggestActions(
		        self, "/interaction_profiles/valve/index_controller",
		        actions, paths, num_actions))
			return NULL;
	}

	// monado ext: ball on stick controller (psmv)
	if (/* TODO: remove when ext exists */ true ||
	    self->monado_stick_on_ball_ext) {
		XrPath squarePath[HANDCOUNT];
		xrStringToPath(self->instance,
		               "/user/hand/left/input/square_mnd/click",
		               &squarePath[HAND_LEFT]);
		xrStringToPath(self->instance,
		               "/user/hand/right/input/square_mnd/click",
		               &squarePath[HAND_RIGHT]);

		XrAction actions[] = {
		    self->actions[POSE_ACTION_INDEX],
		    self->actions[TRIGGER_ACTION_INDEX],
		    self->actions[GRAB_ACTION_INDEX],
		    self->actions[MENU_ACTION_INDEX],
		};
		XrPath *paths[] = {aimPosePath, triggerPath, squarePath,
		                   menuPath};
		int num_actions = sizeof(actions) / sizeof(actions[0]);
		if (!_suggestActions(
		        self,
		        "/interaction_profiles/mnd/ball_on_stick_controller",
		        actions, paths, num_actions))
			return NULL;
	}

	XrActionSpaceCreateInfo actionSpaceInfo = {
	    .type = XR_TYPE_ACTION_SPACE_CREATE_INFO,
	    .next = NULL,
	    .action = self->actions[POSE_ACTION_INDEX],
	    .poseInActionSpace.orientation.w = 1.f,
	    .subactionPath = self->handPaths[0]};

	result = xrCreateActionSpace(self->session, &actionSpaceInfo,
	                             &self->handSpaces[0]);
	if (!xr_result(self->instance, result,
	               "failed to create left hand pose space"))
		return NULL;

	actionSpaceInfo.subactionPath = self->handPaths[1];
	result = xrCreateActionSpace(self->session, &actionSpaceInfo,
	                             &self->handSpaces[1]);
	if (!xr_result(self->instance, result,
	               "failed to create right hand pose space"))
		return NULL;

	XrSessionActionSetsAttachInfo attachInfo = {
	    .type = XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO,
	    .next = NULL,
	    .countActionSets = 1,
	    .actionSets = &self->actionSet};
	result = xrAttachSessionActionSets(self->session, &attachInfo);
	if (!xr_result(self->instance, result, "failed to attach action set"))
		return NULL;

	self->godot_controllers[0] =
	    arvr_api->godot_arvr_add_controller("lefthand", 1, true, true);
	self->godot_controllers[1] =
	    arvr_api->godot_arvr_add_controller("righthand", 2, true, true);

	printf("initialized controllers %d %d\n", self->godot_controllers[0],
	       self->godot_controllers[1]);

	return (OPENXR_API_HANDLE)self;
}

void
render_openxr(OPENXR_API_HANDLE _self,
              int eye,
              uint32_t texid,
              bool has_external_texture_support)
{
	xr_api *self = (xr_api *)_self;

	// printf("Render eye %d texture %d\n", eye, texid);
	XrResult result;

	// TODO: save resources in some states where we don't need to do
	// anything
	if (!self->running || self->state >= XR_SESSION_STATE_STOPPING)
		return;

	self->projection_views[eye].fov = self->views[eye].fov;
	self->projection_views[eye].pose = self->views[eye].pose;

	XrSwapchainImageReleaseInfo swapchainImageReleaseInfo = {
	    .type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO, .next = NULL};
	result = xrReleaseSwapchainImage(self->swapchains[eye],
	                                 &swapchainImageReleaseInfo);
	if (!xr_result(self->instance, result,
	               "failed to release swapchain image!"))
		return;

	if (!has_external_texture_support) {
		glBindTexture(GL_TEXTURE_2D, texid);
		glCopyTextureSubImage2D(
		    self->images[eye][self->buffer_index[eye]].image, 0, 0, 0,
		    0, 0,
		    self->configuration_views[eye].recommendedImageRectWidth,
		    self->configuration_views[eye].recommendedImageRectHeight);
		glBindTexture(GL_TEXTURE_2D, 0);
		// printf("Copy godot texture %d into XR texture %d\n", texid,
		// self->images[eye][bufferIndex].image);
	} else {
		// printf("Godot already rendered into our textures\n");
	}

	if (eye == 1) {
		self->projectionLayer->views = self->projection_views;

		const XrCompositionLayerBaseHeader *const projectionlayers[1] =
		    {(const XrCompositionLayerBaseHeader *const)
		         self->projectionLayer};
		XrFrameEndInfo frameEndInfo = {
		    .type = XR_TYPE_FRAME_END_INFO,
		    .displayTime = self->frameState.predictedDisplayTime,
		    .layerCount = 1,
		    .layers = projectionlayers,
		    .environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
		    .next = NULL};
		result = xrEndFrame(self->session, &frameEndInfo);
		if (!xr_result(self->instance, result, "failed to end frame!"))
			return;
	}
}

void
fill_projection_matrix(OPENXR_API_HANDLE _self, int eye, XrMatrix4x4f *matrix)
{
	xr_api *self = (xr_api *)_self;
	XrView views[self->view_count];
	for (uint32_t i = 0; i < self->view_count; i++) {
		views[i].type = XR_TYPE_VIEW;
		views[i].next = NULL;
	};

	XrViewLocateInfo viewLocateInfo = {
	    .type = XR_TYPE_VIEW_LOCATE_INFO,
	    .displayTime = self->frameState.predictedDisplayTime,
	    .space = self->local_space};

	XrViewState viewState = {.type = XR_TYPE_VIEW_STATE, .next = NULL};
	uint32_t viewCountOutput;
	XrResult result =
	    xrLocateViews(self->session, &viewLocateInfo, &viewState,
	                  self->view_count, &viewCountOutput, views);

	// printf("FOV %f %f %f %f\n", views[eye].fov.angleLeft,
	// views[eye].fov.angleRight, views[eye].fov.angleUp,
	// views[eye].fov.angleDown);

	if (!xr_result(self->instance, result, "Could not locate views")) {
		printf("Locate Views failed??\n");
	} else {
		XrMatrix4x4f_CreateProjectionFov(matrix, GRAPHICS_OPENGL,
		                                 views[eye].fov, 0.05f, 100.0f);
		// printf("Fill projection matrix for eye %d / %d\n", eye,
		// self->view_count
		// - 1);
	}
}

bool
_transform_from_rot_pos(godot_transform *p_dest,
                        XrSpaceLocation *location,
                        float p_world_scale)
{
	godot_quat q;
	godot_basis basis;
	godot_vector3 origin;

	if (location->pose.orientation.x == 0 &&
	    location->pose.orientation.y == 0 &&
	    location->pose.orientation.z == 0 &&
	    location->pose.orientation.w == 0)
		return false;

	// convert orientation quad to position, should add helper function for
	// this
	// :)
	api->godot_quat_new(
	    &q, location->pose.orientation.x, location->pose.orientation.y,
	    location->pose.orientation.z, location->pose.orientation.w);
	api->godot_basis_new_with_euler_quat(&basis, &q);

	api->godot_vector3_new(&origin,
	                       location->pose.position.x * p_world_scale,
	                       location->pose.position.y * p_world_scale,
	                       location->pose.position.z * p_world_scale);
	api->godot_transform_new(p_dest, &basis, &origin);

	return true;
};

void
update_controllers(OPENXR_API_HANDLE _self)
{
	xr_api *self = (xr_api *)_self;
	XrResult result;

	const XrActiveActionSet activeActionSet = {
	    .actionSet = self->actionSet, .subactionPath = XR_NULL_PATH};

	XrActionsSyncInfo syncInfo = {.type = XR_TYPE_ACTIONS_SYNC_INFO,
	                              .countActiveActionSets = 1,
	                              .activeActionSets = &activeActionSet};
	result = xrSyncActions(self->session, &syncInfo);
	xr_result(self->instance, result, "failed to sync actions!");

	XrActionStateFloat triggerStates[HANDCOUNT];
	_getActionStates(self, self->actions[TRIGGER_ACTION_INDEX],
	                 XR_TYPE_ACTION_STATE_FLOAT, (void **)triggerStates);

	XrActionStateBoolean grabStates[HANDCOUNT];
	_getActionStates(self, self->actions[GRAB_ACTION_INDEX],
	                 XR_TYPE_ACTION_STATE_BOOLEAN, (void **)grabStates);

	XrActionStateBoolean menuStates[HANDCOUNT];
	_getActionStates(self, self->actions[MENU_ACTION_INDEX],
	                 XR_TYPE_ACTION_STATE_BOOLEAN, (void **)menuStates);

	XrActionStatePose poseStates[HANDCOUNT];
	_getActionStates(self, self->actions[POSE_ACTION_INDEX],
	                 XR_TYPE_ACTION_STATE_POSE, (void **)poseStates);

	XrSpaceLocation spaceLocation[HANDCOUNT];

	for (int i = 0; i < HANDCOUNT; i++) {
		if (!poseStates[i].isActive) {
			// printf("Pose for hand %d is not active %d\n", i,
			// poseStates[i].isActive);
			continue;
		}

		spaceLocation[i].type = XR_TYPE_SPACE_LOCATION;
		spaceLocation[i].next = NULL;

		result = xrLocateSpace(self->handSpaces[i], self->local_space,
		                       self->frameState.predictedDisplayTime,
		                       &spaceLocation[i]);
		xr_result(self->instance, result, "failed to locate space %d!",
		          i);
		bool spaceLocationValid =
		    //(spaceLocation[i].locationFlags &
		    // XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
		    (spaceLocation[i].locationFlags &
		     XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0;

		godot_transform controller_transform;
		if (!spaceLocationValid) {
			printf("Space location not valid for hand %d\n", i);
			continue;
		} else {
			if (!_transform_from_rot_pos(&controller_transform,
			                             &spaceLocation[i], 1.0)) {
				printf(
				    "Pose for hand %d is active but invalid\n",
				    i);
				continue;
			}
		}

#if 0
		printf("pose for controller %d - %f %f %f - %f %f %f %f\n", i,
			spaceLocation[i].pose.position.x, spaceLocation[i].pose.position.y, spaceLocation[i].pose.position.z,
			spaceLocation[i].pose.orientation.x, spaceLocation[i].pose.orientation.y, spaceLocation[i].pose.orientation.z, spaceLocation[i].pose.orientation.w
		);
#endif

		arvr_api->godot_arvr_set_controller_transform(
		    self->godot_controllers[i], &controller_transform, true,
		    true);

		// TODO: dynamic binding
		const int triggerButton = 15;
		const int grabButton = 2;
		const int menuButton = 1;

#if DEBUG_INPUT
		printf("%d: trigger active %d changed %d state %f\n", i,
		       triggerStates[i].isActive,
		       triggerStates[i].changedSinceLastSync,
		       triggerStates[i].currentState);

		printf("%d: grab active %d changed %d state %d\n", i,
		       grabStates[i].isActive,
		       grabStates[i].changedSinceLastSync,
		       grabStates[i].currentState);

		printf("%d: menu active %d changed %d state %d\n", i,
		       menuStates[i].isActive,
		       menuStates[i].changedSinceLastSync,
		       menuStates[i].currentState);
#endif

		if (triggerStates[i].isActive &&
		    triggerStates[i].changedSinceLastSync) {
			arvr_api->godot_arvr_set_controller_button(
			    self->godot_controllers[i], triggerButton,
			    triggerStates[i].currentState);
		}
		if (grabStates[i].isActive &&
		    grabStates[i].changedSinceLastSync) {
			arvr_api->godot_arvr_set_controller_button(
			    self->godot_controllers[i], grabButton,
			    grabStates[i].currentState);
		}
		if (menuStates[i].isActive &&
		    menuStates[i].changedSinceLastSync) {
			arvr_api->godot_arvr_set_controller_button(
			    self->godot_controllers[i], menuButton,
			    menuStates[i].currentState);
		}
	};
}

void
recommended_rendertarget_size(OPENXR_API_HANDLE _self,
                              uint32_t *width,
                              uint32_t *height)
{
	xr_api *self = (xr_api *)_self;
	*width = self->configuration_views[0].recommendedImageRectWidth;
	*height = self->configuration_views[0].recommendedImageRectHeight;
}

bool
get_view_matrix(OPENXR_API_HANDLE _self, int eye, XrMatrix4x4f *matrix)
{
	xr_api *self = (xr_api *)_self;
	if (self->views == NULL)
		return false;
	const XrVector3f uniformScale = {.x = 1.f, .y = 1.f, .z = 1.f};

	XrMatrix4x4f viewMatrix;
	XrMatrix4x4f_CreateTranslationRotationScaleOrbit(
	    &viewMatrix, &self->views[eye].pose.position,
	    &self->views[eye].pose.orientation, &uniformScale);

	XrMatrix4x4f_InvertRigidBody(matrix, &viewMatrix);
	return true;
}

int
get_external_texture_for_eye(OPENXR_API_HANDLE _self,
                             int eye,
                             bool *has_support)
{
	xr_api *self = (xr_api *)_self;
	// this only gets called from Godot 3.2 and newer, allows us to use
	// OpenXR swapchain directly.

	// process should be called by now but just in case...
	if (self->state > XR_SESSION_STATE_UNKNOWN &&
	    self->buffer_index != NULL) {
		// make sure we know that we're rendering directly to our
		// texture chain
		*has_support = true;
		// printf("eye %d: get texture %d\n", eye,
		// self->buffer_index[eye]);
		return self->images[eye][self->buffer_index[eye]].image;
	}

	return 0;
}

void
process_openxr(OPENXR_API_HANDLE _self)
{
	xr_api *self = (xr_api *)_self;

	XrResult result;

	XrEventDataBuffer runtimeEvent = {.type = XR_TYPE_EVENT_DATA_BUFFER,
	                                  .next = NULL};
	XrResult pollResult = xrPollEvent(self->instance, &runtimeEvent);
	while (pollResult == XR_SUCCESS) {
		switch (runtimeEvent.type) {
		case XR_TYPE_EVENT_DATA_EVENTS_LOST: {
			XrEventDataEventsLost *event =
			    (XrEventDataEventsLost *)&runtimeEvent;

			printf("EVENT: %d event data lost!\n",
			       event->lostEventCount);
			// we probably didn't poll fast enough'
		} break;
		case XR_TYPE_EVENT_DATA_VISIBILITY_MASK_CHANGED_KHR: {
			XrEventDataVisibilityMaskChangedKHR *event =
			    (XrEventDataVisibilityMaskChangedKHR
			         *)&runtimeEvent;
			printf("EVENT: STUB: visibility mask changed\n");
		} break;
		case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: {
			XrEventDataInstanceLossPending *event =
			    (XrEventDataInstanceLossPending *)&runtimeEvent;
			printf("EVENT: instance loss pending at %lu!\n",
			       event->lossTime);
			self->running = false;
			return;
		} break;
		case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
			printf("EVENT: session state changed ");
			XrEventDataSessionStateChanged *event =
			    (XrEventDataSessionStateChanged *)&runtimeEvent;
			XrSessionState state = event->state;

			self->state = event->state;
			printf("to %d", state);
			if (event->state >= XR_SESSION_STATE_STOPPING) {
				printf("\nAbort Mission!\n");
				self->running = false;
				return;
			}
			printf("\n");
		} break;
		case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING: {
			XrEventDataReferenceSpaceChangePending *event =
			    (XrEventDataReferenceSpaceChangePending
			         *)&runtimeEvent;
			printf(
			    "EVENT: reference space type %d change pending!\n",
			    event->referenceSpaceType);
			// TODO: do something
		} break;
		case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED: {
			printf("EVENT: interaction profile changed!\n");
			XrEventDataInteractionProfileChanged *event =
			    (XrEventDataInteractionProfileChanged
			         *)&runtimeEvent;

			XrInteractionProfileState state = {
			    .type = XR_TYPE_INTERACTION_PROFILE_STATE};

			XrPath hand_paths[2];
			xrStringToPath(self->instance, "/user/hand/left",
			               &hand_paths[0]);
			xrStringToPath(self->instance, "/user/hand/right",
			               &hand_paths[1]);
			for (int i = 0; i < 2; i++) {
				XrResult res = xrGetCurrentInteractionProfile(
				    self->session, hand_paths[i], &state);
				if (!xr_result(self->instance, res,
				               "Failed to get interaction "
				               "profile for %d",
				               i))
					continue;

				XrPath prof = state.interactionProfile;

				uint32_t strl;
				char profile_str[XR_MAX_PATH_LENGTH];
				res = xrPathToString(self->instance, prof,
				                     XR_MAX_PATH_LENGTH, &strl,
				                     profile_str);
				if (!xr_result(self->instance, res,
				               "Failed to get interaction "
				               "profile path str for %s",
				               i == 0 ? "/user/hand/left"
				                      : "/user/hand/right"))
					continue;

				printf(
				    "Event: Interaction profile changed for "
				    "%s: %s\n",
				    i == 0 ? "/user/hand/left"
				           : "/user/hand/right",
				    profile_str);
			}

			// TODO: do something
		} break;
		default:
			printf("Unhandled event type %d\n", runtimeEvent.type);
			break;
		}

		runtimeEvent.type = XR_TYPE_EVENT_DATA_BUFFER;
		pollResult = xrPollEvent(self->instance, &runtimeEvent);
	}
	if (pollResult == XR_EVENT_UNAVAILABLE) {
		// processed all events in the queue
	} else {
		printf("Failed to poll events!\n");
		return;
	}

	XrFrameWaitInfo frameWaitInfo = {.type = XR_TYPE_FRAME_WAIT_INFO,
	                                 .next = NULL};
	result = xrWaitFrame(self->session, &frameWaitInfo, &self->frameState);
	if (!xr_result(self->instance, result,
	               "xrWaitFrame() was not successful, exiting..."))
		return;

	update_controllers(self);

	XrViewLocateInfo viewLocateInfo = {
	    .type = XR_TYPE_VIEW_LOCATE_INFO,
	    .next = NULL,
	    .viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
	    .displayTime = self->frameState.predictedDisplayTime,
	    .space = self->local_space};
	XrViewState viewState = {.type = XR_TYPE_VIEW_STATE, .next = NULL};
	uint32_t viewCountOutput;
	result = xrLocateViews(self->session, &viewLocateInfo, &viewState,
	                       self->view_count, &viewCountOutput, self->views);
	if (!xr_result(self->instance, result, "Could not locate views"))
		return;

	XrFrameBeginInfo frameBeginInfo = {.type = XR_TYPE_FRAME_BEGIN_INFO,
	                                   .next = NULL};

	result = xrBeginFrame(self->session, &frameBeginInfo);
	if (!xr_result(self->instance, result, "failed to begin frame!"))
		return;

	for (int eye = 0; eye < 2; eye++) {
		XrSwapchainImageAcquireInfo swapchainImageAcquireInfo = {
		    .type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO, .next = NULL};
		result = xrAcquireSwapchainImage(self->swapchains[eye],
		                                 &swapchainImageAcquireInfo,
		                                 &self->buffer_index[eye]);
		if (!xr_result(self->instance, result,
		               "failed to acquire swapchain image!"))
			return;

		XrSwapchainImageWaitInfo swapchainImageWaitInfo = {
		    .type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO,
		    .next = NULL,
		    .timeout = 0};
		result = xrWaitSwapchainImage(self->swapchains[eye],
		                              &swapchainImageWaitInfo);
		if (!xr_result(self->instance, result,
		               "failed to wait for swapchain image!"))
			return;
	}
}
