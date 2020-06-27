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

// recommended_rendertarget_size() should only be called after init_openxr()
void
recommended_rendertarget_size(OpenXRApi *self,
                              uint32_t *width,
                              uint32_t *height);

// process_openxr() should be called FIRST in the frame loop
void
process_openxr(OpenXRApi *self);

// fill_projection_matrix() should be called after process_openxr()
void
fill_projection_matrix(OpenXRApi *self, int eye, godot_real *p_projection);

// get_view_matrix() should be called after fill_projection_matrix()
bool
get_view_matrix(OpenXRApi *self,
                int eye,
                float world_scale,
                godot_transform *transform_for_eye);

// get_external_texture_for_eye() acquires images and sets has_support to true
int
get_external_texture_for_eye(OpenXRApi *self, int eye, bool *has_support);

/* render_openxr() should be called once per eye.
 *
 * If has_external_texture_support it assumes godot has finished rendering into
 * the external texture and ignores texid. If false, it copies content from
 * texid to the OpenXR swapchain. Then the image is released.
 * If eye == 1, ends the frame.
 */
void
render_openxr(OpenXRApi *self,
              int eye,
              uint32_t texid,
              bool has_external_texture_support);

void
deinit_openxr(OpenXRApi *self);

#ifdef __cplusplus
}
#endif

#endif /* !OXR_CALLS_H */
