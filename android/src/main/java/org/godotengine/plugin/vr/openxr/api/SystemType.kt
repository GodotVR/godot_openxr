package org.godotengine.plugin.vr.openxr.api

/**
 * Represents OpenXR systems.
 */
enum class SystemType(internal val systemName: String) {
    UNKNOWN(""),
    OCULUS_QUEST("Oculus Quest"), // TODO: Validate device name
    OCULUS_QUEST2("Oculus Quest2"); // TODO: Validate device name

    companion object {
        fun toSystemType(value: String?): SystemType {
            if (value == null) {
                return UNKNOWN
            }

            for (systemType in values()) {
                if (systemType.systemName == value) {
                    return systemType
                }
            }
            return UNKNOWN
        }
    }
}