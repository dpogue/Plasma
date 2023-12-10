vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO dpogue/PhysX
    REF be9de0ed4fa1f6210586d6f517d29e95081f6fec
    SHA512 b5999832e6b05706d8ccbbf1da66a3f62d8da4c20af1810aed49aa16bc8e69a860a775496897346c865eba8a6bc58df577beb0a86560c4caa2de69757d1402e2
    HEAD_REF master
)

if(NOT DEFINED RELEASE_CONFIGURATION)
    set(RELEASE_CONFIGURATION "release")
endif()
set(DEBUG_CONFIGURATION "debug")

set(OPTIONS
    "-DPHYSX_ROOT_DIR=${SOURCE_PATH}/physx"
    "-DPXSHARED_PATH=${SOURCE_PATH}/pxshared"
    "-DPXSHARED_INSTALL_PREFIX=${CURRENT_PACKAGES_DIR}"
    "-DCMAKEMODULES_PATH=${SOURCE_PATH}/externals/cmakemodules"
    "-DCMAKEMODULES_NAME=CMakeModules"
    "-DCMAKE_MODULES_VERSION=1.27"
    "-DPX_BUILDSNIPPETS=OFF"
    "-DPX_BUILDPUBLICSAMPLES=OFF"
    "-DPX_FLOAT_POINT_PRECISE_MATH=OFF"
    "-DPX_COPY_EXTERNAL_DLL=OFF"
    "-DGPU_DLL_COPIED=ON"
)

set(OPTIONS_RELEASE
    "-DPX_OUTPUT_BIN_DIR=${CURRENT_PACKAGES_DIR}"
    "-DPX_OUTPUT_LIB_DIR=${CURRENT_PACKAGES_DIR}"
)
set(OPTIONS_DEBUG
    "-DPX_OUTPUT_BIN_DIR=${CURRENT_PACKAGES_DIR}/debug"
    "-DPX_OUTPUT_LIB_DIR=${CURRENT_PACKAGES_DIR}/debug"
    "-DNV_USE_DEBUG_WINCRT=ON"
)

if(VCPKG_TARGET_IS_UWP)
    list(APPEND OPTIONS "-DTARGET_BUILD_PLATFORM=uwp")
    set(configure_options WINDOWS_USE_MSBUILD)
elseif(VCPKG_TARGET_IS_WINDOWS)
    list(APPEND OPTIONS "-DTARGET_BUILD_PLATFORM=windows")
elseif(VCPKG_TARGET_IS_OSX)
    list(APPEND OPTIONS "-DTARGET_BUILD_PLATFORM=mac")
elseif(VCPKG_TARGET_IS_LINUX OR VCPKG_TARGET_IS_FREEBSD)
    list(APPEND OPTIONS "-DTARGET_BUILD_PLATFORM=linux")
elseif(VCPKG_TARGET_IS_ANDROID)
    list(APPEND OPTIONS "-DTARGET_BUILD_PLATFORM=android")
else()
    message(FATAL_ERROR "Unhandled or unsupported target platform.")
endif()

if(VCPKG_TARGET_IS_OSX OR VCPKG_TARGET_IS_IOS)
    list(APPEND OPTIONS "-DNV_FORCE_64BIT_SUFFIX=ON" "-DNV_FORCE_32BIT_SUFFIX=OFF")
endif()

if(VCPKG_LIBRARY_LINKAGE STREQUAL "dynamic")
    list(APPEND OPTIONS "-DPX_GENERATE_STATIC_LIBRARIES=OFF")
else()
    list(APPEND OPTIONS "-DPX_GENERATE_STATIC_LIBRARIES=ON")
endif()

if(VCPKG_CRT_LINKAGE STREQUAL "dynamic")
    list(APPEND OPTIONS "-DNV_USE_STATIC_WINCRT=OFF")
else()
    list(APPEND OPTIONS "-DNV_USE_STATIC_WINCRT=ON")
endif()

if(VCPKG_TARGET_ARCHITECTURE STREQUAL "arm" OR VCPKG_TARGET_ARCHITECTURE STREQUAL "arm64")
    list(APPEND OPTIONS "-DPX_OUTPUT_ARCH=arm")
else()
    list(APPEND OPTIONS "-DPX_OUTPUT_ARCH=x86")
endif()

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}/physx/compiler/public"
    ${configure_options}
    DISABLE_PARALLEL_CONFIGURE
    OPTIONS ${OPTIONS}
    OPTIONS_DEBUG ${OPTIONS_DEBUG}
    OPTIONS_RELEASE ${OPTIONS_RELEASE}
)
vcpkg_cmake_install()

# NVIDIA Gameworks release structure is generally something like <compiler>/<configuration>/[artifact]
# It would be nice to patch this out, but that directory structure is hardcoded over many cmake files.
# So, we have this helpful helper to copy the bins and libs out.
function(fixup_physx_artifacts)
    macro(_fixup _IN_DIRECTORY _OUT_DIRECTORY)
        foreach(_SUFFIX IN LISTS _fpa_SUFFIXES)
            file(GLOB_RECURSE _ARTIFACTS
                LIST_DIRECTORIES false
                "${CURRENT_PACKAGES_DIR}/${_IN_DIRECTORY}/*${_SUFFIX}"
            )
            if(_ARTIFACTS)
                file(COPY ${_ARTIFACTS} DESTINATION "${CURRENT_PACKAGES_DIR}/${_OUT_DIRECTORY}")
            endif()
        endforeach()
    endmacro()

    cmake_parse_arguments(_fpa "" "DIRECTORY" "SUFFIXES" ${ARGN})
    _fixup("bin" ${_fpa_DIRECTORY})
    _fixup("debug/bin" "debug/${_fpa_DIRECTORY}")
endfunction()

fixup_physx_artifacts(
    DIRECTORY "lib"
    SUFFIXES ${VCPKG_TARGET_STATIC_LIBRARY_SUFFIX} ${VCPKG_TARGET_IMPORT_LIBRARY_SUFFIX}
)
fixup_physx_artifacts(
    DIRECTORY "bin"
    SUFFIXES ${VCPKG_TARGET_SHARED_LIBRARY_SUFFIX} ".pdb"
)

# Remove compiler directory and descendents.
if(VCPKG_LIBRARY_LINKAGE STREQUAL "static")
    file(REMOVE_RECURSE
        "${CURRENT_PACKAGES_DIR}/bin/"
        "${CURRENT_PACKAGES_DIR}/debug/bin/"
    )
else()
    file(GLOB PHYSX_ARTIFACTS LIST_DIRECTORIES true
        "${CURRENT_PACKAGES_DIR}/bin/*"
        "${CURRENT_PACKAGES_DIR}/debug/bin/*"
    )
    foreach(_ARTIFACT IN LISTS PHYSX_ARTIFACTS)
        if(IS_DIRECTORY ${_ARTIFACT})
            file(REMOVE_RECURSE ${_ARTIFACT})
        endif()
    endforeach()
endif()

file(REMOVE_RECURSE
    "${CURRENT_PACKAGES_DIR}/debug/include"
    "${CURRENT_PACKAGES_DIR}/debug/source"
    "${CURRENT_PACKAGES_DIR}/source"
)
file(INSTALL "${SOURCE_PATH}/LICENSE.md" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)

