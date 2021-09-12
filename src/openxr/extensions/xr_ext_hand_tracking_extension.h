#ifndef EXT_HAND_TRACKING_EXT
#define EXT_HAND_TRACKING_EXT

#include "openxr/openxr_inc.h"

#ifdef __cplusplus
extern "C" {
#endif

XRAPI_ATTR XrResult XRAPI_CALL xrCreateHandTrackerEXT(
		XrSession session,
		const XrHandTrackerCreateInfoEXT *createInfo,
		XrHandTrackerEXT *handTracker);

XRAPI_ATTR XrResult XRAPI_CALL xrDestroyHandTrackerEXT(
		XrHandTrackerEXT handTracker);

XRAPI_ATTR XrResult XRAPI_CALL xrLocateHandJointsEXT(
		XrHandTrackerEXT handTracker,
		const XrHandJointsLocateInfoEXT *locateInfo,
		XrHandJointLocationsEXT *locations);

XrResult initialise_ext_hand_tracking_extension(XrInstance instance);

#ifdef __cplusplus
}
#endif

#endif // !EXT_HAND_TRACKING_EXT
