package org.godotengine.godotopenxrdemo

import org.godotengine.godot.FullScreenGodotApp
import org.godotengine.godot.xr.XRMode

class MainActivity : FullScreenGodotApp() {

    // Override the engine starting parameters to indicate we want VR mode.
    override fun getCommandLine(): List<String> {
        return listOf(XRMode.OVR.cmdLineArg)
    }
}