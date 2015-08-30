if(EGL_INCLUDE_DIR AND EGL_LIBRARY)
    set(EGL_FIND_QUIETLY TRUE)
endif()


#It would be a good idea to eventually use pkgconfig here.
find_path(EGL_INCLUDE_DIR EGL/egl.h
          /usr/local/include
          /usr/include
)

find_library(EGL_LIBRARY NAMES egl EGL
             PATHS /usr/local/lib /usr/lib
)

set(EGL_LIBRARIES ${EGL_LIBRARY})


if(EGL_INCLUDE_DIR AND EGL_LIBRARY)
    set(EGL_FOUND TRUE)
endif()

if (EGL_FOUND)
    if(NOT EGL_FIND_QUIETLY)
        message(STATUS "Found EGL: ${EGL_INCLUDE_DIR}")
    endif()
else()
    if(EGL_FIND_REQUIRED)
        message(FATAL_ERROR "Could not find EGL")
    endif()
endif()
