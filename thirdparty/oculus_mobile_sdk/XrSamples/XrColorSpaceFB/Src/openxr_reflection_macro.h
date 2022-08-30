#pragma once
#include <openxr/openxr_reflection.h>

// Macro copied from example in openxr_reflection.h
#define XR_ENUM_CASE_STR(name, val) \
    case name:                      \
        return #name;

#define XR_ENUM_STR(enumType)                                                                \
    constexpr const char* XrEnumStr(enumType e) {                                            \
        switch (e) { XR_LIST_ENUM_##enumType(XR_ENUM_CASE_STR) default : return "Unknown"; } \
    }

// Creates overloads of XrEnumStr() function for these enum types
XR_ENUM_STR(XrColorSpaceFB);
XR_ENUM_STR(XrResult);

#undef XR_ENUM_CASE_STR
#undef XR_ENUM_STR
