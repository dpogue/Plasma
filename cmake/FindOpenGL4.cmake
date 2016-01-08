if(OpenGL4_INCLUDE_DIR AND OpenGL4_LIBRARY)
    set(OpenGL4_FIND_QUIETLY TRUE)
endif()


#It would be a good idea to eventually use pkgconfig here.
find_path(OpenGL4_INCLUDE_DIR GL/gl.h
          /usr/local/include
          /usr/include
)

find_library(OpenGL4_LIBRARY NAMES OpenGL
             PATHS /usr/local/lib /usr/lib
)

set(OpenGL4_LIBRARIES ${OpenGL4_LIBRARY})


if(OpenGL4_INCLUDE_DIR AND OpenGL4_LIBRARY)
    set(OpenGL4_FOUND TRUE)
endif()

if (OpenGL4_FOUND)
    if(NOT OpenGL4_FIND_QUIETLY)
        message(STATUS "Found OpenGL: ${OpenGL4_INCLUDE_DIR}")
    endif()
else()
    if(OpenGL4_FIND_REQUIRED)
        message(FATAL_ERROR "Could not find OpenGL")
    endif()
endif()

