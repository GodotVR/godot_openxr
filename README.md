# OpenHMD GDNative driver for OpenHMD

I'll add more info here soon, this is still a work in progress and likely to change drastically.

# License
The source code for the module is released under unlicense (see license file), note that the related products used, hidapi, libusb, openhmd and Godot itself all have their own licenses.

# Install

## X11 (Linux)

Clone this repository using `git clone --recursive` so all the needed packages will be cloned together with the source code. 

After that build libusb, openhmd and hidapi, using the following commands: 

```
cd libusb ; ./bootstrap.sh ; ./configure ; make ; cd ..
cd hidapi ; ./bootstrap ; ./configure ; make ; cd ..
cd OpenHMD ; ./autogen.sh ; ./configure ; make ; cd ..
```

To make things easier, set the `GODOT_ROOT` environment variable with the folder that holds the GODOT source code, for example, like this: 

```
export GODOT_ROOT=../godot.git
```

The you can build by issuing the followin command: 

```GODOT_HEADERS=$GODOT_ROOT/modules/gdnative/include/  /bin/scons```

Last, you can test your build by issuing:

`$GODOT_ROOT/bin/godot.x11.tools.64 demo/project.godot`

 or
 
`godot demo/project.godot`

## Windows

## OSX

