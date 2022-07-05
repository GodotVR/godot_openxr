// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <openxr/openxr_oculus_helpers.h>
#include <OVR_Math.h>

inline XrVector3f ToXrVector3f(const OVR::Vector3f& s) {
    XrVector3f r;
    r.x = s.x;
    r.y = s.y;
    r.z = s.z;
    return r;
}

inline OVR::Vector3f FromXrVector3f(const XrVector3f& s) {
    OVR::Vector3f r;
    r.x = s.x;
    r.y = s.y;
    r.z = s.z;
    return r;
}

inline XrQuaternionf ToXrQuaternionf(const OVR::Quatf& s) {
    XrQuaternionf r;
    r.x = s.x;
    r.y = s.y;
    r.z = s.z;
    r.w = s.w;
    return r;
}

inline OVR::Quatf FromXrQuaternionf(const XrQuaternionf& s) {
    OVR::Quatf r;
    r.x = s.x;
    r.y = s.y;
    r.z = s.z;
    r.w = s.w;
    return r;
}

inline XrPosef ToXrPosef(const OVR::Posef& s) {
    XrPosef r;
    r.orientation = ToXrQuaternionf(s.Rotation);
    r.position = ToXrVector3f(s.Translation);
    return r;
}

inline OVR::Posef FromXrPosef(const XrPosef& s) {
    OVR::Posef r;
    r.Rotation = FromXrQuaternionf(s.orientation);
    r.Translation = FromXrVector3f(s.position);
    return r;
}

inline OVR::Matrix4f OvrFromXr(const XrMatrix4x4f& x) {
    return OVR::Matrix4f(
        x.m[0x0],
        x.m[0x1],
        x.m[0x2],
        x.m[0x3],
        x.m[0x4],
        x.m[0x5],
        x.m[0x6],
        x.m[0x7],
        x.m[0x8],
        x.m[0x9],
        x.m[0xa],
        x.m[0xb],
        x.m[0xc],
        x.m[0xd],
        x.m[0xe],
        x.m[0xf]);
}

inline OVR::Quatf OvrFromXr(const XrQuaternionf& q) {
    return OVR::Quatf(q.x, q.y, q.z, q.w);
}

inline OVR::Vector3f OvrFromXr(const XrVector3f& v) {
    return OVR::Vector3f(v.x, v.y, v.z);
}

inline OVR::Posef OvrFromXr(const XrPosef& p) {
    return OVR::Posef(OvrFromXr(p.orientation), OvrFromXr(p.position));
}
