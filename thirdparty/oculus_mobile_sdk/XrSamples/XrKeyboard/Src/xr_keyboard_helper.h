/************************************************************************************************
Filename    :   xr_keyboard_helper.h
Content     :   Helper Inteface for openxr keyboard extensions
Created     :   April 2021
Authors     :   Federico Schliemann
Language    :   C++
Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.
************************************************************************************************/
#pragma once

#include "xr_helper.h"

class XrKeyboardHelper : public XrHelper {
   public:
    XrKeyboardHelper(XrInstance instance) : XrHelper(instance) {
        instance_ = instance;
        /// Hook up extensions for keyboard tracking
        oxr(xrGetInstanceProcAddr(
            instance,
            "xrQuerySystemTrackedKeyboardFB",
            (PFN_xrVoidFunction*)(&xrQuerySystemTrackedKeyboardFB_)));
        oxr(xrGetInstanceProcAddr(
            instance, "xrCreateKeyboardSpaceFB", (PFN_xrVoidFunction*)(&xrCreateKeyboardSpaceFB_)));

        fakeLocation_.locationFlags =
            XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT |
            XR_SPACE_LOCATION_POSITION_TRACKED_BIT |
            XR_SPACE_LOCATION_ORIENTATION_VALID_BIT |
            XR_SPACE_LOCATION_POSITION_VALID_BIT;
    }

    ~XrKeyboardHelper() override {
        xrQuerySystemTrackedKeyboardFB_ = nullptr;
        xrCreateKeyboardSpaceFB_ = nullptr;
    };

    /// XrHelper Interface
    virtual bool SessionInit(XrSession session) override {
        session_ = session;
        return true;
    }

    virtual bool SessionEnd() override {
        session_ = XR_NULL_HANDLE;
        return StopTracking();
    }

    virtual bool Update(XrSpace currentSpace, XrTime predictedDisplayTime) override {
        if (xrQuerySystemTrackedKeyboardFB_) {
            // current query
            {
                XrKeyboardTrackingQueryFB queryInfo{XR_TYPE_KEYBOARD_TRACKING_QUERY_FB};
                if (useRemoteKeyboard_) {
                    queryInfo.flags = XR_KEYBOARD_TRACKING_QUERY_REMOTE_BIT_FB;
                } else {
                    queryInfo.flags = XR_KEYBOARD_TRACKING_QUERY_LOCAL_BIT_FB;
                }

                XrKeyboardTrackingDescriptionFB desc;
                if (oxr(xrQuerySystemTrackedKeyboardFB_(session_, &queryInfo, &desc))) {
                    if ((desc.flags & XR_KEYBOARD_TRACKING_EXISTS_BIT_FB) != 0) {
                        // found keyboard
                        if (!systemKeyboardExists_ ||
                            systemKeyboardDesc_.trackedKeyboardId != desc.trackedKeyboardId ||
                            systemKeyboardDesc_.flags != desc.flags) {
                            ALOG(
                                "Found new system keyboard '%d' '%s'",
                                desc.trackedKeyboardId,
                                desc.name);
                            systemKeyboardExists_ = true;
                            systemKeyboardDesc_ = desc;
                            systemKeyboardConnected_ =
                                systemKeyboardDesc_.flags & XR_KEYBOARD_TRACKING_CONNECTED_BIT_FB;

                            bool correctKeyboardType = false;
                            if (useRemoteKeyboard_) {
                                correctKeyboardType =
                                    systemKeyboardDesc_.flags & XR_KEYBOARD_TRACKING_REMOTE_BIT_FB;
                            } else {
                                correctKeyboardType =
                                    systemKeyboardDesc_.flags & XR_KEYBOARD_TRACKING_LOCAL_BIT_FB;
                            }

                            if (correctKeyboardType) {
                                trackingSystemKeyboard_ = false;
                                if (trackSystemKeyboard_) {
                                    if (systemKeyboardConnected_ ||
                                        !requireKeyboardConnectedToTrack_) {
                                        if (StartTrackingSystemKeyboard()) {
                                            trackingSystemKeyboard_ = true;
                                        }
                                    }
                                }
                                if (!trackingSystemKeyboard_) {
                                    StopTracking();
                                }
                            } else {
                                ALOG(
                                    "Found new system keyboard '%d' '%s', but not tracking because it doesn't match remote vs local types",
                                    desc.trackedKeyboardId,
                                    desc.name);
                            }

                            systemKeyboardStateChanged_ = true;
                        }
                    } else {
                        // no keyboard
                        if (systemKeyboardExists_) {
                            systemKeyboardExists_ = false;
                            if (trackSystemKeyboard_) {
                                StopTracking();
                                trackingSystemKeyboard_ = false;
                            }
                            systemKeyboardStateChanged_ = true;
                        }
                    }
                }
            }
        }

        if (keyboardSpace_ != XR_NULL_HANDLE) {
            location_.next = nullptr;
            return oxr(
                xrLocateSpace(keyboardSpace_, currentSpace, predictedDisplayTime, &location_));
        }
        return true;
    }

    static std::vector<const char*> RequiredExtensionNames() {
        return {XR_FB_KEYBOARD_TRACKING_EXTENSION_NAME};
    }

   public:
    /// Own interface
    const XrSpaceLocation& Location() const {
        if (IsLocationActive() || trackingRequired_) {
            return location_;
        }
        return fakeLocation_;
    }
    bool IsTracking() const {
        return (keyboardSpace_ != XR_NULL_HANDLE);
    }
    bool IsLocationActive() const {
        const XrSpaceLocationFlags isTracked =
            XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT | XR_SPACE_LOCATION_POSITION_TRACKED_BIT;
        const XrSpaceLocationFlags isValid =
            XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XR_SPACE_LOCATION_POSITION_VALID_BIT;
        XrSpaceLocationFlags flags = isTracked | isValid;
        return (IsTracking() && (location_.locationFlags & flags)) || !trackingRequired_;
    }

    void ResetSystemKeyboardTracking() {
        trackingSystemKeyboard_ = false;
        StopTracking();
        if (trackSystemKeyboard_ && systemKeyboardExists_ &&
            (systemKeyboardConnected_ || !requireKeyboardConnectedToTrack_)) {
            if (StartTrackingSystemKeyboard()) {
                trackingSystemKeyboard_ = true;
            }
        }
        systemKeyboardStateChanged_ = true;
    }

    bool StartTrackingSystemKeyboard() {
        StopTracking();

        if (xrCreateKeyboardSpaceFB_ && systemKeyboardExists_) {
            XrKeyboardSpaceCreateInfoFB createInfo{XR_TYPE_KEYBOARD_SPACE_CREATE_INFO_FB};
            createInfo.trackedKeyboardId = systemKeyboardDesc_.trackedKeyboardId;
            if (XR_SUCCEEDED(
                    oxr(xrCreateKeyboardSpaceFB_(session_, &createInfo, &keyboardSpace_)))) {
                size_ = systemKeyboardDesc_.size;
                return true;
            }
        }

        return false;
    }

    bool StopTracking() {
        bool result = false;
        if (keyboardSpace_ != XR_NULL_HANDLE) {
            result = oxr(xrDestroySpace(keyboardSpace_));
            if (result) {
                keyboardSpace_ = XR_NULL_HANDLE;
            } else {
                ALOG("Failed to destroy keyboardSpace_ %p", keyboardSpace_);
            }
        }
        return result;
    }

    XrVector3f Size() {
        return size_;
    }

    bool TracksSystemKeyboard() {
        return trackSystemKeyboard_;
    }

    bool SetTracksSystemKeyboard(bool track) {
        if (track != trackSystemKeyboard_) {
            trackSystemKeyboard_ = track;
            ResetSystemKeyboardTracking();
            return true;
        }
        return false;
    }

    bool RequireKeyboardConnectedToTrack() {
        return requireKeyboardConnectedToTrack_;
    }

    bool SetRequireKeyboardConnectedToTrack(bool require) {
        if (require != requireKeyboardConnectedToTrack_) {
            requireKeyboardConnectedToTrack_ = require;
            ResetSystemKeyboardTracking();
            return true;
        }
        return false;
    }

    bool UseRemoteKeyboard() {
        return useRemoteKeyboard_;
    }

    bool SetUseRemoteKeyboard(bool state) {
        if (state != useRemoteKeyboard_) {
            // no keyboard
            if (systemKeyboardExists_) {
                systemKeyboardExists_ = false;
                if (trackSystemKeyboard_) {
                    StopTracking();
                    trackingSystemKeyboard_ = false;
                }
                systemKeyboardStateChanged_ = true;
            }

            useRemoteKeyboard_ = state;
            return true;
        }
        return false;
    }

    bool TrackingRequired() {
        return trackingRequired_;
    }

    bool SetTrackingRequired(bool state) {
        if (state != trackingRequired_) {
            trackingRequired_ = state;
            ResetSystemKeyboardTracking();
            return true;
        }
        return false;
    }

    bool TrackingSystemKeyboard() {
        return trackingSystemKeyboard_;
    }

    bool SystemKeyboardExists() {
        return systemKeyboardExists_;
    }

    const XrKeyboardTrackingDescriptionFB& SystemKeyboardDesc() {
        return systemKeyboardDesc_;
    }

    bool SystemKeyboardConnected() {
        return systemKeyboardConnected_;
    }

    bool GetAndClearSystemKeyboardStateChanged() {
        if (systemKeyboardStateChanged_) {
            systemKeyboardStateChanged_ = false;
            return true;
        }
        return false;
    }

   private:
    /// Session cache
    XrSession session_ = XR_NULL_HANDLE;
    /// Instance cache
    XrInstance instance_ = XR_NULL_HANDLE;
    /// Keyboard - extension functions
    PFN_xrQuerySystemTrackedKeyboardFB xrQuerySystemTrackedKeyboardFB_ = nullptr;
    PFN_xrCreateKeyboardSpaceFB xrCreateKeyboardSpaceFB_ = nullptr;
    /// Keyboard - space
    XrSpace keyboardSpace_ = XR_NULL_HANDLE;
    /// Keyboard - data
    XrSpaceLocation location_{XR_TYPE_SPACE_LOCATION};
    XrSpaceLocation fakeLocation_{XR_TYPE_SPACE_LOCATION};

    XrVector3f size_;
    // mapping the system level active tracked keyboard
    bool trackSystemKeyboard_ = true;
    bool trackingSystemKeyboard_ = false;
    bool systemKeyboardExists_ = false;
    bool systemKeyboardConnected_ = false;
    bool systemKeyboardStateChanged_ = false;
    bool requireKeyboardConnectedToTrack_ = true;
    bool useRemoteKeyboard_ = false;
    bool trackingRequired_ = false;
    XrKeyboardTrackingDescriptionFB systemKeyboardDesc_;
};
