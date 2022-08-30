// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/************************************************************************************

Filename    :   SimpleBeamRenderer.h
Content     :   Helper for beam and particle rendering
Created     :   July 2020
Authors     :   Matthew Langille

************************************************************************************/

#pragma once

#include "OVR_Math.h"
#include "BeamRenderer.h"
#include "ParticleSystem.h"
#include "Input/TinyUI.h"

namespace OVRFW {

class SimpleBeamRenderer {
   public:
    SimpleBeamRenderer() = default;
    ~SimpleBeamRenderer() {
        delete spriteAtlas_;
    }

    void Init(
        OVRFW::ovrFileSys* fileSys,
        const char* particleTexture,
        OVR::Vector4f particleColor,
        float scale = 1.0f) {
        PointerParticleColor = particleColor;
        Scale = scale;
        beamRenderer_.Init(256, true);

        if (particleTexture != nullptr) {
            spriteAtlas_ = new OVRFW::ovrTextureAtlas();
            spriteAtlas_->Init(*fileSys, particleTexture);
            spriteAtlas_->BuildSpritesFromGrid(4, 2, 8);
            particleSystem_.Init(
                1024, spriteAtlas_, ovrParticleSystem::GetDefaultGpuState(), false);
        } else {
            particleSystem_.Init(1024, nullptr, ovrParticleSystem::GetDefaultGpuState(), false);
        }
    }

    void Shutdown() {
        beamRenderer_.Shutdown();
        particleSystem_.Shutdown();
    }

    void Update(
        const OVRFW::ovrApplFrameIn& in,
        const std::vector<OVRFW::TinyUI::HitTestDevice>& hitTestDevices) {
        // Clear old beams and particles
        for (auto h : beams_) {
            beamRenderer_.RemoveBeam(h);
        }
        for (auto h : particles_) {
            particleSystem_.RemoveParticle(h);
        }

        // Add UI pointers to render
        for (auto& device : hitTestDevices) {
            constexpr float beamLength = 0.5f; // 0.5 meter beam length
            const OVR::Vector3f beamDir =
                ((device.pointerEnd - device.pointerStart) * 0.5f).Normalized();
            const OVR::Vector3f beamEnd = device.pointerStart + beamDir * beamLength;
            const auto& beam =
                beamRenderer_.AddBeam(in, 0.015f, device.pointerStart, beamEnd, BeamColor);
            beams_.push_back(beam);

            if (device.hitObject) {
                const auto& particle = particleSystem_.AddParticle(
                    in,
                    device.pointerEnd,
                    0.0f,
                    OVR::Vector3f(0.0f),
                    OVR::Vector3f(0.0f),
                    PointerParticleColor,
                    ovrEaseFunc::NONE,
                    0.0f,
                    0.05f * Scale,
                    0.1f,
                    0);
                particles_.push_back(particle);
            }
        }
    }
    void Render(const OVRFW::ovrApplFrameIn& in, OVRFW::ovrRendererOutput& out) {
        /// Render beams
        particleSystem_.Frame(in, spriteAtlas_, out.FrameMatrices.CenterView);
        particleSystem_.RenderEyeView(
            out.FrameMatrices.CenterView, out.FrameMatrices.EyeProjection[0], out.Surfaces);
        beamRenderer_.Frame(in, out.FrameMatrices.CenterView);
        beamRenderer_.Render(out.Surfaces);
    }

   public:
    OVR::Vector4f PointerParticleColor = {0.5f, 0.8f, 1.0f, 1.0f};
    OVR::Vector4f BeamColor = {0.5f, 0.8f, 1.0f, 1.0f};

   private:
    OVRFW::ovrBeamRenderer beamRenderer_;
    OVRFW::ovrParticleSystem particleSystem_;
    OVRFW::ovrTextureAtlas* spriteAtlas_ = nullptr;
    std::vector<OVRFW::ovrBeamRenderer::handle_t> beams_;
    std::vector<OVRFW::ovrParticleSystem::handle_t> particles_;
    float Scale;
};

} // namespace OVRFW
