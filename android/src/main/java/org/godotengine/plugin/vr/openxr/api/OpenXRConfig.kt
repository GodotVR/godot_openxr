@file:JvmName("OpenXRConfig")

package org.godotengine.plugin.vr.openxr.api

import org.godotengine.plugin.vr.openxr.OpenXRPlugin

/**
 * See https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XrColorSpaceFB for reference.
 */
enum class XrColorSpace(internal val value: Int) {
    /**
     *  No color correction, not recommended for production use.
     */
    XR_COLOR_SPACE_UNMANAGED(0),

    /**
     *  Standard Rec. 2020 chromacities.
     *  This is the preferred color space for standardized color across all Oculus HMDs
     *  with D65 white point.
     */
    XR_COLOR_SPACE_REC2020(1),

    /**
     * Standard Rec. 709 chromaticities, similar to sRGB.
     */
    XR_COLOR_SPACE_REC709(2),

    /**
     * Unique color space, between P3 and Adobe RGB using D75 white point.
     *
     * Color Space Details with Chromacity Primaries in CIE 1931 xy:
     *     Red: (0.666, 0.334)
     *     Green: (0.238, 0.714)
     *     Blue: (0.139, 0.053)
     *     White: (0.298, 0.318)
     */
    XR_COLOR_SPACE_RIFT_CV1(3),

    /**
     * Unique color space. Similar to Rec 709 using D75.
     *
     * Color Space Details with Chromacity Primaries in CIE 1931 xy:
     *     Red: (0.640, 0.330)
     *     Green: (0.292, 0.586)
     *     Blue: (0.156, 0.058)
     *     White: (0.298, 0.318)
     */
    XR_COLOR_SPACE_RIFT_S(4),

    /**
     * Unique color space. Similar to Rift CV1 using D75 white point
     *
     * Color Space Details with Chromacity Primaries in CIE 1931 xy:
     *     Red: (0.661, 0.338)
     *     Green: (0.228, 0.718)
     *     Blue: (0.142, 0.042)
     *     White: (0.298, 0.318)
     */
    XR_COLOR_SPACE_QUEST(5),

    /**
     * Similar to DCI-P3, but uses D65 white point instead.
     *
     * Color Space Details with Chromacity Primaries in CIE 1931 xy:
     *     Red: (0.680, 0.320)
     *     Green: (0.265, 0.690)
     *     Blue: (0.150, 0.060)
     *     White: (0.313, 0.329)
     */
    XR_COLOR_SPACE_P3(6),

    /**
     * Standard Adobe chromacities.
     */
    XR_COLOR_SPACE_ADOBE_RGB(7);

    companion object {
        fun toXrColorSpace(value: Int): XrColorSpace {
            for (colorSpace in values()) {
                if (colorSpace.value == value) {
                    return colorSpace
                }
            }
            return XR_COLOR_SPACE_UNMANAGED
        }
    }
}

/**
 * Used to retrieve the device color space.
 */
fun OpenXRPlugin.getColorSpace() = XrColorSpace.toXrColorSpace(nativeGetXrColorSpace())

private external fun nativeGetXrColorSpace(): Int

/**
 * Update the device color space.
 */
fun OpenXRPlugin.setColorSpace(colorSpace: XrColorSpace) = nativeSetXrColorSpace(colorSpace.value)

private external fun nativeSetXrColorSpace(colorSpace: Int)

/**
 * Retrieve the list of color spaces supported on this device.
 */
fun OpenXRPlugin.getAvailableColorSpaces() : Array<XrColorSpace> {
    val colorSpaces = nativeGetAvailableXrColorSpaces()
    return Array(colorSpaces.size) { XrColorSpace.toXrColorSpace(colorSpaces[it]) }
}

private external fun nativeGetAvailableXrColorSpaces(): IntArray

/**
 * Retrieve the device current refresh rate.
 */
external fun OpenXRPlugin.getRefreshRate(): Double

/**
 * Update the device's refresh rate.
 */
external fun OpenXRPlugin.setRefreshRate(refreshRate: Double)

/**
 * Retrieve the list of refresh rates supported by this device.
 */
external fun OpenXRPlugin.getAvailableRefreshRates(): DoubleArray