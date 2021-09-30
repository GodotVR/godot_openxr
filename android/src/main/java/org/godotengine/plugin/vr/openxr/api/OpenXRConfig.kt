@file:JvmName("OpenXRConfig")

package org.godotengine.plugin.vr.openxr.api

import org.godotengine.plugin.vr.openxr.OpenXRPlugin

/**
 * Used to retrieve the system color space.
 */
fun OpenXRPlugin.getColorSpace() = XrColorSpace.toXrColorSpace(nativeGetXrColorSpace())

private external fun nativeGetXrColorSpace(): Int

/**
 * Update the system color space.
 */
fun OpenXRPlugin.setColorSpace(colorSpace: XrColorSpace) = nativeSetXrColorSpace(colorSpace.value)

private external fun nativeSetXrColorSpace(colorSpace: Int)

/**
 * Retrieve the list of color spaces supported on this system.
 */
fun OpenXRPlugin.getAvailableColorSpaces() : Array<XrColorSpace> {
    val colorSpaces = nativeGetAvailableXrColorSpaces()
    return Array(colorSpaces.size) { XrColorSpace.toXrColorSpace(colorSpaces[it]) }
}

private external fun nativeGetAvailableXrColorSpaces(): IntArray

/**
 * Retrieve the system current refresh rate.
 */
external fun OpenXRPlugin.getRefreshRate(): Double

/**
 * Update the system's refresh rate.
 */
external fun OpenXRPlugin.setRefreshRate(refreshRate: Double)

/**
 * Retrieve the list of refresh rates supported by this system.
 */
external fun OpenXRPlugin.getAvailableRefreshRates(): DoubleArray

/**
 * Return the current [SystemType] type.
 */
fun OpenXRPlugin.getSystemType() = SystemType.toSystemType(nativeGetSystemName())

private external fun nativeGetSystemName(): String

/**
 * Retrieve the system's cpu performance settings level.
 */
fun OpenXRPlugin.getCpuLevel() = PerformanceSettingsLevel.toPerformanceSettingsLevel(nativeGetCpuLevel())

private external fun nativeGetCpuLevel(): Int

/**
 * Update the system's cpu performance settings level.
 */
fun OpenXRPlugin.setCpuLevel(level: PerformanceSettingsLevel) = nativeSetCpuLevel(level.value)

private external fun nativeSetCpuLevel(level: Int): Boolean

/**
 * Retrieve the system's gpu performance settings level.
 */
fun OpenXRPlugin.getGpuLevel() = PerformanceSettingsLevel.toPerformanceSettingsLevel(nativeGetGpuLevel())

private external fun nativeGetGpuLevel(): Int

/**
 * Update the system's gpu performance settings level.
 */
fun OpenXRPlugin.setGpuLevel(level: PerformanceSettingsLevel) = nativeSetGpuLevel(level.value)

private external fun nativeSetGpuLevel(level: Int): Boolean

/**
 * Retrieve the factor by which the render target size is multiplied.
 */
external fun OpenXRPlugin.getRenderTargetSizeMultiplier(): Float

/**
 * Set the factor by which to multiply the recommended render target size for the app.
 */
external fun OpenXRPlugin.setRenderTargetSizeMultiplier(multiplier: Float): Boolean

/**
 * Set the system foveation level
 */
fun OpenXRPlugin.setFoveationLevel(level: FoveationLevel, isDynamic: Boolean) = nativeSetFoveationLevel(level.value, isDynamic)

private external fun nativeSetFoveationLevel(level: Int, isDynamic: Boolean)
