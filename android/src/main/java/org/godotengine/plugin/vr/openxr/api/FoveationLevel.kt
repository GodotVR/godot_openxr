package org.godotengine.plugin.vr.openxr.api

/**
 * For reference, see https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XrFoveationLevelFB
 */
enum class FoveationLevel(internal val value: Int) {
    /**
     * No foveation
     */
    NONE(0),

    /**
     * Less foveation (higher periphery visual fidelity, lower performance)
     */
    LOW(1),

    /**
     * Medium foveation (medium periphery visual fidelity, medium performance)
     */
    MEDIUM(2),

    /**
     * High foveation (lower periphery visual fidelity, higher performance)
     */
    HIGH(3);

    companion object {
        fun toFoveationLevel(value: Int): FoveationLevel {
            for (level in values()) {
                if (level.value == value) {
                    return level
                }
            }
            return NONE
        }
    }
}