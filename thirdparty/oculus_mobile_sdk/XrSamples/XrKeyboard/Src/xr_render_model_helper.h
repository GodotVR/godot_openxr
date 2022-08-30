/************************************************************************************************
Filename    :   xr_render_model_helper.h
Content     :   Helper Inteface for openxr render_model extensions
Created     :   April 2021
Authors     :   Federico Schliemann
Language    :   C++
Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.
************************************************************************************************/
#pragma once

#include "xr_helper.h"

class XrRenderModelHelper : public XrHelper {
   public:
    XrRenderModelHelper(XrInstance instance) : XrHelper(instance) {
        /// Hook up extensions for device settings
        oxr(xrGetInstanceProcAddr(
            instance,
            "xrEnumerateRenderModelPathsFB",
            (PFN_xrVoidFunction*)(&xrEnumerateRenderModelPathsFB_)));
        oxr(xrGetInstanceProcAddr(
            instance,
            "xrGetRenderModelPropertiesFB",
            (PFN_xrVoidFunction*)(&xrGetRenderModelPropertiesFB_)));
        oxr(xrGetInstanceProcAddr(
            instance, "xrLoadRenderModelFB", (PFN_xrVoidFunction*)(&xrLoadRenderModelFB_)));
    }

    ~XrRenderModelHelper() override {
        xrEnumerateRenderModelPathsFB_ = nullptr;
        xrGetRenderModelPropertiesFB_ = nullptr;
        xrLoadRenderModelFB_ = nullptr;
    };

    /// XrHelper Interface
    virtual bool SessionInit(XrSession session) override {
        isIntialized_ = false;
        session_ = session;
        return true;
    }
    virtual bool SessionEnd() override {
        session_ = XR_NULL_HANDLE;
        return true;
    }
    virtual bool Update(XrSpace currentSpace, XrTime predictedDisplayTime) override {
        OneTimeInitialize();
        return true;
    }

    static std::vector<const char*> RequiredExtensionNames() {
        return {XR_FB_RENDER_MODEL_EXTENSION_NAME};
    }

   public:
    /// Own interface
    const std::vector<XrRenderModelPathInfoFB>& GetPaths() const {
        return paths_;
    }
    const std::vector<XrRenderModelPropertiesFB>& GetProperties() const {
        return properties_;
    }

    std::vector<uint8_t> LoadRenderModel(bool remote) {
        std::vector<uint8_t> buffer;
        XrInstance instance = GetInstance();
        std::string strToCheck;
        if (remote) {
            strToCheck = "/model_fb/keyboard/remote";
        } else {
            strToCheck = "/model_fb/keyboard/local";
        }

        for (const auto& p : paths_) {
            char buf[256];
            uint32_t bufCount = 0;
            // OpenXR two call pattern. First call gets buffer size, second call gets the buffer
            // data
            oxr(xrPathToString(instance, p.path, bufCount, &bufCount, nullptr));
            oxr(xrPathToString(instance, p.path, bufCount, &bufCount, &buf[0]));
            std::string pathString = buf;
            if (pathString.rfind(strToCheck, 0) == 0) {
                XrRenderModelPropertiesFB prop{XR_TYPE_RENDER_MODEL_PROPERTIES_FB};
                XrResult result = xrGetRenderModelPropertiesFB_(session_, p.path, &prop);
                if (result == XR_SUCCESS) {
                    if (prop.modelKey != XR_NULL_RENDER_MODEL_KEY_FB) {
                        XrRenderModelLoadInfoFB loadInfo = {XR_TYPE_RENDER_MODEL_LOAD_INFO_FB};
                        loadInfo.modelKey = prop.modelKey;

                        XrRenderModelBufferFB rmb{XR_TYPE_RENDER_MODEL_BUFFER_FB};
                        rmb.next = nullptr;
                        rmb.bufferCapacityInput = 0;
                        rmb.buffer = nullptr;
                        if (oxr(xrLoadRenderModelFB_(session_, &loadInfo, &rmb))) {
                            XRLOG(
                                "XrRenderModelHelper: loading modelKey %u size %u ",
                                prop.modelKey,
                                rmb.bufferCountOutput);
                            buffer.resize(rmb.bufferCountOutput);
                            rmb.buffer = (uint8_t*)buffer.data();
                            rmb.bufferCapacityInput = rmb.bufferCountOutput;
                            if (!oxr(xrLoadRenderModelFB_(session_, &loadInfo, &rmb))) {
                                XRLOG(
                                    "XrRenderModelHelper: FAILED to load modelKey %u on pass 2",
                                    prop.modelKey);
                                buffer.resize(0);
                            } else {
                                XRLOG(
                                    "XrRenderModelHelper: loaded modelKey %u buffer size is %u",
                                    prop.modelKey,
                                    buffer.size());
                                return buffer;
                            }
                        } else {
                            XRLOG(
                                "XrRenderModelHelper: FAILED to load modelKey %u on pass 1",
                                prop.modelKey);
                        }
                    }
                } else {
                    XRLOG(
                        "XrRenderModelHelper: FAILED to load prop for path '%s'",
                        pathString.c_str());
                }
            }
        }

        return buffer;
    }

   private:
    void OneTimeInitialize() {
        if (isIntialized_)
            return;
        /// Enumerate available models
        XrInstance instance = GetInstance();
        if (xrEnumerateRenderModelPathsFB_) {
            /// query path count
            uint32_t pathCount = 0;
            oxr(xrEnumerateRenderModelPathsFB_(session_, pathCount, &pathCount, nullptr));
            if (pathCount > 0) {
                XRLOG("XrRenderModelHelper: found %u models ", pathCount);
                paths_.resize(pathCount, { XR_TYPE_RENDER_MODEL_PATH_INFO_FB });
                /// Fill in the path data
                oxr(xrEnumerateRenderModelPathsFB_(session_, pathCount, &pathCount, &paths_[0]));
                /// Print paths for debug purpose
                for (const auto& p : paths_) {
                    char buf[256];
                    uint32_t bufCount = 0;
                    oxr(xrPathToString(instance, p.path, bufCount, &bufCount, nullptr));
                    oxr(xrPathToString(instance, p.path, bufCount, &bufCount, &buf[0]));
                    XRLOG("XrRenderModelHelper: path=%u `%s`", (uint32_t)p.path, &buf[0]);
                }
                /// Get properties
                for (const auto& p : paths_) {
                    XrRenderModelPropertiesFB prop{XR_TYPE_RENDER_MODEL_PROPERTIES_FB};
                    XrResult result = xrGetRenderModelPropertiesFB_(session_, p.path, &prop);
                    if (result == XR_SUCCESS) {
                        properties_.push_back(prop);
                    }
                }
            }
        }
        isIntialized_ = true;
    }

   private:
    /// Session cache
    XrSession session_ = XR_NULL_HANDLE;
    /// RenderModel - extension functions
    PFN_xrEnumerateRenderModelPathsFB xrEnumerateRenderModelPathsFB_ = nullptr;
    PFN_xrGetRenderModelPropertiesFB xrGetRenderModelPropertiesFB_ = nullptr;
    PFN_xrLoadRenderModelFB xrLoadRenderModelFB_ = nullptr;
    /// RenderModel - data buffers
    std::vector<XrRenderModelPathInfoFB> paths_;
    std::vector<XrRenderModelPropertiesFB> properties_;
    /// Lifetime
    bool isIntialized_ = false;
};
