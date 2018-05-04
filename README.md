# OpenHMD GDNative driver for OpenHMD

I'll add more info here soon, this is still a work in progress and likely to change drastically.

The leading version of this repository now lives at:
https://github.com/GodotVR/godot_openhmd

Building this module
--------------------
In order to compile this module you will have to clone the source code to disk. You will need a C/C++ compiler, python and scons installed. This is the same toolchain you will need in order to compile Godot from master. The documentation on Godot is a very good place to read up on this. It is too much information to duplicate here.

When cloning this repository make sure you also install all the submodules.
Either clone with `git clone --recursive` or execute:
```
git submodules init
git submodules update
```
after cloning.

If you're pulling a newer version (re)execute the `git submodules update` command to make sure the modules are up to date. If you're interested in using different branches of the 3rd party modules just CD into their subfolder and you can execute git commands on those repositories. 

*Compiling*
If everything into place compiling should be pretty straight forward

For Linux: ```scons platform=linux```
For OSX: ```scons platform=osx```
For Windows: ```scons platform=windows```

License
-------
The source code for the module is released under MIT license (see license file).
Note that the related products used, hidapi, libusb, openhmd and Godot itself all have their own licenses.

About this repository
---------------------
This repository was created by and is maintained by Bastiaan Olij a.k.a. Mux213

You can follow me on twitter for regular updates here:
https://twitter.com/mux213

Videos about my work with Godot including tutorials on working with VR in Godot can by found on my youtube page:
https://www.youtube.com/channel/UCrbLJYzJjDf2p-vJC011lYw




# PARKED FOR NOW... PLEASE IGNORE

The info below shows how to compile each 3rd party library however we currently do not use any of that. The scons file has all compilation instructions for all 3rd party tools.
I'm leaving these instructions in the readme file because this is what we do want to move too to prevent issues coming from 3rd party modules changing or having special instructions for individual platforms.

## X11 (Linux)

Clone this repository using `git clone --recursive` so all the needed packages will be cloned together with the source code. 

After that build libusb, openhmd and hidapi, using the following commands: 

```
export CORES=$(grep processor /proc/cpuinfo | wc -l)
cd libusb ; ./bootstrap.sh ; ./configure ; make -j $CORES ; cd ..
cd hidapi ; ./bootstrap ; ./configure ; make -j $CORES ; cd ..
cd OpenHMD ; ./autogen.sh ; ./configure ; make -j $CORES ; cd ..
```

To make things easier, set the `GODOT_ROOT` environment variable with the folder that holds the GODOT source code, for example, like this: 

```
export GODOT_ROOT=../godot.git
```

The you can build by issuing the followin command: 

```GODOT_HEADERS=$GODOT_ROOT/modules/gdnative/include/  /bin/scons -j $CORES```

Last, you can test your build by issuing:

`$GODOT_ROOT/bin/godot.x11.tools.64 demo/project.godot`

 or
 
`godot demo/project.godot`

## Windows

## OSX
