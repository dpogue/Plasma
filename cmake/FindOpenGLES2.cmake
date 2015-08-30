if(OpenGLES2_INCLUDE_DIR AND OpenGLES2_LIBRARY)
    set(OpenGLES2_FIND_QUIETLY TRUE)
endif()


#It would be a good idea to eventually use pkgconfig here.
find_path(OpenGLES2_INCLUDE_DIR GLES2/gl2.h
          /usr/local/include
          /usr/include
)

find_library(OpenGLES2_LIBRARY NAMES glesv2 GLESv2
             PATHS /usr/local/lib /usr/lib
)

set(OpenGLES2_LIBRARIES ${OpenGLES2_LIBRARY})


if(OpenGLES2_INCLUDE_DIR AND OpenGLES2_LIBRARY)
    set(OpenGLES2_FOUND TRUE)
endif()

if (OpenGLES2_FOUND)
    if(NOT OpenGLES2_FIND_QUIETLY)
        message(STATUS "Found OpenGLES2: ${OpenGLES2_INCLUDE_DIR}")
    endif()
else()
    if(OpenGLES2_FIND_REQUIRED)
        message(FATAL_ERROR "Could not find OpenGLES2")
    endif()
endif()

