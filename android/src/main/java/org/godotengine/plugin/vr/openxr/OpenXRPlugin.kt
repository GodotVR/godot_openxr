package org.godotengine.plugin.vr.openxr

import org.godotengine.godot.Godot
import org.godotengine.godot.plugin.GodotPlugin
import java.util.concurrent.ConcurrentLinkedQueue

/**
 * Driver class for the Godot OpenXR plugin.
 *
 * This is used only on Android (Oculus Quest) devices and helps provide
 * access to the plugin functionality to the Android layer.
 */
class OpenXRPlugin(godot: Godot): GodotPlugin(godot) {
  companion object {
    init {
      System.loadLibrary("openxr_loader")
      System.loadLibrary("godot_openxr")
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
  }

  private val eventListeners = ConcurrentLinkedQueue<EventListener>()

  override fun getPluginName() = "OpenXR"

  override fun getPluginGDNativeLibrariesPaths() = setOf("godot_openxr.gdnlib")

  fun registerEventListener(listener: EventListener) {
    eventListeners.add(listener)
  }

  fun unregisterEventListener(listener: EventListener) {
    eventListeners.remove(listener);
  }
}