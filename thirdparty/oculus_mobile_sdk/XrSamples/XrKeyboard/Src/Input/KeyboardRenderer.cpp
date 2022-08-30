/************************************************************************************************
Filename    :   KeyboardRenderer.cpp
Content     :   A one stop for rendering keyboards
Created     :   April 2021
Authors     :   Federico Schliemann
Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.
************************************************************************************************/
#include "KeyboardRenderer.h"
#include "Model/ModelFile.h"
#include "Model/ModelFileLoading.h"
#include "XrApp.h"

using OVR::Matrix4f;
using OVR::Posef;
using OVR::Quatf;
using OVR::Vector3f;
using OVR::Vector4f;

namespace OVRFW {
namespace Keyboard {

/// clang-format off
static const char* VertexShaderSrc = R"glsl(
attribute highp vec4 Position;
attribute highp vec3 Normal;
attribute highp vec2 TexCoord;

varying lowp vec3 oEye;
varying lowp vec3 oNormal;
varying lowp vec2 oTexCoord;

vec3 multiply( mat4 m, vec3 v )
{
  return vec3(
  m[0].x * v.x + m[1].x * v.y + m[2].x * v.z,
  m[0].y * v.x + m[1].y * v.y + m[2].y * v.z,
  m[0].z * v.x + m[1].z * v.y + m[2].z * v.z );
}

vec3 transposeMultiply( mat4 m, vec3 v )
{
  return vec3(
  m[0].x * v.x + m[0].y * v.y + m[0].z * v.z,
  m[1].x * v.x + m[1].y * v.y + m[1].z * v.z,
  m[2].x * v.x + m[2].y * v.y + m[2].z * v.z );
}

void main()
{
  gl_Position = TransformVertex( Position );
  vec3 eye = transposeMultiply( sm.ViewMatrix[VIEW_ID], -vec3( sm.ViewMatrix[VIEW_ID][3] ) );
  oEye = eye - vec3( ModelMatrix * Position );
  vec3 iNormal = Normal * 100.0f;
  oNormal = multiply( ModelMatrix, iNormal );
  oTexCoord = TexCoord;
}
)glsl";

static const char* FragmentShaderSrc = R"glsl(
precision lowp float;

uniform sampler2D Texture0;
uniform lowp vec3 SpecularLightDirection;
uniform lowp vec3 SpecularLightColor;
uniform lowp vec3 AmbientLightColor;
uniform float Opacity;
uniform float AlphaBlend;

varying lowp vec3 oEye;
varying lowp vec3 oNormal;
varying lowp vec2 oTexCoord;

lowp vec3 multiply( lowp mat3 m, lowp vec3 v )
{
  return vec3(
  m[0].x * v.x + m[1].x * v.y + m[2].x * v.z,
  m[0].y * v.x + m[1].y * v.y + m[2].y * v.z,
  m[0].z * v.x + m[1].z * v.y + m[2].z * v.z );
}

void main()
{
  lowp vec3 eyeDir = normalize( oEye.xyz );
  lowp vec3 Normal = normalize( oNormal );

  lowp vec3 reflectionDir = dot( eyeDir, Normal ) * 2.0 * Normal - eyeDir;
  lowp vec4 diffuse = texture2D( Texture0, oTexCoord );
  lowp vec3 ambientValue = diffuse.xyz * AmbientLightColor;

  lowp float nDotL = max( dot( Normal , SpecularLightDirection ), 0.0 );
  lowp vec3 diffuseValue = diffuse.xyz * SpecularLightColor * nDotL;

  lowp float specularPower = 1.0f - diffuse.a;
  specularPower = specularPower * specularPower;

  lowp vec3 H = normalize( SpecularLightDirection + eyeDir );
  lowp float nDotH = max( dot( Normal, H ), 0.0 );
  lowp float specularIntensity = pow( nDotH, 64.0f * ( specularPower ) ) * specularPower;
  lowp vec3 specularValue = specularIntensity * SpecularLightColor;

  lowp vec3 controllerColor = diffuseValue + ambientValue + specularValue;

  float alphaBlendFactor = max(diffuse.w, AlphaBlend) * Opacity;

  // apply alpha
  gl_FragColor.w = alphaBlendFactor;
  // premult
  gl_FragColor.xyz = controllerColor * gl_FragColor.w;
}
)glsl";

/// clang-format on

} // namespace Keyboard

bool KeyboardRenderer::Init(std::vector<uint8_t>& keyboardBuffer) {
    /// Shader
    ovrProgramParm UniformParms[] = {
        {"Texture0", ovrProgramParmType::TEXTURE_SAMPLED},
        {"SpecularLightDirection", ovrProgramParmType::FLOAT_VECTOR3},
        {"SpecularLightColor", ovrProgramParmType::FLOAT_VECTOR3},
        {"AmbientLightColor", ovrProgramParmType::FLOAT_VECTOR3},
        {"Opacity", ovrProgramParmType::FLOAT},
        {"AlphaBlend", ovrProgramParmType::FLOAT},
    };
    ProgKeyboard = GlProgram::Build(
        "",
        Keyboard::VertexShaderSrc,
        "",
        Keyboard::FragmentShaderSrc,
        UniformParms,
        sizeof(UniformParms) / sizeof(ovrProgramParm));

    MaterialParms materials = {};
    ModelGlPrograms programs = {};
    programs.ProgSingleTexture = &ProgKeyboard;
    programs.ProgBaseColorPBR = &ProgKeyboard;
    programs.ProgSkinnedBaseColorPBR = &ProgKeyboard;
    programs.ProgLightMapped = &ProgKeyboard;
    programs.ProgBaseColorEmissivePBR = &ProgKeyboard;
    programs.ProgSkinnedBaseColorEmissivePBR = &ProgKeyboard;
    programs.ProgSimplePBR = &ProgKeyboard;
    programs.ProgSkinnedSimplePBR = &ProgKeyboard;

    KeyboardModel = LoadModelFile_glB(
        "keyboard", (const char*)keyboardBuffer.data(), keyboardBuffer.size(), programs, materials);

    if (KeyboardModel == nullptr || static_cast<int>(KeyboardModel->Models.size()) < 1) {
        ALOGE("Couldn't load keyboard model!");
        return false;
    }

    for (auto& model : KeyboardModel->Models) {
        auto& gc = model.surfaces[0].surfaceDef.graphicsCommand;
        gc.UniformData[0].Data = &gc.Textures[0];
        gc.UniformData[1].Data = &SpecularLightDirection;
        gc.UniformData[2].Data = &SpecularLightColor;
        gc.UniformData[3].Data = &AmbientLightColor;
        gc.UniformData[4].Data = &Opacity;
        gc.UniformData[5].Data = &AlphaBlendFactor;
        gc.GpuState.depthEnable = gc.GpuState.depthMaskEnable = true;
        gc.GpuState.blendEnable = ovrGpuState::BLEND_ENABLE;
        gc.GpuState.blendMode = GL_FUNC_ADD;
        gc.GpuState.blendSrc = GL_ONE;
        gc.GpuState.blendDst = GL_ONE_MINUS_SRC_ALPHA;
    }

    /// Set defaults
    SpecularLightDirection = Vector3f(1.0f, 1.0f, 0.0f);
    SpecularLightColor = Vector3f(1.0f, 0.95f, 0.8f) * 0.75f;
    AmbientLightColor = Vector3f(1.0f, 1.0f, 1.0f) * 0.15f;

    /// all good
    return true;
}

void KeyboardRenderer::Shutdown() {
    OVRFW::GlProgram::Free(ProgKeyboard);
    if (KeyboardModel != nullptr) {
        delete KeyboardModel;
        KeyboardModel = nullptr;
    }
}

void KeyboardRenderer::Update(const OVR::Posef& pose, const OVR::Vector3f& scale) {
    /// Compute transform for the root
    Transform = Matrix4f(pose);
}

void KeyboardRenderer::Render(std::vector<ovrDrawSurface>& surfaceList) {
    /// toggle alpha override
    AlphaBlendFactor = UseSolidTexture ? 1.0f : 0.0f;
    if (KeyboardModel != nullptr) {
        for (auto& model : KeyboardModel->Models) {
            ovrDrawSurface controllerSurface;
            controllerSurface.surface = &(model.surfaces[0].surfaceDef);
            controllerSurface.modelMatrix = Transform;
            surfaceList.push_back(controllerSurface);
        }
    }
}

} // namespace OVRFW
