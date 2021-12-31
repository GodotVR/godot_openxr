Changes to the Godot OpenXR asset
=================================

1.1.1
-------------------
- Organised all third party resources into a thirdparty folder for easy maintenance.
- Update to OpenXR 1.0.20
- Update to Oculus OpenXR Mobile SDK v35
- Added support for Oculus passthrough (Quest support only).
- Fixed hand tracking support on Oculus Quest devices.
- Added option to automatically initialise plugin when using the premade scenes.
- Added function to retrieve playspace
- Fixed rumble sending too short durations to controllers

1.1.0
-------------------
- Implemented Android build (currently using Oculus loader, Quest support only)
- Fix invalid transforms generated from invalid space locations when using OpenXRSkeleton or OpenXRPose.
- Improved action map supporting secondary thumbstick/trackpad, menu and select buttons.

1.0.3
-------------------
- Copy loader dll in place when compiling
- Added mesh based hand scenes using Valve OpenXR hand meshes
- Updated to OpenXR 1.0.18
- Added action and interaction profile for thumbstick/joystick click, using button index 14 `JOY_VR_PAD`.

1.0.2
-------------------
- Fix folder structure of godot_openxr.zip created by Github actions

1.0.1
-------------------
- Fix crash issue on Oculus Link when taking headset off and putting it back on
- Add support for finger tracking motion range

1.0.0
-------------------
- Original implementation
- Switched to use godot-cpp
- Added actions and default profiles
