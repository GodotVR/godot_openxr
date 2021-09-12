#ifndef FB_DRR_EXT
#define FB_DRR_EXT

#include "openxr/openxr_inc.h"

#ifdef __cplusplus
extern "C" {
#endif

XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateDisplayRefreshRatesFB(
		XrSession session,
		uint32_t displayRefreshRateCapacityInput,
		uint32_t *displayRefreshRateCountOutput,
		float *displayRefreshRates);

XRAPI_ATTR XrResult XRAPI_CALL xrGetDisplayRefreshRateFB(
		XrSession session,
		float *displayRefreshRate);

XRAPI_ATTR XrResult XRAPI_CALL xrRequestDisplayRefreshRateFB(
		XrSession session,
		float displayRefreshRate);

XrResult initialise_fb_display_refresh_rate_extension(XrInstance instance);

#ifdef __cplusplus
}
#endif

#endif
