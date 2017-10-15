#!python
import os, subprocess

# Local dependency paths, adapt them to your setup
#godot_headers_path = ARGUMENTS.get("headers", "godot_headers/")
godot_headers_path = ARGUMENTS.get("headers", "../../godot3-git/modules/gdnative/include")
godot_glad_path = ARGUMENTS.get("headers", "glad")

target = ARGUMENTS.get("target", "debug")

# platform= makes it in line with Godots scons file, keeping p for backwards compatibility
platform = ARGUMENTS.get("p", "linux")
platform = ARGUMENTS.get("platform", platform)

# This makes sure to keep the session environment variables on windows, 
# that way you can run scons in a vs 2017 prompt and it will find all the required tools
env = Environment()
if platform == "windows":
    env = Environment(ENV = os.environ)

if ARGUMENTS.get("use_llvm", "no") == "yes":
    env["CXX"] = "clang++"

def add_sources(sources, directory):
    for file in os.listdir(directory):
        if file.endswith('.c'):
            sources.append(directory + '/' + file)

sources = []

if platform == "osx":
    env.Append(CCFLAGS = ['-g','-O3', '-arch', 'x86_64'])
    env.Append(LINKFLAGS = ['-arch', 'x86_64'])
    env.Append(LINKFLAGS=['-framework', 'Cocoa', '-framework', 'OpenGL', '-framework', 'IOKit'])
    env.Append(LIBS=['pthread'])

if platform == "linux":
    env.Append(CCFLAGS = ['-fPIC', '-g','-O3', '-std=c++14'])

if platform == "windows":
    if target == "debug":
        env.Append(CCFLAGS = ['-EHsc', '-D_DEBUG', '/MDd'])
    else:
        env.Append(CCFLAGS = ['-O2', '-EHsc', '-DNDEBUG', '/MD'])
    env.Append(LIBS=["opengl32", "setupapi", "advapi32.lib"])

####################################################################################################################################
# Link in glad
sources.append(godot_glad_path + "\glad.c")

####################################################################################################################################
# Link in libusb, but for now just for linux
if platform == 'linux':
    env.Append(CPPPATH=["libusb/libusb"])
    env.Append(CPPPATH=["libusb/libusb/os"])

    libusb_sources = [
        "core.c",
        "descriptor.c",
        "hotplug.c",
        "io.c",
        "strerror.c",
        "sync.c"
    ]

    sources.append(["libusb/libusb/" + file for file in libusb_sources])

    if platform == 'x11':
        sources.append("libusb/libusb/os/linux_netlink.c")
        sources.append("libusb/libusb/os/linux_usbfs.c")
        sources.append("/libusb/libusb/os/poll_posix.c")
        sources.append("libusb/libusb/os/threads_posix.c")
        env.Append(CPPDEFINES=["OS_LINUX"])
#    elif platform == 'windows':
#        sources.append("libusb/libusb/os/windows_nt_common.c")
#        sources.append("libusb/libusb/os/windows_usbdk.c")
#        sources.append("libusb/libusb/os/windows_winusb.c")
#        sources.append("libusb/libusb/os/poll_windows.c")
#        sources.append("libusb/libusb/os/threads_windows.c")
#        env,Append(CPPDEFINES=["OS_WINDOWS"])
#    elif platform == 'osx':
#        sources.append("libusb/libusb/os/darwin_usb.c")
#        sources.append("libusb/libusb/os/poll_posix.c")
#        sources.append("libusb/libusb/os/threads_posix.c")
#        env.Append(CPPDEFINES=["OS_DARWIN"])

####################################################################################################################################
# Link in hidapi
hidapi_headers = "hidapi/hidapi/"
env.Append(CPPPATH=[hidapi_headers])

if platform == 'windows':
    sources.append("hidapi/windows/hid.c" )
elif platform == 'x11':
    # If we can use the libusb version it should allow us to undo our detect.py changes
    # See thirdparty/hidapi/linux/README.txt and thirdparty/hidapi/udev/99-hid-rules for more info
    # env_openhmd.add_source_files(env.modules_sources, [ "#thirdparty/hidapi/linux/hid.c" ])
    sources.append("hidapi/libusb/hid.c")
elif platform == 'osx':
    sources.append("hidapi/mac/hid.c")

####################################################################################################################################
# Link in openhmd, we're linking in static.
env.Append(CFLAGS=["-DOHMD_STATIC"])
env.Append(CPPFLAGS=["-DOHMD_STATIC"])

# We don't include android because we're not compiling this for android at this time.
# Our native mobile VR class already handles android.
# We do include the Vive so we have basic native support even though we have an OpenVR implementation
env.Append(CFLAGS=["-DDRIVER_OCULUS_RIFT"])
env.Append(CFLAGS=["-DDRIVER_DEEPOON"])
env.Append(CFLAGS=["-DDRIVER_HTC_VIVE"])
env.Append(CFLAGS=["-DDRIVER_PSVR"])
env.Append(CFLAGS=["-DDRIVER_NOLO"])
#env.Append(CFLAGS=["-DDRIVER_EXTERNAL"])
#env.Append(CFLAGS=["-DDRIVER_ANDROID"])

# miniz is already compiled within Godot so just want the headers here...
# env_openhmd.Append(CFLAGS=["-DMINIZ_HEADER_FILE_ONLY"])

openhmd_headers = "openhmd/include/"
env.Append(CPPPATH=[openhmd_headers])

openhmd_dir = "openhmd/src/"

openhmd_sources = [
    "fusion.c",
    "omath.c",
    "openhmd.c",
    "queue.c",
    "shaders.c",
    "ext_deps/mjson.c",
#    "drv_android/android.c",
    "drv_deepoon/deepoon.c",
    "drv_deepoon/packet.c",
    "drv_dummy/dummy.c",
 #   "drv_external/external.c",
    "drv_htc_vive/packet.c",
    "drv_htc_vive/vive.c",
    "drv_oculus_rift/packet.c",
    "drv_oculus_rift/rift.c",
    "drv_psvr/packet.c",
    "drv_psvr/psvr.c",
    "drv_nolo/nolo.c",
    "drv_nolo/packet.c"
]

sources.append([openhmd_dir + file for file in openhmd_sources])

if platform == 'windows':
    sources.append("openhmd/src/platform-win32.c" )
else:
    sources.append("openhmd/src/platform-posix.c" )

####################################################################################################################################
# and add our main project
env.Append(CPPPATH=['.', godot_headers_path, godot_glad_path])

add_sources(sources, "src")

library = env.SharedLibrary(target='demo/bin/godot_openhmd', source=sources)
Default(library)
