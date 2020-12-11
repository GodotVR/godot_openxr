#GDNative driver for OpenXR

## Versions

Requires Godot 3.2.4 (currently in beta), does not (yet) work with Godot 4.

Note: The godot engine (the full engine, not this plugin) must be compiled in release_debug mode, i.e. with

    scons platform=x11 target=release_debug

If godot engine is compiled with `target=debug` instead, you might get a (bogus) error from monado

    client_gl_compositor_init - Needed extension GL_EXT_memory_object_fd not supported

On the other hand the godot_openxr plugin can be compiled in debug mode without issues.

## Status

* This plugin only supports Linux/X11. Windows support is in early WIP.
* VR is tested with SteamVR and Monado
* Explicit support for controller input with interaction profile:
  * Valve Index Controller
  * Monado Playstation Move Controller
* Limited support for other controllers via Khronos Simple interaction profile.

## Building this module

In order to compile this module you will have to clone the source code to disk. You will need a C/C++ compiler, python and scons installed. This is the same toolchain you will need in order to compile Godot from master. The documentation on Godot is a very good place to read up on this. It is too much information to duplicate here.
You will also need cmake if you're compiling the OpenXR SDK loader

### Godot-cpp

Currently this project includes the godot-cpp repository as a submodule.
If you do not already have this repositories downloaded you can execute:
```
git submodule update --init --recursive
```
To download the required version.
This will also include the `godot-headers` submodule.

This submodule needs to be compiled with the following
```
cd godot-cpp
scons platform=<platform> target=release generate_bindings=yes
cd ..
```
Replace `<platform>` with `linux` or `windows` depending on your platform.

**note** As we rely on Godot 3.2.4 which is still in beta `godot-headers` hasn't been updated with a new `api.json` file. For now add `custom_api_file=../api.json` to the scons command up above so:
```
scons platform=windows target=release generate_bindings=yes custom_api_file=../api.json
```

### OpenXR SDK loader

OpenXR on desktop PCs usually requires using a loader provided by Khronos, you can find the source here: https://github.com/KhronosGroup/OpenXR-SDK

On Linux the loader and include files should be installed system wide via package manager (or manually) within the `/usr` folder structure and they should be picked up automatically.

On Windows a precompiled version of the loaded can be found in `openxr_loader_windows`, currently only the x86 version is used.
You can download the latest version from: https://github.com/KhronosGroup/OpenXR-SDK/releases

### Compiling the plugin

If everything is in place compiling should be pretty straight forward

For Windows: ```scons platform=windows```
For Linux: ```scons platform=linux```

On older linux distributions (e.g. Ubuntu 18.04) the plugin may not build with gcc.
In this case install clang and compile the plugin with ```scons use_llvm=yes platform=linux```

Currently there is no proper OpenXR support on OSX.

The compiled plugin and related files will be placed in `demo/addons/`.
When using godot_openxr in another project, copy this directory.

If you compile with gcc and encounter the error message
```
sorry, unimplemented: non-trivial designated initializers not supported
```
it probably means your gcc is too old (e.g. Ubuntu 18.04).
The easiest way around this is to compile the plugin with clang instead.
```
apt install clang
scons platform=linux use_llvm=yes
```

### Android

Generic OpenXR on Android is only available as a Tech preview. It is not endorsed by Khronos (yet?).

Only the monado runtime implements the preview of the generic Android OpenXR loader.

1. Setup Monado

Get Monado with `git clone https://gitlab.freedesktop.org/monado/monado.git`. Open in Android Studio, build and run on your android device.

2. Get the preview broker

This is currently not open source. It will be open sourced once the approach to its implementation stabilizes.

Install this apk on your android device:
https://cdn.discordapp.com/attachments/608975708638806017/806995278199914496/broker_impl-debug.apk

3. Compile the plugin

The godot-cpp bindings do have to be generated manually:

```
cd godot-cpp
scons platform=linux generate_bindings=yes -j32
```

Then open the project in android-studio and click compile.

aar files are created in src/build/outputs/aar/

Good luck.

## Debugging

If you want to debug the module make sure you use a copy of the godot binary build with either `target=release_debug` or `target=debug` provided to scons and that you build the plugin using `target=debug`. This will ensure debugging symbol files are created.

### using VS code

I've only tested debugging using VS code on Windows with the MSVC C++ compiler but VS Code has good templates to get you up and running on Linux or on MinGW as well. Make sure you have the C/C++ extensions installed and the debugger installed.

Either manually create a .vscode folder or let vs code do this for you and setup the following two files:

`launch.json`
```
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Runtime Launch",
            "type": "cppvsdbg",
            "request": "launch",
            "program": "/path-to-godot/bin/godot.windows.opt.tools.64.exe",
            "args": [
                "--path",
                "demo"
            ],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "console": "integratedTerminal",
            "preLaunchTask": "build"
        },
        {
            "name": "Editor Launch",
            "type": "cppvsdbg",
            "request": "launch",
            "program": "/path-to-godot/bin/godot.windows.opt.tools.64.exe",
            "args": [
                "-e",
                "--path",
                "demo"
            ],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "console": "externalTerminal",
            "preLaunchTask": "build"
        }
    ]
}
```
Be sure to change `path-to-godot` to the actual path that contains your godot source and adjust the name of the godot executable as it may change depending on your compiler settings.
Note that two options are provided, launching the demo project, or opening the demo project in the Godot editor.

`tasks.json`
```
{
    // See https://go.microsoft.com/fwlink/?LinkId=733558
    // for the documentation about the tasks.json format
    "version": "2.0.0",
    "tasks": [
        {
            "label": "build",
            "type": "shell",
            "command": "scons",
            "group": "build",
            "args": [
                "platform=windows",
                "target=debug",
                "-j8"
            ],
            "problemMatcher": "$msCompile"
        }
    ]
}
```
Note that our build script does *not* build godot-cpp!

## Testing

After compiling the plugin, start Godot, open the godot_openxr/demo project and click play.

## Demo

There is a demo project contained within this repository as well that shows how to set things up.

## Using this module in your own project

Example: Converting a godot-openvr project

1. Remove the `addons/godot-openvr` directory from the project.
2. Copy the `godot-openxr/demo/addons/godot-openxr` directory to `your_project/addons/`.
3. If your project was already set up to use OpenVR, find `ARVRServer.find_interface("OpenVR")` and replace `"OpenVR"` with `"OpenXR"`.

Since the module is laid out like godot-openvr, the basic documentation for integrating OpenVR into a project also applies to OpenXR.

The only differences should be: Since there is no OpenXR asset on the store, the godot-openxr directory has to be manually put into the project's addon/ directory and in the project's gdscript the `OpenXR` string has to be used in the `find_interface()` call.

## Hooks

When contributing to the source code for the plugin we highly recommend you installed clang-format and copy the contents of the `hooks` folder into the folder `.git/hooks/`.
This will ensure clang-format is run on any changed files before commiting the changes to github and prevent disappointment when formatting issues prevent changes from being merged.

## License

The source code for the module is released under MIT license (see license file).

## About this repository

This repository is a fork of godot_openhmd which was created and is maintained by [Bastiaan Olij](https://github.com/BastiaanOlij) a.k.a. Mux213
OpenXR implementation was created by [Christoph Haag](https://github.com/ChristophHaag/)

Originally hosted on https://gitlab.freedesktop.org/monado/demos/godot_openxr now lives on https://github.com/GodotVR/godot_openxr
