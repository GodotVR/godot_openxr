////////////////////////////////////////////////////////////////////////////////////////////////
// Helper calls and singleton container for accessing openxr

#include "OpenXRApi.h"

OpenXRApi * OpenXRApi::singleton = NULL;

void OpenXRApi::openxr_release_api() {
	if (singleton == NULL) {
		// nothing to release
		printf("OpenXR: tried to release non-existent OpenXR context\n");
	} else if (singleton->use_count > 1) {
		// decrease use count
		singleton->use_count--;
		printf("OpenXR: decreased use count to %i\n", singleton->use_count);
	} else {
		// cleanup openxr
		printf("OpenXR: releasing OpenXR context\n");

		delete singleton;
		singleton = NULL;
	};
};

OpenXRApi * OpenXRApi::openxr_get_api() {
	if (singleton != NULL) {
		// increase use count
		singleton->use_count++;
		printf("OpenXR: increased use count to %i\n", singleton->use_count);
	} else {
		// init openxr
		printf("OpenXR: initialising OpenXR context\n");

		singleton = new OpenXRApi();
		if (singleton == NULL) {
			printf("OpenXR init failed\n");
		} else {
			printf("OpenXR init succeeded\n");
		}
	}

	return singleton;
};

bool OpenXRApi::xr_result(XrResult result, const char *format, ...) {
	if (XR_SUCCEEDED(result))
		return true;

	char resultString[XR_MAX_RESULT_STRING_SIZE];
	xrResultToString(instance, result, resultString);

	size_t len1 = strlen(format);
	size_t len2 = strlen(resultString) + 1;

	// Damn you microsoft for not supporting this!!
	// char formatRes[len1 + len2 + 5]; // + " []\n"
	char *formatRes = (char *)malloc(len1 + len2 + 5);
	sprintf(formatRes, "%s [%s]\n", format, resultString);

	va_list args;
	va_start(args, format);
	vprintf(formatRes, args);
	va_end(args);

	free(formatRes);

	return false;
}

bool OpenXRApi::isExtensionSupported(char *extensionName, XrExtensionProperties *instanceExtensionProperties, uint32_t instanceExtensionCount) {
	for (uint32_t supportedIndex = 0; supportedIndex < instanceExtensionCount; supportedIndex++) {
		if (!strcmp(extensionName, instanceExtensionProperties[supportedIndex].extensionName)) {
			return true;
		}
	}
	return false;
}

bool OpenXRApi::isViewConfigSupported(XrViewConfigurationType type, XrSystemId systemId) {
	XrResult result;
	uint32_t viewConfigurationCount;

	result = xrEnumerateViewConfigurations(instance, systemId, 0, &viewConfigurationCount, NULL);
	if (!xr_result(result, "Failed to get view configuration count")) {
		return false;
	}

	// Damn you microsoft for not supporting this!!
	// XrViewConfigurationType viewConfigurations[viewConfigurationCount];
	XrViewConfigurationType * viewConfigurations = (XrViewConfigurationType *)malloc(sizeof(XrViewConfigurationType) * viewConfigurationCount);
	if (viewConfigurations == NULL) {
		printf("Couldn''t allocate memory for view configurations\n");
		return false;
	}

	result = xrEnumerateViewConfigurations(instance, systemId, viewConfigurationCount, &viewConfigurationCount, viewConfigurations);
	if (!xr_result(result, "Failed to enumerate view configurations!")) {
		free(viewConfigurations);
		return false;
	}

	for (uint32_t i = 0; i < viewConfigurationCount; ++i) {
		if (viewConfigurations[i] == type) {
			free(viewConfigurations);
			return true;
		}
	}

	free(viewConfigurations);
	return false;
}

bool OpenXRApi::isReferenceSpaceSupported(XrReferenceSpaceType type) {
	XrResult result;
	uint32_t referenceSpacesCount;

	result = xrEnumerateReferenceSpaces(session, 0, &referenceSpacesCount, NULL);
	if (!xr_result(result, "Getting number of reference spaces failed!")) {
		return 1;
	}

	// Damn you microsoft for not supporting this!!
	// XrReferenceSpaceType referenceSpaces[referenceSpacesCount];
	XrReferenceSpaceType * referenceSpaces = (XrReferenceSpaceType *) malloc(sizeof(XrReferenceSpaceType) * referenceSpacesCount);
	if (referenceSpaces == NULL) {
		printf("Couldn't allocate memory for reference spaces\n");
	}
	result = xrEnumerateReferenceSpaces(session, referenceSpacesCount, &referenceSpacesCount, referenceSpaces);
	if (!xr_result(result, "Enumerating reference spaces failed!")) {
		free(referenceSpaces);
		return false;
	}

	for (uint32_t i = 0; i < referenceSpacesCount; i++) {
		if (referenceSpaces[i] == type) {
			free(referenceSpaces);
			return true;
		}
	}

	free(referenceSpaces);
	return false;
}

OpenXRApi::OpenXRApi() {
#ifdef WIN32
	if (!gladLoadGL()) {
		printf("Failed to initialize GLAD\n");
		return;
	}
#endif

	buffer_index = NULL;

	state = XR_SESSION_STATE_UNKNOWN;
	should_render = false;

	monado_stick_on_ball_ext = false;

	XrResult result;

	uint32_t extensionCount = 0;
	result = xrEnumerateInstanceExtensionProperties(NULL, 0, &extensionCount, NULL);

	/* TODO: instance null will not be able to convert XrResult to string */
	if (!xr_result(result, "Failed to enumerate number of extension properties")) {
		return;
	}

	// Damn you microsoft for not supporting this!!
	// XrExtensionProperties extensionProperties[extensionCount];
	XrExtensionProperties * extensionProperties = (XrExtensionProperties *)malloc(sizeof(XrExtensionProperties) * extensionCount);
	if (extensionProperties == NULL) {
		printf("Couldn't allocate memory for extension properties\n");
		return;
	}
	for (uint16_t i = 0; i < extensionCount; i++) {
		extensionProperties[i].type = XR_TYPE_EXTENSION_PROPERTIES;
		extensionProperties[i].next = NULL;
	}

	result = xrEnumerateInstanceExtensionProperties(NULL, extensionCount, &extensionCount, extensionProperties);
	if (!xr_result(result, "Failed to enumerate extension properties")) {
		free(extensionProperties);
		return;
	}

	if (!isExtensionSupported(XR_KHR_OPENGL_ENABLE_EXTENSION_NAME, extensionProperties, extensionCount)) {
		printf("Runtime does not support OpenGL extension!\n");
		free(extensionProperties);
		return;
	}

	if (isExtensionSupported(XR_MND_BALL_ON_STICK_EXTENSION_NAME, extensionProperties, extensionCount)) {
		monado_stick_on_ball_ext = true;
	}

	free(extensionProperties);

	// Damn you microsoft for not supporting this!!
	// const char *enabledExtensions[extensionCount];
	char ** enabledExtensions = (char **) malloc(sizeof(char *) * extensionCount);
	if (enabledExtensions == NULL) {
		printf("Couldn't allocate memory to record enabled extensions\n");
		return;
	}

	uint32_t enabledExtensionCount = 0;
	enabledExtensions[enabledExtensionCount++] = XR_KHR_OPENGL_ENABLE_EXTENSION_NAME;

	if (monado_stick_on_ball_ext) {
		enabledExtensions[enabledExtensionCount++] = XR_MND_BALL_ON_STICK_EXTENSION_NAME;
	}

	// Microsoft wants fields in definition to be in order or it will have a hissy fit!
	XrInstanceCreateInfo instanceCreateInfo = {
		.type = XR_TYPE_INSTANCE_CREATE_INFO,
		.next = NULL,
		.createFlags = 0,
		.applicationInfo = {
				// TODO: get application name from godot
				// TODO: establish godot version -> uint32_t versioning
				.applicationName = "Godot OpenXR Plugin",
				.applicationVersion = 1,
				.engineName = "Godot Engine",
				.engineVersion = 0,
				.apiVersion = XR_CURRENT_API_VERSION,
		},
		.enabledApiLayerCount = 0,
		.enabledApiLayerNames = NULL,
		.enabledExtensionCount = enabledExtensionCount,
		.enabledExtensionNames = enabledExtensions,
	};

	result = xrCreateInstance(&instanceCreateInfo, &instance);
	if (!xr_result(result, "Failed to create XR instance.")) {
		free(enabledExtensions);
		return;
	}
	free(enabledExtensions);

	XrSystemGetInfo systemGetInfo = {
		.type = XR_TYPE_SYSTEM_GET_INFO,
		.next = NULL,
		.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY,
	};

	XrSystemId systemId;
	result = xrGetSystem(instance, &systemGetInfo, &systemId);
	if (!xr_result(result, "Failed to get system for HMD form factor.")) {
		return;
	}

	XrSystemProperties systemProperties = {
		.type = XR_TYPE_SYSTEM_PROPERTIES,
		.next = NULL,
		.graphicsProperties = { 0 },
		.trackingProperties = { 0 },
	};
	result = xrGetSystemProperties(instance, systemId, &systemProperties);
	if (!xr_result(result, "Failed to get System properties")) {
		return;
	}

	XrViewConfigurationType viewConfigType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
	if (!isViewConfigSupported(viewConfigType, systemId)) {
		printf("Stereo View Configuration not supported!");
		return;
	}

	result = xrEnumerateViewConfigurationViews(instance, systemId, viewConfigType, 0, &view_count, NULL);
	if (!xr_result(result, "Failed to get view configuration view count!")) {
		return;
	}

	configuration_views = (XrViewConfigurationView *) malloc(sizeof(XrViewConfigurationView) * view_count);

	result = xrEnumerateViewConfigurationViews(instance, systemId, viewConfigType, view_count, &view_count, configuration_views);
	if (!xr_result(result, "Failed to enumerate view configuration views!")) {
		return;
	}

	buffer_index = (uint32_t *) malloc(sizeof(uint32_t) * view_count);

	if (!check_graphics_requirements_gl(systemId)) {
		return;
	}

	// TODO: support wayland
	// TODO: maybe support xcb separately?
	// TODO: support vulkan
#ifdef WIN32
	// TODO: support windows

	graphics_binding_gl = XrGraphicsBindingOpenGLWin32KHR {
		.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR,
	};

	// TODO: get this from godot engine
	// graphics_binding_gl.hDC = ???;
	// graphics_binding_gl.hGLRC = ???;
#else
	graphics_binding_gl = (XrGraphicsBindingOpenGLXlibKHR) {
		.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR,
	};

	// TODO: get this from godot engine
	graphics_binding_gl.xDisplay = XOpenDisplay(NULL);
	graphics_binding_gl.glxContext = glXGetCurrentContext();
	graphics_binding_gl.glxDrawable = glXGetCurrentDrawable();

	printf("Graphics: Display %p, Context %" PRIxPTR ", Drawable %" PRIxPTR
		   "\n",
			graphics_binding_gl.xDisplay,
			(uintptr_t)graphics_binding_gl.glxContext,
			(uintptr_t)graphics_binding_gl.glxDrawable);
#endif


	printf("Using OpenGL version: %s\n", glGetString(GL_VERSION));
	printf("Using OpenGL Renderer: %s\n", glGetString(GL_RENDERER));

	XrSessionCreateInfo session_create_info = {
		.type = XR_TYPE_SESSION_CREATE_INFO,
		.next = &graphics_binding_gl,
		.systemId = systemId
	};

	result = xrCreateSession(instance, &session_create_info, &session);
	if (!xr_result(result, "Failed to create session")) {
		return;
	}

	XrReferenceSpaceType playSpace = XR_REFERENCE_SPACE_TYPE_LOCAL;
	if (!isReferenceSpaceSupported(playSpace)) {
		printf("runtime does not support local space!\n");
		return;
	}

	XrPosef identityPose = {
		.orientation = { .x = 0, .y = 0, .z = 0, .w = 1.0 },
		.position = { .x = 0, .y = 0, .z = 0 }
	};

	XrReferenceSpaceCreateInfo localSpaceCreateInfo = {
		.type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
		.next = NULL,
		.referenceSpaceType = playSpace,
		.poseInReferenceSpace = identityPose
	};

	result = xrCreateReferenceSpace(session, &localSpaceCreateInfo, &local_space);
	if (!xr_result(result, "Failed to create local space!")) {
		return;
	}

	XrSessionBeginInfo sessionBeginInfo = {
		.type = XR_TYPE_SESSION_BEGIN_INFO,
		.next = NULL,
		.primaryViewConfigurationType = viewConfigType
	};
	result = xrBeginSession(session, &sessionBeginInfo);
	if (!xr_result(result, "Failed to begin session!")) {
		return;
	}

	uint32_t swapchainFormatCount;
	result = xrEnumerateSwapchainFormats(session, 0, &swapchainFormatCount, NULL);
	if (!xr_result(result, "Failed to get number of supported swapchain formats")) {
		return;
	}

	// Damn you microsoft for not supporting this!!
	// int64_t swapchainFormats[swapchainFormatCount];
	int64_t * swapchainFormats = (int64_t *) malloc(sizeof(int64_t) * swapchainFormatCount);
	if (swapchainFormats == NULL) {
		printf("Couldn't allocate memory for swap chain formats\n");
		return;
	}

	result = xrEnumerateSwapchainFormats(session, swapchainFormatCount, &swapchainFormatCount, swapchainFormats);
	if (!xr_result(result, "Failed to enumerate swapchain formats")) {
		free(swapchainFormats);
		return;
	}

	const bool SRGB_SWAPCHAIN = true;

	int64_t swapchainFormatToUse = swapchainFormats[0];

	printf("Swapchain Formats\n");
	for (int i = 0; i < swapchainFormatCount; i++) {
		printf("%llX\n", swapchainFormats[i]);
#ifdef WIN32
		if (SRGB_SWAPCHAIN && swapchainFormats[i] == GL_SRGB8_ALPHA8) {
			swapchainFormatToUse = swapchainFormats[i];
			printf("Using SRGB swapchain!\n");
		}
		if (!SRGB_SWAPCHAIN && swapchainFormats[i] == GL_RGBA8) {
			swapchainFormatToUse = swapchainFormats[i];
			printf("Using RGBA swapchain!\n");
		}
#else
		if (SRGB_SWAPCHAIN && swapchainFormats[i] == GL_SRGB8_ALPHA8_EXT) {
			swapchainFormatToUse = swapchainFormats[i];
			printf("Using SRGB swapchain!\n");
		}
		if (!SRGB_SWAPCHAIN && swapchainFormats[i] == GL_RGBA8_EXT) {
			swapchainFormatToUse = swapchainFormats[i];
			printf("Using RGBA swapchain!\n");
		}
#endif
	}

	free(swapchainFormats);

	swapchains = (XrSwapchain *)malloc(sizeof(XrSwapchain) * view_count);

	// Damn you microsoft for not supporting this!!
	//uint32_t swapchainLength[view_count];
	uint32_t * swapchainLength = (uint32_t *) malloc(sizeof(uint32_t) * view_count);
	for (uint32_t i = 0; i < view_count; i++) {
		// again Microsoft wants these in order!
		XrSwapchainCreateInfo swapchainCreateInfo = {
			.type = XR_TYPE_SWAPCHAIN_CREATE_INFO,
			.next = NULL,
			.createFlags = 0,
			.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT,
			.format = swapchainFormatToUse,
			.sampleCount = 1,
			.width = configuration_views[i].recommendedImageRectWidth,
			.height = configuration_views[i].recommendedImageRectHeight,
			.faceCount = 1,
			.arraySize = 1,
			.mipCount = 1,
		};

		result = xrCreateSwapchain(session, &swapchainCreateInfo, &swapchains[i]);
		if (!xr_result(result, "Failed to create swapchain %d!", i)) {
			free(swapchainLength);
			return;
		}

		result = xrEnumerateSwapchainImages(swapchains[i], 0, &swapchainLength[i], NULL);
		if (!xr_result(result, "Failed to enumerate swapchains")) {
			free(swapchainLength);
			return;
		}
	}
	free(swapchainLength);

	uint32_t maxSwapchainLength = 0;
	for (uint32_t i = 0; i < view_count; i++) {
		if (swapchainLength[i] > maxSwapchainLength) {
			maxSwapchainLength = swapchainLength[i];
		}
	}

	images = (XrSwapchainImageOpenGLKHR **)malloc(sizeof(XrSwapchainImageOpenGLKHR **) * view_count);
	for (uint32_t i = 0; i < view_count; i++) {
		images[i] = (XrSwapchainImageOpenGLKHR *)malloc(sizeof(XrSwapchainImageOpenGLKHR *) * maxSwapchainLength);

		for (int j = 0; j < maxSwapchainLength; j++) {
			images[i][j].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR;
			images[i][j].next = NULL;
		}
	}

	for (uint32_t i = 0; i < view_count; i++) {
		result = xrEnumerateSwapchainImages(swapchains[i], swapchainLength[i], &swapchainLength[i], (XrSwapchainImageBaseHeader *)images[i]);
		if (!xr_result(result, "Failed to enumerate swapchain images")) {
			return;
		}
	}

	// only used for OpenGL depth testing
	/*
	glGenTextures(1, &depthbuffer);
	glBindTexture(GL_TEXTURE_2D, depthbuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24,
	             configuration_views[0].recommendedImageRectWidth,
	             configuration_views[0].recommendedImageRectHeight, 0,
	             GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, 0);
      */

	projectionLayer = (XrCompositionLayerProjection *)malloc(sizeof(XrCompositionLayerProjection));
	projectionLayer->type = XR_TYPE_COMPOSITION_LAYER_PROJECTION;
	projectionLayer->next = NULL;
	projectionLayer->layerFlags = 0;
	projectionLayer->space = local_space;
	projectionLayer->viewCount = view_count;
	projectionLayer->views = NULL;

	frameState.type = XR_TYPE_FRAME_STATE;
	frameState.next = NULL;

	running = true;

	views = (XrView *)malloc(sizeof(XrView) * view_count);
	projection_views = (XrCompositionLayerProjectionView *)malloc(sizeof(XrCompositionLayerProjectionView) * view_count);
	for (uint32_t i = 0; i < view_count; i++) {
		views[i].type = XR_TYPE_VIEW;
		views[i].next = NULL;

		projection_views[i].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
		projection_views[i].next = NULL;
		projection_views[i].subImage.swapchain = swapchains[i];
		projection_views[i].subImage.imageArrayIndex = 0;
		projection_views[i].subImage.imageRect.offset.x = 0;
		projection_views[i].subImage.imageRect.offset.y = 0;
		projection_views[i].subImage.imageRect.extent.width = configuration_views[i].recommendedImageRectWidth;
		projection_views[i].subImage.imageRect.extent.height = configuration_views[i].recommendedImageRectHeight;
	};

	XrActionSetCreateInfo actionSetInfo = {
		.type = XR_TYPE_ACTION_SET_CREATE_INFO,
		.next = NULL,
		.priority = 0
	};
	strcpy(actionSetInfo.actionSetName, "godotset");
	strcpy(actionSetInfo.localizedActionSetName, "Action Set Used by Godot");

	result = xrCreateActionSet(instance, &actionSetInfo, &actionSet);
	if (!xr_result(result, "failed to create actionset")) {
		return;
	}

	xrStringToPath(instance, "/user/hand/left", &handPaths[HAND_LEFT]);
	xrStringToPath(instance, "/user/hand/right", &handPaths[HAND_RIGHT]);

	// TODO: add action editor to godot and create actions dynamically
	actions[TRIGGER_ACTION_INDEX] = createAction(XR_ACTION_TYPE_FLOAT_INPUT, "trigger", "Trigger Button");
	if (actions[TRIGGER_ACTION_INDEX] == NULL) {
		return;
	}

	actions[GRAB_ACTION_INDEX] = createAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "grab", "Grab Button");
	if (actions[GRAB_ACTION_INDEX] == NULL) {
		return;
	}

	actions[MENU_ACTION_INDEX] = createAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "menu", "Menu Button");
	if (actions[GRAB_ACTION_INDEX] == NULL) {
		return;
	}

	actions[POSE_ACTION_INDEX] = createAction(XR_ACTION_TYPE_POSE_INPUT, "handpose", "Hand Pose");
	if (actions[POSE_ACTION_INDEX] == NULL) {
		return;
	}

	XrPath selectClickPath[HANDCOUNT];
	xrStringToPath(instance, "/user/hand/left/input/select/click", &selectClickPath[HAND_LEFT]);
	xrStringToPath(instance, "/user/hand/right/input/select/click", &selectClickPath[HAND_RIGHT]);

	XrPath aimPosePath[HANDCOUNT];
	xrStringToPath(instance, "/user/hand/left/input/aim/pose", &aimPosePath[HAND_LEFT]);
	xrStringToPath(instance, "/user/hand/right/input/aim/pose", &aimPosePath[HAND_RIGHT]);

	XrPath triggerPath[HANDCOUNT];
	xrStringToPath(instance, "/user/hand/left/input/trigger", &triggerPath[HAND_LEFT]);
	xrStringToPath(instance, "/user/hand/right/input/trigger", &triggerPath[HAND_RIGHT]);

	XrPath menuPath[HANDCOUNT];
	xrStringToPath(instance, "/user/hand/left/input/menu/click", &menuPath[HAND_LEFT]);
	xrStringToPath(instance, "/user/hand/right/input/menu/click", &menuPath[HAND_RIGHT]);

	XrPath aPath[HANDCOUNT];
	xrStringToPath(instance, "/user/hand/left/input/a/click", &aPath[HAND_LEFT]);
	xrStringToPath(instance, "/user/hand/right/input/a/click", &aPath[HAND_RIGHT]);

	XrPath bPath[HANDCOUNT];
	xrStringToPath(instance, "/user/hand/left/input/b/click", &bPath[HAND_LEFT]);
	xrStringToPath(instance, "/user/hand/right/input/b/click", &bPath[HAND_RIGHT]);

	// khr simple controller
	{
		XrAction actions[] = { actions[POSE_ACTION_INDEX], actions[TRIGGER_ACTION_INDEX] };
		XrPath *paths[] = { aimPosePath, selectClickPath };
		int num_actions = sizeof(actions) / sizeof(actions[0]);
		if (!suggestActions("/interaction_profiles/khr/simple_controller", actions, paths, num_actions)) {
			return;
		}
	}

	// valve index controller
	{
		XrAction actions[] = {
			actions[POSE_ACTION_INDEX],
			actions[TRIGGER_ACTION_INDEX],
			actions[GRAB_ACTION_INDEX],
			actions[MENU_ACTION_INDEX],
		};
		XrPath *paths[] = { aimPosePath, triggerPath, aPath, bPath };
		int num_actions = sizeof(actions) / sizeof(actions[0]);
		if (!suggestActions("/interaction_profiles/valve/index_controller", actions, paths, num_actions)) {
			return;
		}
	}

	// monado ext: ball on stick controller (psmv)
	if (/* TODO: remove when ext exists */ true || monado_stick_on_ball_ext) {
		XrPath squarePath[HANDCOUNT];
		xrStringToPath(instance, "/user/hand/left/input/square_mnd/click", &squarePath[HAND_LEFT]);
		xrStringToPath(instance, "/user/hand/right/input/square_mnd/click", &squarePath[HAND_RIGHT]);

		XrAction actions[] = {
			actions[POSE_ACTION_INDEX],
			actions[TRIGGER_ACTION_INDEX],
			actions[GRAB_ACTION_INDEX],
			actions[MENU_ACTION_INDEX],
		};
		XrPath *paths[] = { aimPosePath, triggerPath, squarePath, menuPath };
		int num_actions = sizeof(actions) / sizeof(actions[0]);
		if (!suggestActions("/interaction_profiles/mnd/ball_on_stick_controller", actions, paths, num_actions)) {
			return;
		}
	}

	XrActionSpaceCreateInfo actionSpaceInfo = {
		.type = XR_TYPE_ACTION_SPACE_CREATE_INFO,
		.next = NULL,
		.action = actions[POSE_ACTION_INDEX],
		.subactionPath = handPaths[0],
		// seriously MS, you can't support this either?!?!
		//.poseInActionSpace.orientation.w = 1.f,
		.poseInActionSpace = {
			.orientation = {
				.w = 1.f
			}
		},
	};

	result = xrCreateActionSpace(session, &actionSpaceInfo, &handSpaces[0]);
	if (!xr_result(result, "failed to create left hand pose space")) {
		return;
	}

	actionSpaceInfo.subactionPath = handPaths[1];
	result = xrCreateActionSpace(session, &actionSpaceInfo, &handSpaces[1]);
	if (!xr_result(result, "failed to create right hand pose space")) {
		return;
	}

	XrSessionActionSetsAttachInfo attachInfo = {
		.type = XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO,
		.next = NULL,
		.countActionSets = 1,
		.actionSets = &actionSet
	};
	result = xrAttachSessionActionSets(session, &attachInfo);
	if (!xr_result(result, "failed to attach action set")) {
		return;
	}

	godot_controllers[0] = arvr_api->godot_arvr_add_controller("lefthand", 1, true, true);
	godot_controllers[1] = arvr_api->godot_arvr_add_controller("righthand", 2, true, true);

	printf("initialized controllers %d %d\n", godot_controllers[0], godot_controllers[1]);
}


OpenXRApi::~OpenXRApi() {
	free(projection_views);
	free(configuration_views);
	free(buffer_index);
	free(swapchains);
	if (images) {
		for (uint32_t i = 0; i < view_count; i++) {
			free(images[i]);
		}
	}
	free(images);
	free(projectionLayer);
	free(views);

	if (session) {
		xrDestroySession(session);
	}
	xrDestroyInstance(instance);
}

XrAction OpenXRApi::createAction(XrActionType actionType, char *actionName, char *localizedActionName) {
	XrActionCreateInfo actionInfo = {
		.type = XR_TYPE_ACTION_CREATE_INFO,
		.next = NULL,
		.actionType = actionType,
		.countSubactionPaths = HANDCOUNT,
		.subactionPaths = handPaths
	};

	strcpy(actionInfo.actionName, actionName);
	strcpy(actionInfo.localizedActionName, localizedActionName);

	XrAction action;
	XrResult result = xrCreateAction(actionSet, &actionInfo, &action);
	if (!xr_result(result, "failed to create %s action", actionName)) {
		return NULL;
	}

	return action;
}

XrResult OpenXRApi::getActionStates(XrAction action, XrStructureType actionStateType, void *states) {
	for (int i = 0; i < HANDCOUNT; i++) {
		XrActionStateGetInfo getInfo = {
			.type = XR_TYPE_ACTION_STATE_GET_INFO,
			.next = NULL,
			.action = action,
			.subactionPath = handPaths[i]
		};

		switch (actionStateType) {
			case XR_TYPE_ACTION_STATE_FLOAT: {
				XrActionStateFloat *resultStates = (XrActionStateFloat *)states;
				resultStates[i].type = XR_TYPE_ACTION_STATE_FLOAT,
				resultStates[i].next = NULL;
				XrResult result = xrGetActionStateFloat(session, &getInfo, &resultStates[i]);
				if (!xr_result(result, "failed to get float value for hand %d!", i)) {
					resultStates[i].isActive = false;
				}
				break;
			}
			case XR_TYPE_ACTION_STATE_BOOLEAN: {
				XrActionStateBoolean *resultStates = (XrActionStateBoolean *)states;
				resultStates[i].type = XR_TYPE_ACTION_STATE_BOOLEAN,
				resultStates[i].next = NULL;
				XrResult result = xrGetActionStateBoolean(session, &getInfo, &resultStates[i]);
				if (!xr_result(result, "failed to get boolean value for hand %d!", i)) {
					resultStates[i].isActive = false;
				}
				break;
			}
			case XR_TYPE_ACTION_STATE_POSE: {
				XrActionStatePose *resultStates = (XrActionStatePose *)states;
				resultStates[i].type = XR_TYPE_ACTION_STATE_POSE;
				resultStates[i].next = NULL;
				XrResult result = xrGetActionStatePose(session, &getInfo, &resultStates[i]);
				if (!xr_result(result, "failed to get pose value for hand %d!", i)) {
					resultStates[i].isActive = false;
				}
				break;
			}

			default: return XR_ERROR_ACTION_TYPE_MISMATCH; // TOOD
		}
	}

	return XR_SUCCESS;
}

bool OpenXRApi::suggestActions(char *interaction_profile, XrAction *actions, XrPath **paths, int num_actions) {
	XrPath interactionProfilePath;

	XrResult result = xrStringToPath(instance, interaction_profile, &interactionProfilePath);
	if (!xr_result(result, "failed to get interaction profile path")) {
		return false;
	}

	uint32_t num_bindings = num_actions * HANDCOUNT;
	printf("Suggesting actions for %s, %d bindings\n", interaction_profile, num_bindings);

	// ugh..
	// XrActionSuggestedBinding bindings[num_bindings];
	XrActionSuggestedBinding * bindings = (XrActionSuggestedBinding *) malloc(sizeof(XrActionSuggestedBinding) * num_bindings);

	for (int action_count = 0; action_count < num_actions; action_count++) {
		for (int handCount = 0; handCount < HANDCOUNT; handCount++) {
			int binding_index =
					action_count * HANDCOUNT + handCount;

			bindings[binding_index].action = actions[action_count];
			bindings[binding_index].binding = paths[action_count][handCount];

#if 0
			for (int k = 0; k < LAST_ACTION_INDEX; k++) {
				if (actions[k] == actions[action_count]) {
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
		.suggestedBindings = bindings
	};

	xrSuggestInteractionProfileBindings(instance, &suggestedBindings);
	if (!xr_result(result, "failed to suggest simple bindings")) {
		free(bindings);
		return false;
	}

	free(bindings);

	return true;
}

bool OpenXRApi::check_graphics_requirements_gl(XrSystemId system_id) {
	XrGraphicsRequirementsOpenGLKHR opengl_reqs = {
		.type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR, .next = NULL
	};

	PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR = NULL;
	XrResult result = xrGetInstanceProcAddr(instance, "xrGetOpenGLGraphicsRequirementsKHR", (PFN_xrVoidFunction *)&pfnGetOpenGLGraphicsRequirementsKHR);

	if (!xr_result(result, "Failed to get xrGetOpenGLGraphicsRequirementsKHR fp!")) {
		return false;
	}

	result = pfnGetOpenGLGraphicsRequirementsKHR(instance, system_id, &opengl_reqs);
	if (!xr_result(result, "Failed to get OpenGL graphics requirements!")) {
		return false;
	}

	XrVersion desired_opengl_version = XR_MAKE_VERSION(3, 3, 0);
	if (desired_opengl_version > opengl_reqs.maxApiVersionSupported || desired_opengl_version < opengl_reqs.minApiVersionSupported) {
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

XrResult OpenXRApi::acquire_image(int eye) {
	XrResult result;
	XrSwapchainImageAcquireInfo swapchainImageAcquireInfo = {
		.type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO, .next = NULL
	};
	result = xrAcquireSwapchainImage(swapchains[eye], &swapchainImageAcquireInfo, &buffer_index[eye]);
	if (!xr_result(result, "failed to acquire swapchain image!")) {
		return result;
	}

	XrSwapchainImageWaitInfo swapchainImageWaitInfo = {
		.type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO,
		.next = NULL,
		.timeout = 0
	};
	result = xrWaitSwapchainImage(swapchains[eye], &swapchainImageWaitInfo);
	if (!xr_result(result, "failed to wait for swapchain image!")) {
		return result;
	}
	return XR_SUCCESS;
}

void OpenXRApi::render_openxr(int eye, uint32_t texid, bool has_external_texture_support) {
	// printf("Render eye %d texture %d\n", eye, texid);
	XrResult result;

	// TODO: save resources in some states where we don't need to do
	// anything
	if (!running || state >= XR_SESSION_STATE_STOPPING)
		return;

	if (!frameState.shouldRender) {

		// TODO: When godot doesn't render & call
		// get_external_texture_for_eye(), we also don't need to release
		if (has_external_texture_support) {
			XrSwapchainImageReleaseInfo swapchainImageReleaseInfo = {
				.type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO,
				.next = NULL 
			};
			result = xrReleaseSwapchainImage(swapchains[eye], &swapchainImageReleaseInfo);
			if (!xr_result(result, "failed to release swapchain image!")) {
				return;
			}
		}

		if (eye == 1) {
			// MS wants these in order..
			// submit 0 layers when we shouldn't render
			XrFrameEndInfo frameEndInfo = {
				.type = XR_TYPE_FRAME_END_INFO,
				.next = NULL,
				.displayTime = frameState.predictedDisplayTime,
				.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
				.layerCount = 0,
				.layers = NULL,
			};
			result = xrEndFrame(session, &frameEndInfo);
			xr_result(result, "failed to end frame!");
		}

		// neither eye is rendered
		return;
	}

	if (!has_external_texture_support) {
		result = acquire_image(eye);
		if (!xr_result(result, "failed to acquire swapchain image!")) {
			return;
		}

		glBindTexture(GL_TEXTURE_2D, texid);
#ifdef WIN32
		glCopyTexSubImage2D(
#else
		glCopyTextureSubImage2D(
#endif
				images[eye][buffer_index[eye]].image, 0, 0, 0,
				0, 0,
				configuration_views[eye].recommendedImageRectWidth,
				configuration_views[eye].recommendedImageRectHeight);
		glBindTexture(GL_TEXTURE_2D, 0);
		// printf("Copy godot texture %d into XR texture %d\n", texid,
		// images[eye][bufferIndex].image);
	} else {
		// printf("Godot already rendered into our textures\n");
	}

	XrSwapchainImageReleaseInfo swapchainImageReleaseInfo = {
		.type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO,
		.next = NULL
	};
	result = xrReleaseSwapchainImage(swapchains[eye], &swapchainImageReleaseInfo);
	if (!xr_result(result, "failed to release swapchain image!")) {
		return;
	}

	projection_views[eye].fov = views[eye].fov;
	projection_views[eye].pose = views[eye].pose;

	if (eye == 1) {
		projectionLayer->views = projection_views;

		const XrCompositionLayerBaseHeader *const projectionlayers[1] = { (const XrCompositionLayerBaseHeader *const)
																				  projectionLayer };
		XrFrameEndInfo frameEndInfo = {
			.type = XR_TYPE_FRAME_END_INFO,
			.next = NULL,
			.displayTime = frameState.predictedDisplayTime,
			.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
			.layerCount = 1,
			.layers = projectionlayers,
		};
		result = xrEndFrame(session, &frameEndInfo);
		if (!xr_result(result, "failed to end frame!")) {
			return;
		}
	}
}

void OpenXRApi::fill_projection_matrix(int eye, godot_real p_z_near, godot_real p_z_far, godot_real *p_projection) {
	// ugh again
	// XrView views[view_count];
	XrView * views = (XrView *) malloc(sizeof(XrView) * view_count);
	for (uint32_t i = 0; i < view_count; i++) {
		views[i].type = XR_TYPE_VIEW;
		views[i].next = NULL;
	};

	XrViewLocateInfo viewLocateInfo = {
		.type = XR_TYPE_VIEW_LOCATE_INFO,
		.displayTime = frameState.predictedDisplayTime,
		.space = local_space
	};

	XrViewState viewState = { .type = XR_TYPE_VIEW_STATE, .next = NULL };
	uint32_t viewCountOutput;
	XrResult result = xrLocateViews(session, &viewLocateInfo, &viewState, view_count, &viewCountOutput, views);

	// printf("FOV %f %f %f %f\n", views[eye].fov.angleLeft,
	// views[eye].fov.angleRight, views[eye].fov.angleUp,
	// views[eye].fov.angleDown);

	XrMatrix4x4f matrix;
	if (!xr_result(result, "Could not locate views")) {
		printf("Locate Views failed??\n");
	} else {
		XrMatrix4x4f_CreateProjectionFov(&matrix, GRAPHICS_OPENGL, views[eye].fov, p_z_near, p_z_far);
		// printf("Fill projection matrix for eye %d / %d\n", eye,
		// view_count
		// - 1);
	}

	// printf("Projection Matrix: ");
	for (int i = 0; i < 16; i++) {
		p_projection[i] = matrix.m[i];
		// printf("%f ", p_projection[i]);
	}

	free(views);
}

bool OpenXRApi::transform_from_rot_pos(godot_transform *p_dest, XrSpaceLocation *location, float p_world_scale) {
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

void OpenXRApi::update_controllers() {
	XrResult result;

	const XrActiveActionSet activeActionSet = {
		.actionSet = actionSet, 
		.subactionPath = XR_NULL_PATH
	};

	XrActionsSyncInfo syncInfo = { 
		.type = XR_TYPE_ACTIONS_SYNC_INFO,
		.countActiveActionSets = 1,
		.activeActionSets = &activeActionSet 
	};
	result = xrSyncActions(session, &syncInfo);
	xr_result(result, "failed to sync actions!");

	XrActionStateFloat triggerStates[HANDCOUNT];
	getActionStates(actions[TRIGGER_ACTION_INDEX], XR_TYPE_ACTION_STATE_FLOAT, (void **)triggerStates);

	XrActionStateBoolean grabStates[HANDCOUNT];
	getActionStates(actions[GRAB_ACTION_INDEX], XR_TYPE_ACTION_STATE_BOOLEAN, (void **)grabStates);

	XrActionStateBoolean menuStates[HANDCOUNT];
	getActionStates(actions[MENU_ACTION_INDEX], XR_TYPE_ACTION_STATE_BOOLEAN, (void **)menuStates);

	XrActionStatePose poseStates[HANDCOUNT];
	getActionStates(actions[POSE_ACTION_INDEX], XR_TYPE_ACTION_STATE_POSE, (void **)poseStates);

	XrSpaceLocation spaceLocation[HANDCOUNT];

	for (int i = 0; i < HANDCOUNT; i++) {
		if (!poseStates[i].isActive) {
			// printf("Pose for hand %d is not active %d\n", i,
			// poseStates[i].isActive);
			continue;
		}

		spaceLocation[i].type = XR_TYPE_SPACE_LOCATION;
		spaceLocation[i].next = NULL;

		result = xrLocateSpace(handSpaces[i], local_space, frameState.predictedDisplayTime, &spaceLocation[i]);
		xr_result(result, "failed to locate space %d!", i);
		bool spaceLocationValid =
				//(spaceLocation[i].locationFlags &
				// XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
				(spaceLocation[i].locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0;

		godot_transform controller_transform;
		if (!spaceLocationValid) {
			printf("Space location not valid for hand %d\n", i);
			continue;
		} else {
			if (!transform_from_rot_pos(&controller_transform, &spaceLocation[i], 1.0)) {
				printf("Pose for hand %d is active but invalid\n", i);
				continue;
			}
		}

#if 0
		printf("pose for controller %d - %f %f %f - %f %f %f %f\n", i,
			spaceLocation[i].pose.position.x, spaceLocation[i].pose.position.y, spaceLocation[i].pose.position.z,
			spaceLocation[i].pose.orientation.x, spaceLocation[i].pose.orientation.y, spaceLocation[i].pose.orientation.z, spaceLocation[i].pose.orientation.w
		);
#endif

		arvr_api->godot_arvr_set_controller_transform(godot_controllers[i], &controller_transform, true, true);

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

		if (triggerStates[i].isActive && triggerStates[i].changedSinceLastSync) {
			arvr_api->godot_arvr_set_controller_button(godot_controllers[i], triggerButton, triggerStates[i].currentState);
		}
		if (grabStates[i].isActive && grabStates[i].changedSinceLastSync) {
			arvr_api->godot_arvr_set_controller_button(godot_controllers[i], grabButton, grabStates[i].currentState);
		}
		if (menuStates[i].isActive && menuStates[i].changedSinceLastSync) {
			arvr_api->godot_arvr_set_controller_button(godot_controllers[i], menuButton, menuStates[i].currentState);
		}
	};
}

void OpenXRApi::recommended_rendertarget_size(uint32_t *width,uint32_t *height) {
	*width = configuration_views[0].recommendedImageRectWidth;
	*height = configuration_views[0].recommendedImageRectHeight;
}

void OpenXRApi::transform_from_matrix(godot_transform *p_dest, XrMatrix4x4f *matrix, float p_world_scale) {
	godot_basis basis;
	godot_vector3 origin;
	float *basis_ptr =
			(float *)&basis; // Godot can switch between real_t being
	// double or float.. which one is used...
	float m[4][4];

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			m[i][j] = matrix->m[(i * 4) + j];
		}
	}

	int k = 0;
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			basis_ptr[k++] = m[i][j];
		};
	};

	api->godot_vector3_new(&origin, -m[3][0] * p_world_scale,
			-m[3][1] * p_world_scale,
			-m[3][2] * p_world_scale);
	// printf("Origin %f %f %f\n", origin.x, origin.y, origin.z);
	api->godot_transform_new(p_dest, &basis, &origin);
};

bool OpenXRApi::get_view_matrix(int eye, float world_scale, godot_transform *transform_for_eye) {
	if (views == NULL) {
		return false;
	}

	const XrVector3f uniformScale = { .x = 1.f, .y = 1.f, .z = 1.f };

	XrMatrix4x4f viewMatrix;
	XrMatrix4x4f_CreateTranslationRotationScaleOrbit(&viewMatrix, &views[eye].pose.position, &views[eye].pose.orientation, &uniformScale);

	XrMatrix4x4f matrix;
	XrMatrix4x4f_InvertRigidBody(&matrix, &viewMatrix);

	transform_from_matrix(transform_for_eye, &matrix, world_scale);

	return true;
}

int OpenXRApi::get_external_texture_for_eye(int eye, bool *has_support) {
	// this only gets called from Godot 3.2 and newer, allows us to use
	// OpenXR swapchain directly.

	XrResult result = acquire_image(eye);
	if (!xr_result(result, "failed to acquire swapchain image!")) {
		return 0;
	}

	// process should be called by now but just in case...
	if (state > XR_SESSION_STATE_UNKNOWN && buffer_index != NULL) {
		// make sure we know that we're rendering directly to our
		// texture chain
		*has_support = true;
		// printf("eye %d: get texture %d\n", eye, buffer_index[eye]);
		return images[eye][buffer_index[eye]].image;
	}

	return 0;
}

void OpenXRApi::process_openxr() {
	XrResult result;

	XrEventDataBuffer runtimeEvent = {
		.type = XR_TYPE_EVENT_DATA_BUFFER,
		.next = NULL 
	};

	XrResult pollResult = xrPollEvent(instance, &runtimeEvent);
	while (pollResult == XR_SUCCESS) {
		switch (runtimeEvent.type) {
			case XR_TYPE_EVENT_DATA_EVENTS_LOST: {
				XrEventDataEventsLost *event = (XrEventDataEventsLost *)&runtimeEvent;

				printf("EVENT: %d event data lost!\n", event->lostEventCount);
				// we probably didn't poll fast enough'
			} break;
			case XR_TYPE_EVENT_DATA_VISIBILITY_MASK_CHANGED_KHR: {
				XrEventDataVisibilityMaskChangedKHR *event = (XrEventDataVisibilityMaskChangedKHR *)&runtimeEvent;
				printf("EVENT: STUB: visibility mask changed\n");
			} break;
			case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: {
				XrEventDataInstanceLossPending *event = (XrEventDataInstanceLossPending *)&runtimeEvent;
				printf("EVENT: instance loss pending at %llu!\n", event->lossTime);
				running = false;
				return;
			} break;
			case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
				printf("EVENT: session state changed ");
				XrEventDataSessionStateChanged *event = (XrEventDataSessionStateChanged *)&runtimeEvent;
				// XrSessionState state = event->state;

				state = event->state;
				printf("to %d", state);
				if (event->state >= XR_SESSION_STATE_STOPPING) {
					printf("\nAbort Mission!\n");
					running = false;
					return;
				}
				printf("\n");
			} break;
			case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING: {
				XrEventDataReferenceSpaceChangePending *event = (XrEventDataReferenceSpaceChangePending *)&runtimeEvent;
				printf("EVENT: reference space type %d change pending!\n", event->referenceSpaceType);
				// TODO: do something
			} break;
			case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED: {
				printf("EVENT: interaction profile changed!\n");
				XrEventDataInteractionProfileChanged *event = (XrEventDataInteractionProfileChanged *)&runtimeEvent;

				XrInteractionProfileState state = {
					.type = XR_TYPE_INTERACTION_PROFILE_STATE
				};

				XrPath hand_paths[2];
				xrStringToPath(instance, "/user/hand/left", &hand_paths[0]);
				xrStringToPath(instance, "/user/hand/right", &hand_paths[1]);
				for (int i = 0; i < 2; i++) {
					XrResult res = xrGetCurrentInteractionProfile(session, hand_paths[i], &state);
					if (!xr_result(res, "Failed to get interaction profile for %d", i)) {
						continue;
					}

					XrPath prof = state.interactionProfile;

					uint32_t strl;
					char profile_str[XR_MAX_PATH_LENGTH];
					res = xrPathToString(instance, prof, XR_MAX_PATH_LENGTH, &strl, profile_str);
					if (!xr_result(res, "Failed to get interaction profile path str for %s", i == 0 ? "/user/hand/left" : "/user/hand/right")) {
						continue;
					}

					printf("Event: Interaction profile changed for %s: %s\n", i == 0 ? "/user/hand/left" : "/user/hand/right", profile_str);
				}

				// TODO: do something
			} break;
			default:
				printf("Unhandled event type %d\n", runtimeEvent.type);
				break;
		}

		runtimeEvent.type = XR_TYPE_EVENT_DATA_BUFFER;
		pollResult = xrPollEvent(instance, &runtimeEvent);
	}
	if (pollResult == XR_EVENT_UNAVAILABLE) {
		// processed all events in the queue
	} else {
		printf("Failed to poll events!\n");
		return;
	}

	XrFrameWaitInfo frameWaitInfo = { 
		.type = XR_TYPE_FRAME_WAIT_INFO,
		.next = NULL 
	};
	result = xrWaitFrame(session, &frameWaitInfo, &frameState);
	if (!xr_result(result, "xrWaitFrame() was not successful, exiting...")) {
		return;
	}

	update_controllers();

	XrViewLocateInfo viewLocateInfo = {
		.type = XR_TYPE_VIEW_LOCATE_INFO,
		.next = NULL,
		.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
		.displayTime = frameState.predictedDisplayTime,
		.space = local_space
	};
	XrViewState viewState = {
		.type = XR_TYPE_VIEW_STATE, 
		.next = NULL 
	};
	uint32_t viewCountOutput;
	result = xrLocateViews(session, &viewLocateInfo, &viewState, view_count, &viewCountOutput, views);
	if (!xr_result(result, "Could not locate views")) {
		return;
	}

	XrFrameBeginInfo frameBeginInfo = { 
		.type = XR_TYPE_FRAME_BEGIN_INFO,
		.next = NULL 
	};

	result = xrBeginFrame(session, &frameBeginInfo);
	if (!xr_result(result, "failed to begin frame!")) {
		return;
	}

	if (frameState.shouldRender) {
		// TODO: Tell godot not do render VR to save resources.
		// See render_openxr() for the corresponding early exit.
	}
}
