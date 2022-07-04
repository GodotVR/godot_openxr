// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

static const char VERTEX_SHADER[] = R"(
  #define NUM_VIEWS 2
  #define VIEW_ID gl_ViewID_OVR
  #extension GL_OVR_multiview2 : require
  layout(num_views=NUM_VIEWS) in;
  in vec3 vertexPosition;
  in vec4 vertexColor;
  uniform mat4 ModelMatrix;
  uniform SceneMatrices {
  	uniform mat4 ViewMatrix[NUM_VIEWS];
  	uniform mat4 ProjectionMatrix[NUM_VIEWS];
  } sm;
  out vec4 fragmentColor;
  void main() {
  	gl_Position = sm.ProjectionMatrix[VIEW_ID] * (sm.ViewMatrix[VIEW_ID] * (ModelMatrix * vec4(vertexPosition, 1.0)));
  	fragmentColor = vertexColor;
  }
)";

static const char FRAGMENT_SHADER[] = R"(
  in lowp vec4 fragmentColor;
  out lowp vec4 outColor;
  void main() {
  	outColor = fragmentColor;
  }
)";

static const char STAGE_VERTEX_SHADER[] = R"(
  #define NUM_VIEWS 2
  #define VIEW_ID gl_ViewID_OVR
  #extension GL_OVR_multiview2 : require
  layout(num_views=NUM_VIEWS) in;
  in vec3 vertexPosition;
  uniform mat4 ModelMatrix;
  uniform SceneMatrices {
  	uniform mat4 ViewMatrix[NUM_VIEWS];
  	uniform mat4 ProjectionMatrix[NUM_VIEWS];
  } sm;
  void main() {
  	gl_Position = sm.ProjectionMatrix[VIEW_ID] * (sm.ViewMatrix[VIEW_ID] * (ModelMatrix * (vec4(vertexPosition, 1.0))));
  }
)";

static const char STAGE_FRAGMENT_SHADER[] = R"(
  out lowp vec4 outColor;
  void main() {
  	outColor = vec4(0.5, 0.5, 1.0, 0.5);
  }
)";
