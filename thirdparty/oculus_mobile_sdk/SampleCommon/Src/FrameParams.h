// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/*******************************************************************************

Filename    :   FrameParams.h
Content     :   Common frame parameters
Created     :   July 2020
Authors     :   Federico Schliemann
Language    :   C++

*******************************************************************************/

#pragma once

#include "OVR_Math.h"

#include "Render/SurfaceRender.h"
#include <vector>

namespace OVRFW {

struct ovrKeyEvent {
    ovrKeyEvent(const int32_t keyCode, const int32_t action, const double t)
        : KeyCode(keyCode), Action(action), Time(t) {}
    int32_t KeyCode = 0;
    int32_t Action = 0;
    double Time = 0.0;
};

struct ovrTouchEvent {
    ovrTouchEvent(const int32_t action, const int32_t x_, const int32_t y_, const double t)
        : Action(action), x(x_), y(y_), Time(t) {}
    int32_t Action = 0;
    int32_t x = 0;
    int32_t y = 0;
    double Time = 0.0;
};

struct FrameMatrices {
    OVR::Matrix4f CenterView; // the view transform for the point between the eyes
    OVR::Matrix4f EyeView[2]; // the view transforms for each of the eyes
    OVR::Matrix4f EyeProjection[2]; // the projection transforms for each of the eyes
};

struct ovrApplFrameIn {
    /// accounting
    int64_t FrameIndex = 0;
    /// timing
    double PredictedDisplayTime = 0.0;
    double RealTimeInSeconds = 0.0;
    float DeltaSeconds = 0.0f;
    /// device config
    float IPD = 0.065f;
    float EyeHeight = 1.6750f;
    int32_t RecenterCount = 0;
    /// tracking
    OVR::Posef HeadPose;
    struct {
        OVR::Matrix4f ViewMatrix;
        OVR::Matrix4f ProjectionMatrix;
    } Eye[2];
    /// controllers
    OVR::Posef LeftRemotePose;
    OVR::Posef LeftRemotePointPose;
    OVR::Posef RightRemotePose;
    OVR::Posef RightRemotePointPose;
    /// joysticks
    OVR::Vector2f LeftRemoteJoystick = {0.0f, 0.0f};
    OVR::Vector2f RightRemoteJoystick = {0.0f, 0.0f};
    bool LeftRemoteTracked = false;
    bool RightRemoteTracked = false;
    /// controller buttons
    uint32_t AllButtons = 0u;
    uint32_t AllTouches = 0u;
    uint32_t LastFrameAllButtons = 0u;
    uint32_t LastFrameAllTouches = 0u;
    bool LeftRemoteIndexClick = false;
    bool RightRemoteIndexClick = false;
    float LeftRemoteIndexTrigger = 0.0f;
    float RightRemoteIndexTrigger = 0.0;
    float LeftRemoteGripTrigger = 0.0f;
    float RightRemoteGripTrigger = 0.0;

    /// Headset
    bool HeadsetIsMounted = true;
    bool LastFrameHeadsetIsMounted = true;
    /// Key/Touch android events
    std::vector<ovrKeyEvent> KeyEvents;
    std::vector<ovrTouchEvent> TouchEvents;

    /// Convenience APIs
    static const int kButtonA = 1 << 0;
    static const int kButtonB = 1 << 1;
    static const int kButtonX = 1 << 2;
    static const int kButtonY = 1 << 3;
    static const int kButtonMenu = 1 << 4;
    static const int kGripTrigger = 1 << 5;
    static const int kTrigger = 1 << 6;
    static const int kJoystick = 1 << 7;
    /// touch
    static const int kTouchJoystick = 1 << 8;
    static const int kTouchTrigger = 1 << 9;
    static const int kTouchThumbrest = 1 << 10;

    inline bool Clicked(const uint32_t& b) const {
        const bool isDown = (b & AllButtons) != 0;
        const bool wasDown = (b & LastFrameAllButtons) != 0;
        return (wasDown && !isDown);
    }
    inline bool Touched(const uint32_t& t) const {
        const bool isDown = (t & AllTouches) != 0;
        const bool wasDown = (t & LastFrameAllTouches) != 0;
        return (wasDown && !isDown);
    }
    inline bool HeadsetMounted() const {
        return (!LastFrameHeadsetIsMounted && HeadsetIsMounted);
    }
    inline bool HeadsetUnMounted() const {
        return (LastFrameHeadsetIsMounted && !HeadsetIsMounted);
    }
};

class ovrApplFrameOut {
   public:
    ovrApplFrameOut() {}
    explicit ovrApplFrameOut(const bool exitApp) : ExitApp(exitApp) {}
    bool ExitApp = false;
};

struct ovrRendererOutput {
    FrameMatrices FrameMatrices; // view and projection transforms
    std::vector<ovrDrawSurface> Surfaces; // list of surfaces to render
};

} // namespace OVRFW
