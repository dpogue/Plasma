set(VCPKG_TARGET_ARCHITECTURE arm64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_DISABLE_COMPILER_TRACKING TRUE)

# This is a terrible hack because meson seems to suck.
if(PORT STREQUAL cairo)
    set(VCPKG_LIBRARY_LINKAGE dynamic)
    set(VCPKG_BUILD_TYPE release)
endif()
