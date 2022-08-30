/************************************************************************************************
Filename    :   xr_passthrough_helper.h
Content     :   Helper Inteface for openxr passthrough extensions
Created     :   May 2021
Authors     :   Federico Schliemann
Language    :   C++
Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.
************************************************************************************************/
#pragma once

#include "xr_helper.h"

class XrPassthroughHelper : public XrHelper {
   public:
    XrPassthroughHelper(XrInstance instance, XrSpace defaultSpace) : XrHelper(instance) {
        /// cache space
        defaultSpace_ = defaultSpace;

        /// passthrough
        oxr(xrGetInstanceProcAddr(
            instance, "xrCreatePassthroughFB", (PFN_xrVoidFunction*)(&xrCreatePassthroughFB_)));
        oxr(xrGetInstanceProcAddr(
            instance, "xrDestroyPassthroughFB", (PFN_xrVoidFunction*)(&xrDestroyPassthroughFB_)));
        oxr(xrGetInstanceProcAddr(
            instance, "xrPassthroughStartFB", (PFN_xrVoidFunction*)(&xrPassthroughStartFB_)));
        oxr(xrGetInstanceProcAddr(
            instance, "xrPassthroughPauseFB", (PFN_xrVoidFunction*)(&xrPassthroughPauseFB_)));
        /// layer
        oxr(xrGetInstanceProcAddr(
            instance,
            "xrCreatePassthroughLayerFB",
            (PFN_xrVoidFunction*)(&xrCreatePassthroughLayerFB_)));
        oxr(xrGetInstanceProcAddr(
            instance,
            "xrDestroyPassthroughLayerFB",
            (PFN_xrVoidFunction*)(&xrDestroyPassthroughLayerFB_)));
        oxr(xrGetInstanceProcAddr(
            instance,
            "xrPassthroughLayerPauseFB",
            (PFN_xrVoidFunction*)(&xrPassthroughLayerPauseFB_)));
        oxr(xrGetInstanceProcAddr(
            instance,
            "xrPassthroughLayerResumeFB",
            (PFN_xrVoidFunction*)(&xrPassthroughLayerResumeFB_)));
        /// style
        oxr(xrGetInstanceProcAddr(
            instance,
            "xrPassthroughLayerSetStyleFB",
            (PFN_xrVoidFunction*)(&xrPassthroughLayerSetStyleFB_)));
        /// geometry
        oxr(xrGetInstanceProcAddr(
            instance,
            "xrCreateGeometryInstanceFB",
            (PFN_xrVoidFunction*)(&xrCreateGeometryInstanceFB_)));
        oxr(xrGetInstanceProcAddr(
            instance,
            "xrDestroyGeometryInstanceFB",
            (PFN_xrVoidFunction*)(&xrDestroyGeometryInstanceFB_)));
        oxr(xrGetInstanceProcAddr(
            instance,
            "xrGeometryInstanceSetTransformFB",
            (PFN_xrVoidFunction*)(&xrGeometryInstanceSetTransformFB_)));
        /// Passthrough - mesh extension functions
        /// mesh
        oxr(xrGetInstanceProcAddr(
            instance, "xrCreateTriangleMeshFB", (PFN_xrVoidFunction*)(&xrCreateTriangleMeshFB_)));
        oxr(xrGetInstanceProcAddr(
            instance, "xrDestroyTriangleMeshFB", (PFN_xrVoidFunction*)(&xrDestroyTriangleMeshFB_)));
        /// buffers
        oxr(xrGetInstanceProcAddr(
            instance,
            "xrTriangleMeshGetVertexBufferFB",
            (PFN_xrVoidFunction*)(&xrTriangleMeshGetVertexBufferFB_)));
        oxr(xrGetInstanceProcAddr(
            instance,
            "xrTriangleMeshGetIndexBufferFB",
            (PFN_xrVoidFunction*)(&xrTriangleMeshGetIndexBufferFB_)));
        /// update
        oxr(xrGetInstanceProcAddr(
            instance,
            "xrTriangleMeshBeginUpdateFB",
            (PFN_xrVoidFunction*)(&xrTriangleMeshBeginUpdateFB_)));
        oxr(xrGetInstanceProcAddr(
            instance,
            "xrTriangleMeshEndUpdateFB",
            (PFN_xrVoidFunction*)(&xrTriangleMeshEndUpdateFB_)));
        oxr(xrGetInstanceProcAddr(
            instance,
            "xrTriangleMeshBeginVertexBufferUpdateFB",
            (PFN_xrVoidFunction*)(&xrTriangleMeshBeginVertexBufferUpdateFB_)));
        oxr(xrGetInstanceProcAddr(
            instance,
            "xrTriangleMeshEndVertexBufferUpdateFB",
            (PFN_xrVoidFunction*)(&xrTriangleMeshEndVertexBufferUpdateFB_)));
    }

    ~XrPassthroughHelper() override {
        /// Passthrough - extension functions
        /// feature
        xrCreatePassthroughFB_ = nullptr;
        xrDestroyPassthroughFB_ = nullptr;
        xrPassthroughStartFB_ = nullptr;
        xrPassthroughPauseFB_ = nullptr;
        /// layer
        xrCreatePassthroughLayerFB_ = nullptr;
        xrDestroyPassthroughLayerFB_ = nullptr;
        xrPassthroughLayerPauseFB_ = nullptr;
        xrPassthroughLayerResumeFB_ = nullptr;
        /// style
        xrPassthroughLayerSetStyleFB_ = nullptr;
        /// geometry
        xrCreateGeometryInstanceFB_ = nullptr;
        xrDestroyGeometryInstanceFB_ = nullptr;
        xrGeometryInstanceSetTransformFB_ = nullptr;
        /// Passthrough - mesh extension functions
        /// mesh
        xrCreateTriangleMeshFB_ = nullptr;
        xrDestroyTriangleMeshFB_ = nullptr;
        xrTriangleMeshGetVertexBufferFB_ = nullptr;
        xrTriangleMeshGetIndexBufferFB_ = nullptr;
        xrTriangleMeshBeginUpdateFB_ = nullptr;
        xrTriangleMeshEndUpdateFB_ = nullptr;
        xrTriangleMeshBeginVertexBufferUpdateFB_ = nullptr;
        xrTriangleMeshEndVertexBufferUpdateFB_ = nullptr;
    };

    /// XrHelper Interface
    virtual bool SessionInit(XrSession session) override {
        session_ = session;
        if (!xrCreatePassthroughFB_)
            return false;
        if (!xrCreatePassthroughLayerFB_)
            return false;
        if (!xrCreateTriangleMeshFB_)
            return false;
        if (!xrCreateGeometryInstanceFB_)
            return false;
        if (!xrPassthroughLayerSetStyleFB_)
            return false;

        /// feature
        XrPassthroughCreateInfoFB passthroughInfo{XR_TYPE_PASSTHROUGH_CREATE_INFO_FB};
        passthroughInfo.next = nullptr;
        passthroughInfo.flags = 0u;
        if (!oxr(xrCreatePassthroughFB_(session_, &passthroughInfo, &passthrough_))) {
            return false;
        }

        /// layer
        XrPassthroughLayerCreateInfoFB layerInfo{XR_TYPE_PASSTHROUGH_LAYER_CREATE_INFO_FB};
        layerInfo.next = nullptr;
        layerInfo.passthrough = passthrough_;
        layerInfo.purpose = XR_PASSTHROUGH_LAYER_PURPOSE_PROJECTED_FB;
        layerInfo.flags = XR_PASSTHROUGH_IS_RUNNING_AT_CREATION_BIT_FB;
        if (!oxr(xrCreatePassthroughLayerFB_(session_, &layerInfo, &layerProjected_))) {
            return false;
        }
        /// also create a hand layer
        layerInfo.purpose = XR_PASSTHROUGH_LAYER_PURPOSE_TRACKED_KEYBOARD_HANDS_FB;
        if (!oxr(xrCreatePassthroughLayerFB_(session_, &layerInfo, &layerHands_))) {
            // If the runtime does not support passthrough keyboard hands, that's ok
            // we will keep going with the layer we have.
            // return false;
        }

        /// style
        XrPassthroughStyleFB style{XR_TYPE_PASSTHROUGH_STYLE_FB};
        style.next = nullptr;
        style.textureOpacityFactor = 1.0f;
        style.edgeColor = {0.0f, 0.0f, 0.0f, 0.0f};
        if (!oxr(xrPassthroughLayerSetStyleFB_(layerProjected_, &style))) {
            return false;
        }

        const int colorMapSize = XR_PASSTHROUGH_COLOR_MAP_MONO_SIZE_FB;
        const float step = 1.0f / (XR_PASSTHROUGH_COLOR_MAP_MONO_SIZE_FB - 1);

        /// default color map
        colorMap_.next = nullptr;
        for (int i = 0; i < colorMapSize; ++i) {
            const float t = step * i;
            XrColor4f c{t, 1.0f - t, 0.0f, 1.0f};
            GetColorMap()[i] = c;
        }

        /// default mono map
        monoMap_.next = nullptr;
        for (int i = 0; i < colorMapSize; ++i) {
            GetMonoMap()[i] = (uint8_t)i;
        }

        /// mesh
        // Surface-projected passthrough geometry (horizontal (xz) unit square)
        /* clang-format off */
    static const XrVector3f squareVertices[4] = {
      {-0.5,  0, -0.5},
      {-0.5,  0,  0.5},
      {0.5,  0,  0.5},
      {0.5,  0, -0.5}};
    uint32_t squareIndices[6] = {0, 1, 2, 2, 3, 0};
        /* clang-format on */

        XrTriangleMeshCreateInfoFB meshInfo{XR_TYPE_TRIANGLE_MESH_CREATE_INFO_FB};

        meshInfo.next = nullptr;
        meshInfo.flags = 0u;
        meshInfo.vertexCount = 4;
        meshInfo.vertexBuffer = squareVertices;
        meshInfo.triangleCount = 2;
        meshInfo.indexBuffer = squareIndices;
        meshInfo.windingOrder = XR_WINDING_ORDER_UNKNOWN_FB;
        if (!oxr(xrCreateTriangleMeshFB_(session_, &meshInfo, &mesh_))) {
            return false;
        }

        /// transform - intialize defaults
        transform_.type = XR_TYPE_GEOMETRY_INSTANCE_TRANSFORM_FB;
        transform_.next = nullptr;
        transform_.baseSpace = defaultSpace_;
        transform_.pose.orientation = {0.0f, 0.0f, 0.0f, 1.0f};
        transform_.pose.position = {0.0f, 0.0f, 0.0f};
        transform_.scale = {1.0f, 1.0f, 1.0f};

        /// geometry
        XrGeometryInstanceCreateInfoFB geometryInfo{XR_TYPE_GEOMETRY_INSTANCE_CREATE_INFO_FB};
        geometryInfo.next = nullptr;
        geometryInfo.layer = layerProjected_;
        geometryInfo.mesh = mesh_;
        geometryInfo.scale = transform_.scale;
        geometryInfo.baseSpace = transform_.baseSpace;
        geometryInfo.pose = transform_.pose;
        if (!oxr(xrCreateGeometryInstanceFB_(session_, &geometryInfo, &geometryProjected_))) {
            return false;
        }

        Start();

        return true;
    }

    virtual bool SessionEnd() override {
        session_ = XR_NULL_HANDLE;
        bool success = true;
        if (xrDestroyGeometryInstanceFB_) {
            success = success && oxr(xrDestroyGeometryInstanceFB_(geometryProjected_));
        }
        if (xrDestroyTriangleMeshFB_) {
            success = success && oxr(xrDestroyTriangleMeshFB_(mesh_));
        }
        if (xrDestroyPassthroughLayerFB_) {
            success = success && oxr(xrDestroyPassthroughLayerFB_(layerProjected_));
            success = success && oxr(xrDestroyPassthroughLayerFB_(layerHands_));
        }
        if (xrDestroyPassthroughFB_) {
            success = success && oxr(xrDestroyPassthroughFB_(passthrough_));
        }

        return success;
    }

    virtual bool Update(XrSpace currentSpace, XrTime predictedDisplayTime) override {
        if (xrGeometryInstanceSetTransformFB_) {
            transform_.baseSpace = currentSpace;
            transform_.time = predictedDisplayTime;
            return oxr(xrGeometryInstanceSetTransformFB_(geometryProjected_, &transform_));
        }
        return false;
    }

    static std::vector<const char*> RequiredExtensionNames() {
        return {
            XR_FB_PASSTHROUGH_EXTENSION_NAME,
            XR_FB_TRIANGLE_MESH_EXTENSION_NAME,
            XR_FB_PASSTHROUGH_KEYBOARD_HANDS_EXTENSION_NAME};
    }

   public:
    /// Own interface
    void SetTransform(const XrPosef& pose, const XrVector3f& scale) {
        transform_.pose = pose;
        transform_.scale = scale;
    }
    const XrPassthroughLayerFB& GetProjectedLayer() const {
        return layerProjected_;
    }
    const XrPassthroughLayerFB& GetHandsLayer() const {
        return layerHands_;
    }
    void SetDefaultSpace(XrSpace defaultSpace) {
        defaultSpace_ = defaultSpace;
    }
    bool Start() {
        if (xrPassthroughStartFB_) {
            return oxr(xrPassthroughStartFB_(passthrough_));
        }
        return false;
    }
    bool Pause() {
        if (xrPassthroughPauseFB_) {
            return oxr(xrPassthroughPauseFB_(passthrough_));
        }
        return false;
    }
    XrColor4f* GetColorMap() {
        return &(colorMap_.textureColorMap[0]);
    }
    uint8_t* GetMonoMap() {
        return &(monoMap_.textureColorMap[0]);
    }
    bool SetStyleColor(const XrPassthroughLayerFB& layer) {
        colorMap_.next = nullptr;
        XrPassthroughStyleFB style{XR_TYPE_PASSTHROUGH_STYLE_FB};
        style.next = &colorMap_;
        style.textureOpacityFactor = 1.0f;
        style.edgeColor = {0.0f, 0.0f, 0.0f, 0.0f};
        style.textureOpacityFactor = 1.0f;
        style.edgeColor = {0.0f, 0.0f, 0.0f, 0.0f};
        return oxr(xrPassthroughLayerSetStyleFB_(layer, &style));
    }
    bool SetStyleMono(const XrPassthroughLayerFB& layer) {
        monoMap_.next = nullptr;
        XrPassthroughStyleFB style{XR_TYPE_PASSTHROUGH_STYLE_FB};
        style.next = &monoMap_;
        style.textureOpacityFactor = 1.0f;
        style.edgeColor = {0.0f, 0.0f, 0.0f, 0.0f};
        style.textureOpacityFactor = 1.0f;
        style.edgeColor = {0.0f, 0.0f, 0.0f, 0.0f};
        return oxr(xrPassthroughLayerSetStyleFB_(layer, &style));
    }

   private:
    /// Session cache
    XrSpace defaultSpace_ = XR_NULL_HANDLE;
    XrSession session_ = XR_NULL_HANDLE;

    /// Passthrough - extension functions
    /// feature
    PFN_xrCreatePassthroughFB xrCreatePassthroughFB_ = nullptr;
    PFN_xrDestroyPassthroughFB xrDestroyPassthroughFB_ = nullptr;
    PFN_xrPassthroughStartFB xrPassthroughStartFB_ = nullptr;
    PFN_xrPassthroughPauseFB xrPassthroughPauseFB_ = nullptr;
    /// layer
    PFN_xrCreatePassthroughLayerFB xrCreatePassthroughLayerFB_ = nullptr;
    PFN_xrDestroyPassthroughLayerFB xrDestroyPassthroughLayerFB_ = nullptr;
    PFN_xrPassthroughLayerPauseFB xrPassthroughLayerPauseFB_ = nullptr;
    PFN_xrPassthroughLayerResumeFB xrPassthroughLayerResumeFB_ = nullptr;
    /// style
    PFN_xrPassthroughLayerSetStyleFB xrPassthroughLayerSetStyleFB_ = nullptr;
    /// geometry
    PFN_xrCreateGeometryInstanceFB xrCreateGeometryInstanceFB_ = nullptr;
    PFN_xrDestroyGeometryInstanceFB xrDestroyGeometryInstanceFB_ = nullptr;
    PFN_xrGeometryInstanceSetTransformFB xrGeometryInstanceSetTransformFB_ = nullptr;
    /// Passthrough - mesh extension functions
    /// mesh
    PFN_xrCreateTriangleMeshFB xrCreateTriangleMeshFB_ = nullptr;
    PFN_xrDestroyTriangleMeshFB xrDestroyTriangleMeshFB_ = nullptr;
    PFN_xrTriangleMeshGetVertexBufferFB xrTriangleMeshGetVertexBufferFB_ = nullptr;
    PFN_xrTriangleMeshGetIndexBufferFB xrTriangleMeshGetIndexBufferFB_ = nullptr;
    PFN_xrTriangleMeshBeginUpdateFB xrTriangleMeshBeginUpdateFB_ = nullptr;
    PFN_xrTriangleMeshEndUpdateFB xrTriangleMeshEndUpdateFB_ = nullptr;
    PFN_xrTriangleMeshBeginVertexBufferUpdateFB xrTriangleMeshBeginVertexBufferUpdateFB_ = nullptr;
    PFN_xrTriangleMeshEndVertexBufferUpdateFB xrTriangleMeshEndVertexBufferUpdateFB_ = nullptr;

    /// data
    XrPassthroughFB passthrough_ = XR_NULL_HANDLE;
    XrPassthroughLayerFB layerProjected_ = XR_NULL_HANDLE;
    XrPassthroughLayerFB layerHands_ = XR_NULL_HANDLE;
    XrTriangleMeshFB mesh_ = XR_NULL_HANDLE;
    XrGeometryInstanceFB geometryProjected_ = XR_NULL_HANDLE;
    XrGeometryInstanceTransformFB transform_{XR_TYPE_GEOMETRY_INSTANCE_TRANSFORM_FB};
    XrPassthroughColorMapMonoToRgbaFB colorMap_{XR_TYPE_PASSTHROUGH_COLOR_MAP_MONO_TO_RGBA_FB};
    XrPassthroughColorMapMonoToMonoFB monoMap_{XR_TYPE_PASSTHROUGH_COLOR_MAP_MONO_TO_MONO_FB};
};
