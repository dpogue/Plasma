include_directories("../../Apps")
include_directories("../../CoreLib")
include_directories("../../FeatureLib/inc")
include_directories("../../FeatureLib")
include_directories("../../NucleusLib/inc")
include_directories("../../NucleusLib")
include_directories("../../PubUtilLib/inc")
include_directories("../../PubUtilLib")

include_directories(${OPENAL_INCLUDE_DIR})
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${PYTHON_INCLUDE_DIR})
include_directories(${CURL_INCLUDE_DIR})

set(plGLClient_HEADERS
    plGLClient.h
)

set(plGLClient_SOURCES
    plCreatables.cpp
    plGLClient.cpp
)

if(WIN32)
    set(plGLClient_SOURCES ${plGLClient_SOURCES}
        Win32/main.cpp
    )
elseif(APPLE AND ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    include_directories("../../PubUtilLib/plPipeline/GL/OSX")
    set(plGLClient_SOURCES ${plGLClient_SOURCES}
        OSX/main.mm
    )
elseif(UNIX)
    set(plGLClient_SOURCES ${plGLClient_SOURCES}
        X11/main.cpp
    )
endif(WIN32)

add_executable(plGLClient ${plGLClient_SOURCES} ${plGLClient_HEADERS})

# Plasma Libs
target_link_libraries(plGLClient CoreLib)
target_link_libraries(plGLClient plAgeDescription)
target_link_libraries(plGLClient plPipeline)
target_link_libraries(plGLClient plResMgr)
target_link_libraries(plGLClient plScene)
target_link_libraries(plGLClient pfAnimation)

# System Libs
if(APPLE AND ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_link_libraries(plGLClient "-framework Cocoa")
elseif(UNIX)
    target_link_libraries(plGLClient xcb)
    target_link_libraries(plGLClient X11)
endif()

source_group("Source Files" FILES ${plGLClient_SOURCES})
source_group("Header Files" FILES ${plGLClient_HEADERS})
