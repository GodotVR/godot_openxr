package org.godotengine.plugin.vr.openxr.api

/**
 * For reference, see https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XrPerfSettingsLevelEXT
 */
enum class PerformanceSettingsLevel(internal val value: Int) {
    /**
     * Used by the application to indicate that it enters a non-XR section
     * (head-locked / static screen), during which power savings are to be prioritized.
     */
    POWER_SAVINGS(0),

    /**
     * Used by the application to indicate that it enters a low and stable complexity section,
     * during which reducing power is more important than occasional late rendering frames.
     */
    SUSTAINED_LOW(25),

    /**
     * Used by the application to indicate that it enters a high or dynamic complexity section,
     * during which the XR Runtime strives for consistent XR compositing and frame rendering within
     * a thermally sustainable range(*).
     *
     * This is the default if the application does not specify its performance level.
     */
    SUSTAINED_HIGH(50),

    /**
     * Used to indicate that the application enters a section with very high complexity,
     * during which the XR Runtime is allowed to step up beyond the thermally sustainable range.
     *
     * This is meant to be used for short-term durations (< 30 seconds).
     */
    BOOST(75);

    companion object {
        fun toPerformanceSettingsLevel(value: Int): PerformanceSettingsLevel {
            for (level in values()) {
                if (level.value == value) {
                    return level
                }
            }

            return SUSTAINED_HIGH
        }
    }
}