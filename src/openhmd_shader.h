/**
  I'm just parking this code here for now, this compiles and uses the shader
  that is needed for the lens distortion.

  There is a big question whether, and how much, of this code should be in
  OpenHMD itself or how much of it we should piggy back onto Godots shader
  logic. 
**/

#ifndef OPENHMD_SHADER_H
#define OPENHMD_SHADER_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include <openhmd.h>

void openhmd_shader_set_device_parameters(ohmd_device *p_device);
void openhmd_shader_render_eye(GLuint p_texture_id, int p_left_or_right_eye);
void openhmd_shader_init();
void openhmd_shader_cleanup();

#endif /* OPENHMD_SHADER_H */