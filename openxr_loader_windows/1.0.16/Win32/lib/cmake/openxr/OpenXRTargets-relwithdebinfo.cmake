#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "OpenXR::openxr_loader" for configuration "RelWithDebInfo"
set_property(TARGET OpenXR::openxr_loader APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(OpenXR::openxr_loader PROPERTIES
  IMPORTED_IMPLIB_RELWITHDEBINFO "${_IMPORT_PREFIX}/Win32/lib/openxr_loader.lib"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/Win32/bin/openxr_loader.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS OpenXR::openxr_loader )
list(APPEND _IMPORT_CHECK_FILES_FOR_OpenXR::openxr_loader "${_IMPORT_PREFIX}/Win32/lib/openxr_loader.lib" "${_IMPORT_PREFIX}/Win32/bin/openxr_loader.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
