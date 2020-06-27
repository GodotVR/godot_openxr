////////////////////////////////////////////////////////////////////////////////////////////////
// Helper calls and singleton container for accessing openxr

#ifndef OXR_CALLS_H
#define OXR_CALLS_H

#include "GodotCalls.h"

// OXRCalls encapsulates all interaction with the OpenXR API and exposes
// the relevant results in terms of Godot types.
typedef struct _openxr_api_private OpenXRApi;

typedef struct
{
	int use_count;
	OpenXRApi *openxr_api;
} openxr_data_struct;

#ifdef __cplusplus
extern "C" {
#endif

void
openxr_release_data();

openxr_data_struct *
openxr_get_data();

OpenXRApi *
init_openxr();

void
deinit_openxr(OpenXRApi *self);

void
render_openxr(OpenXRApi *self,
              int eye,
              uint32_t texid,
              bool has_external_texture_support);

void
fill_projection_matrix(OpenXRApi *self, int eye, godot_real *p_projection);

void
recommended_rendertarget_size(OpenXRApi *self,
                              uint32_t *width,
                              uint32_t *height);

bool
get_view_matrix(OpenXRApi *self,
                int eye,
                float world_scale,
                godot_transform *transform_for_eye);

int
get_external_texture_for_eye(OpenXRApi *self, int eye, bool *has_support);

void
process_openxr(OpenXRApi *self);

#ifdef __cplusplus
}
#endif

#endif /* !OXR_CALLS_H */
