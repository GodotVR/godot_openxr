package org.godotengine.plugin.vr.openxr.api

/**
 * See https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XrColorSpaceFB for reference.
 */
enum class XrColorSpace(internal val value: Int) {
    /**
     *  No color correction, not recommended for production use.
     */
    UNMANAGED(0),

    /**
     *  Standard Rec. 2020 chromacities.
     *  This is the preferred color space for standardized color across all Oculus HMDs
     *  with D65 white point.
     */
    REC2020(1),

    /**
     * Standard Rec. 709 chromaticities, similar to sRGB.
     */
    REC709(2),

    /**
     * Unique color space, between P3 and Adobe RGB using D75 white point.
     *
     * Color Space Details with Chromacity Primaries in CIE 1931 xy:
     *     Red: (0.666, 0.334)
     *     Green: (0.238, 0.714)
     *     Blue: (0.139, 0.053)
     *     White: (0.298, 0.318)
     */
    RIFT_CV1(3),

    /**
     * Unique color space. Similar to Rec 709 using D75.
     *
     * Color Space Details with Chromacity Primaries in CIE 1931 xy:
     *     Red: (0.640, 0.330)
     *     Green: (0.292, 0.586)
     *     Blue: (0.156, 0.058)
     *     White: (0.298, 0.318)
     */
    RIFT_S(4),

    /**
     * Unique color space. Similar to Rift CV1 using D75 white point
     *
     * Color Space Details with Chromacity Primaries in CIE 1931 xy:
     *     Red: (0.661, 0.338)
     *     Green: (0.228, 0.718)
     *     Blue: (0.142, 0.042)
     *     White: (0.298, 0.318)
     */
    QUEST(5),

    /**
     * Similar to DCI-P3, but uses D65 white point instead.
     *
     * Color Space Details with Chromacity Primaries in CIE 1931 xy:
     *     Red: (0.680, 0.320)
     *     Green: (0.265, 0.690)
     *     Blue: (0.150, 0.060)
     *     White: (0.313, 0.329)
     */
    P3(6),

    /**
     * Standard Adobe chromacities.
     */
    ADOBE_RGB(7);

    companion object {
        fun toXrColorSpace(value: Int): XrColorSpace {
            for (colorSpace in values()) {
                if (colorSpace.value == value) {
                    return colorSpace
                }
            }
            return UNMANAGED
        }
    }
}