# GDNative driver for OpenXR

Versions
--------

Requires Godot 3.1.

Note: The godot engine (the full engine, not this plugin) must be compiled in release_debug mode, i.e. with

    scons platform=x11 target=release_debug

If godot engine is compiled with `target=debug` instead, you might get a (bogus) error from monado

    client_gl_compositor_init - Needed extension GL_EXT_memory_object_fd not supported

On the other hand the godot_openxr plugin can be compiled in debug mode without issues.

Status
------

* This plugin only supports Linux/X11.
* HMD poses and HMD rendering work.
* Controller/Actions are unimplemented.
* The code structure could be improved.

Building this module
--------------------
In order to compile this module you will have to clone the source code to disk. You will need a C/C++ compiler, python and scons installed. This is the same toolchain you will need in order to compile Godot from master. The documentation on Godot is a very good place to read up on this. It is too much information to duplicate here.

This module presumes that the OpenXR headers openxr/openxr.h and openxr/openxr_platform.h are installed in a location where they can be included without setup, and that the OpenXR loader libopenxr_api.so can be linked without special setup, for example /usr/include/openxr/openxr.h and /usr/lib/libopenxr_api.so.

TODO: add openxr loader and headers as a submodule

*Compiling*
If everything is in place compiling should be pretty straight forward

For Linux: ```scons platform=linux```
For OSX: ```scons platform=osx```
For Windows: ```scons platform=windows```

The compiled plugin and related files will be placed in `demo/addons/`. When using godot_openxr in another project, copy this directory.

Testing
-------
Start Godot, open the godot_openxr/demo project and click play.

License
-------
The source code for the module is released under MIT license (see license file).

About this repository
---------------------
This repository is a fork of godot_openhmd which was created and is maintained by Bastiaan Olij a.k.a. Mux213
