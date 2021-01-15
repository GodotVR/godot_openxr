#!python
import os, subprocess

# Reads variables from an optional file.
customs = ['../custom.py']
opts = Variables(customs, ARGUMENTS)

# Gets the standard flags CC, CCX, etc.
env = Environment(ENV = os.environ)

# Define our parameters
opts.Add(EnumVariable('platform', "Platform", 'windows', ['linux', 'windows']))
opts.Add(EnumVariable('target', "Compilation target", 'release', ['d', 'debug', 'r', 'release']))
opts.AddVariables(
    PathVariable('target_path', 'The path where the lib is installed.', 'demo/addons/godot-openxr/bin/'),
    PathVariable('target_name', 'The library name.', 'libgodot_openxr', PathVariable.PathAccept),
)
opts.Add(BoolVariable('use_llvm', "Use the LLVM / Clang compiler", 'no'))

# Updates the environment with the option variables.
opts.Update(env)

# Other needed paths
godot_glad_path = "glad/"
godot_headers_path = "godot-cpp/godot_headers/"
godot_cpp_path = "godot-cpp/"
godot_cpp_library = "libgodot-cpp"
target_path = env['target_path']

openxr_include_path = ""
openxr_library_path = ""

# Source files to include
sources = []

# Platform dependent settings
if env['platform'] == "windows":
    target_path += "win64/"
    godot_cpp_library += '.windows'
    openxr_include_path += "openxr_loader_windows/1.0.12/include/"
    openxr_library_path += "openxr_loader_windows/1.0.12/x64/lib"

    # Check some environment settings
    if env['use_llvm']:
        env['CXX'] = 'clang++'
        env['CC'] = 'clang'

        if env['target'] in ('debug', 'd'):
            env.Append(CCFLAGS = ['-fPIC', '-g3','-Og'])
            env.Append(CXXFLAGS = ['-fPIC', '-g3','-Og', '-std=c++17'])
        else:
            env.Append(CCFLAGS = ['-fPIC','-O3'])
            env.Append(CXXFLAGS = ['-fPIC','-O3', '-std=c++17'])
    else:
        # This makes sure to keep the session environment variables on windows,
        # that way you can run scons in a vs 2017 prompt and it will find all the required tools
        env.Append(ENV = os.environ)

        env.Append(CCFLAGS = ['-DWIN32', '-D_WIN32', '-D_WINDOWS', '-W3', '-GR', '-D_CRT_SECURE_NO_WARNINGS','-std:c++latest'])
        if env['target'] in ('debug', 'd'):
            env.Append(CCFLAGS = ['-Od', '-EHsc', '-D_DEBUG', '/MDd', '/Zi', '/FS'])
            env.Append(LINKFLAGS = ['/DEBUG'])
        else:
            env.Append(CCFLAGS = ['-O2', '-EHsc', '-DNDEBUG', '/MD'])

    # Do we need these?
    env.Append(LIBS = ["opengl32", "setupapi", "advapi32.lib"])

    # For now just include Glad on Windows, but maybe also use with Linux?
    env.Append(CPPPATH = [godot_glad_path])
    sources += Glob(godot_glad_path + '*.c')

elif env['platform'] == "linux":
    target_path += "linux/"
    godot_cpp_library += '.linux'

    # note, on linux the OpenXR SDK is installed in /usr and should be accessible
    if env['use_llvm']:
        env['CXX'] = 'clang++'
        env['CC'] = 'clang'

    if env['target'] in ('debug', 'd'):
        env.Append(CCFLAGS = ['-fPIC', '-ggdb','-O0'])
        env.Append(CXXFLAGS = ['-fPIC', '-ggdb','-O0'])
    else:
        env.Append(CCFLAGS = ['-fPIC', '-g','-O3'])
        env.Append(CXXFLAGS = ['-fPIC', '-g','-O3'])
    env.Append(CXXFLAGS = [ '-std=c++0x' ])
    env.Append(LINKFLAGS = [ '-Wl,-R,\'$$ORIGIN\'' ])

#elif env['platform'] == "osx":
#    # not tested
#
#    target_path += "win64/"
#    openxr_include_path += "??"
#    openxr_library_path += "??"
#
#    env.Append(CCFLAGS = ['-g','-O3', '-arch', 'x86_64'])
#    env.Append(LINKFLAGS = ['-arch', 'x86_64'])
#    env.Append(LINKFLAGS=['-framework', 'Cocoa', '-framework', 'OpenGL', '-framework', 'IOKit'])
#    env.Append(LIBS=['pthread'])

# Complete godot-cpp library path
if env['target'] in ('debug', 'd'):
    godot_cpp_library += '.debug.64'
    env.Append(CCFLAGS = [ '-DDEBUG' ])
else:
    godot_cpp_library += '.release.64'

####################################################################################################################################
# and add our main project

env.Append(CPPPATH=[
    '.',
    'src/',
    godot_headers_path,
    godot_cpp_path + 'include/',
    godot_cpp_path + 'include/core/',
    godot_cpp_path + 'include/gen/'
])

# Add our godot-cpp library
env.Append(LIBPATH=[godot_cpp_path + 'bin/'])
env.Append(LIBS=[godot_cpp_library])

# Add openxr
if openxr_include_path != "":
    env.Append(CPPPATH = [ openxr_include_path ])

if openxr_library_path != "":
    env.Append(LIBPATH = [ openxr_library_path ])

env.Append(LIBS=['openxr_loader'])

sources += Glob('src/*.cpp')
sources += Glob('src/*/*.cpp')
sources += Glob('src/*/*/*.cpp')

library = env.SharedLibrary(target=target_path + env['target_name'], source=sources)
Default(library)

# Generates help for the -h scons option.
Help(opts.GenerateHelpText(env))
