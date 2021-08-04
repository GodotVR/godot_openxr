package org.godotengine.plugin.vr.openxr

import org.godotengine.godot.Godot
import org.godotengine.godot.plugin.GodotPlugin

class GodotOpenXRPlugin(godot: Godot) : GodotPlugin(godot) {

  companion object {
    init {
      System.loadLibrary("openxr_loader")
      System.loadLibrary("godot_openxr")
    }
  }

  override fun getPluginName() = "GodotOpenXR"

  override fun getPluginGDNativeLibrariesPaths() =
    setOf("addons/godot-openxr/godot_openxr.gdnlib")
}