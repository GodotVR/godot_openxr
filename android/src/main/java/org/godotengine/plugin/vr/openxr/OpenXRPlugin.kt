package org.godotengine.plugin.vr.openxr

import android.util.Log
import org.godotengine.godot.Godot
import org.godotengine.godot.plugin.GodotPlugin
import java.util.concurrent.ConcurrentLinkedQueue
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

/**
 * Driver class for the Godot OpenXR plugin.
 *
 * This is used only on Android (Oculus Quest) devices and helps provide
 * access to the plugin functionality to the Android layer.
 */
class OpenXRPlugin(godot: Godot): GodotPlugin(godot) {
  companion object {
    private val TAG = OpenXRPlugin::class.java.simpleName
    private var loadedSharedLibs = false

    init {
      loadedSharedLibs = try {
        System.loadLibrary("openxr_loader")
        System.loadLibrary("godot_openxr")
        true
      } catch (e: UnsatisfiedLinkError) {
        Log.e(TAG, "Unable to load the godot_openxr shared libraries")
        false
      }
    }
  }

  /**
   * Used to listen to events dispatched by the OpenXR plugin.
   */
  interface EventListener {
    /**
     * Notifies when the headset is mounted.
     */
    fun onHeadsetMounted()

    /**
     * Notifies when the headset is unmounted.
     */
    fun onHeadsetUnmounted()

    /**
     * Notifies when the app gains focus.
     */
    fun onFocusGained()

    /**
     * Notifies when the app loses focus.
     */
    fun onFocusLost()

    /**
     * Notifies when the session has started.
     */
    fun onSessionBegun()

    /**
     * Notifies when the session is about to end.
     */
    fun onSessionEnding()
  }

  private val eventListeners = ConcurrentLinkedQueue<EventListener>()

  override fun getPluginName() = "OpenXR"

  override fun getPluginGDNativeLibrariesPaths() = setOf("addons/godot-openxr/config/godot_openxr.gdnlib")

  override fun onGLSurfaceCreated(gl: GL10, config: EGLConfig) {
    super.onGLSurfaceCreated(gl, config)
    invokeNative {
      initializeWrapper()
    }
  }

  override fun onMainDestroy() {
    super.onMainDestroy()
    invokeNative {
      runOnRenderThread {
        uninitializeWrapper()
      }
    }
  }

  fun registerEventListener(listener: EventListener) {
    eventListeners.add(listener)
  }

  fun unregisterEventListener(listener: EventListener) {
    eventListeners.remove(listener);
  }

  private fun onHeadsetMounted() = eventListeners.forEach { it.onHeadsetMounted() }
  private fun onHeadsetUnmounted() = eventListeners.forEach { it.onHeadsetUnmounted() }
  private fun onFocusGained() = eventListeners.forEach { it.onFocusGained() }
  private fun onFocusLost() = eventListeners.forEach { it.onFocusLost() }
  private fun onSessionBegun() = eventListeners.forEach { it.onSessionBegun() }
  private fun onSessionEnding() = eventListeners.forEach { it.onSessionEnding() }

  internal inline fun invokeNative(body: () -> Unit) {
    if (loadedSharedLibs) {
      body()
    }
  }

  internal inline fun <reified T> invokeNativeWithDefault(defaultValue: T, body: () -> T): T {
    return if (loadedSharedLibs) {
      body()
    } else {
      defaultValue
    }
  }

  private external fun initializeWrapper()

  private external fun uninitializeWrapper()
}
