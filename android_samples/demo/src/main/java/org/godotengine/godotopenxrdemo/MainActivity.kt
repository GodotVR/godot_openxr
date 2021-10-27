package org.godotengine.godotopenxrdemo

import org.godotengine.godot.FullScreenGodotApp
import org.godotengine.godot.xr.XRMode
import java.util.*

class MainActivity : FullScreenGodotApp() {

    // Override the engine starting parameters to indicate we want VR mode.
    override fun getCommandLine(): List<String> {
        return if (BuildConfig.FLAVOR == "ovr") {
            listOf(XRMode.OVR.cmdLineArg)
        } else {
            Collections.emptyList()
        }
    }
}