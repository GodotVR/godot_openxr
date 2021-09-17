@file:JvmName("OpenXRConfig")

package org.godotengine.plugin.vr.openxr.api

import org.godotengine.plugin.vr.openxr.OpenXRPlugin

/**
 * Used to retrieve the system color space.
 */
fun OpenXRPlugin.getColorSpace(): XrColorSpace {
    return invokeNativeWithDefault(XrColorSpace.UNMANAGED) {
        XrColorSpace.toXrColorSpace(nativeGetXrColorSpace())
    }
}

private external fun nativeGetXrColorSpace(): Int

/**
 * Update the system color space.
 */
fun OpenXRPlugin.setColorSpace(colorSpace: XrColorSpace) {
    invokeNative {
        nativeSetXrColorSpace(colorSpace.value)
    }
}

private external fun nativeSetXrColorSpace(colorSpace: Int)

/**
 * Retrieve the list of color spaces supported on this system.
 */
fun OpenXRPlugin.getAvailableColorSpaces() : Array<XrColorSpace> {
    return invokeNativeWithDefault(emptyArray()) {
        val colorSpaces = nativeGetAvailableXrColorSpaces()
        return Array(colorSpaces.size) { XrColorSpace.toXrColorSpace(colorSpaces[it]) }
    }
}

private external fun nativeGetAvailableXrColorSpaces(): IntArray

/**
 * Retrieve the system current refresh rate.
 */
fun OpenXRPlugin.getRefreshRate(): Double {
    return invokeNativeWithDefault(0.0) {
        nativeGetRefreshRate()
    }
}

private external fun nativeGetRefreshRate(): Double

/**
 * Update the system's refresh rate.
 */
fun OpenXRPlugin.setRefreshRate(refreshRate: Double) {
    invokeNative { nativeSetRefreshRate(refreshRate) }
}

private external fun nativeSetRefreshRate(refreshRate: Double)

/**
 * Retrieve the list of refresh rates supported by this system.
 */
fun OpenXRPlugin.getAvailableRefreshRates(): DoubleArray {
    return invokeNativeWithDefault(doubleArrayOf()) {
        nativeGetAvailableRefreshRates()
    }
}

private external fun nativeGetAvailableRefreshRates(): DoubleArray

/**
 * Return the current [SystemType] type.
 */
fun OpenXRPlugin.getSystemType(): SystemType {
    return invokeNativeWithDefault(SystemType.UNKNOWN) {
        SystemType.toSystemType(nativeGetSystemName())
    }
}

private external fun nativeGetSystemName(): String

/**
 * Retrieve the system's cpu performance settings level.
 */
fun OpenXRPlugin.getCpuLevel(): PerformanceSettingsLevel {
    return invokeNativeWithDefault(PerformanceSettingsLevel.SUSTAINED_HIGH) {
        PerformanceSettingsLevel.toPerformanceSettingsLevel(nativeGetCpuLevel())
    }
}

private external fun nativeGetCpuLevel(): Int

/**
 * Update the system's cpu performance settings level.
 */
fun OpenXRPlugin.setCpuLevel(level: PerformanceSettingsLevel): Boolean {
    return invokeNativeWithDefault(false) {
        nativeSetCpuLevel(level.value)
    }
}

private external fun nativeSetCpuLevel(level: Int): Boolean

/**
 * Retrieve the system's gpu performance settings level.
 */
fun OpenXRPlugin.getGpuLevel(): PerformanceSettingsLevel {
    return invokeNativeWithDefault(PerformanceSettingsLevel.SUSTAINED_HIGH) {
        PerformanceSettingsLevel.toPerformanceSettingsLevel(nativeGetGpuLevel())
    }
}

private external fun nativeGetGpuLevel(): Int

/**
 * Update the system's gpu performance settings level.
 */
fun OpenXRPlugin.setGpuLevel(level: PerformanceSettingsLevel): Boolean {
    return invokeNativeWithDefault(false) {
        nativeSetGpuLevel(level.value)
    }
}

private external fun nativeSetGpuLevel(level: Int): Boolean

/**
 * Retrieve the factor by which the render target size is multiplied.
 */
fun OpenXRPlugin.getRenderTargetSizeMultiplier(): Float {
    return invokeNativeWithDefault(1f) {
        nativeGetRenderTargetSizeMultiplier()
    }
}

private external fun nativeGetRenderTargetSizeMultiplier(): Float

/**
 * Set the factor by which to multiply the recommended render target size for the app.
 */
fun OpenXRPlugin.setRenderTargetSizeMultiplier(multiplier: Float): Boolean {
    return invokeNativeWithDefault(false) {
        nativeSetRenderTargetSizeMultiplier(multiplier)
    }
}

private external fun nativeSetRenderTargetSizeMultiplier(multipler: Float): Boolean

/**
 * Set the system foveation level
 */
fun OpenXRPlugin.setFoveationLevel(level: FoveationLevel, isDynamic: Boolean) {
    invokeNative {
        nativeSetFoveationLevel(level.value, isDynamic)
    }
}

private external fun nativeSetFoveationLevel(level: Int, isDynamic: Boolean)

/**
 * Start passthrough on the device.
 */
fun OpenXRPlugin.startPassthrough(): Boolean {
    return invokeNativeWithDefault(false) {
        nativeStartPassthrough()
    }
}

private external fun nativeStartPassthrough(): Boolean

/**
 * Stop passthrough on the device.
 */
fun OpenXRPlugin.stopPassthrough() {
    invokeNative { nativeStopPassthrough() }
}

private external fun nativeStopPassthrough()
