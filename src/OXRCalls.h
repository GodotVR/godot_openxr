////////////////////////////////////////////////////////////////////////////////////////////////
// Helper calls and singleton container for accessing openvr

#ifndef OXR_CALLS_H
#define OXR_CALLS_H

#include "GodotCalls.h"
#include <openxr/openxr.h>
#include "xrmath.h"

typedef void *OPENXR_API_HANDLE;

typedef struct openxr_data_struct
{
	int use_count;
	OPENXR_API_HANDLE api;
} openxr_data_struct;

#ifdef __cplusplus
extern "C" {
#endif

void
openxr_release_data();
openxr_data_struct *
openxr_get_data();



OPENXR_API_HANDLE
init_openxr();
void
deinit_openxr(OPENXR_API_HANDLE _self);
void
render_openxr(OPENXR_API_HANDLE _self, int eye, uint32_t texid);
void
fill_projection_matrix(OPENXR_API_HANDLE _self, int eye, XrMatrix4x4f *matrix);
void
recommended_rendertarget_size(OPENXR_API_HANDLE _self,
                              uint32_t *width,
                              uint32_t *height);
bool
get_view_matrix(OPENXR_API_HANDLE _self, int eye, XrMatrix4x4f *matrix);

#ifdef __cplusplus
}
#endif

#endif /* !OXR_CALLS_H */
