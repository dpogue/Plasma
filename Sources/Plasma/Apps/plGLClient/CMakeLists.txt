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
)

set(plGLClient_SOURCES
    main.cpp
)

add_executable(plGLClient ${plGLClient_SOURCES} ${plGLClient_HEADERS})

# Plasma Libs
target_link_libraries(plGLClient CoreLib)
target_link_libraries(plGLClient pnNetCommon)
target_link_libraries(plGLClient plDrawable)
target_link_libraries(plGLClient plGLight)
target_link_libraries(plGLClient plPipeline)

# System Libs
target_link_libraries(plGLClient xcb)
target_link_libraries(plGLClient X11)
target_link_libraries(plGLClient X11-xcb)
#target_link_libraries(plGLClient ${JPEG_LIBRARY})
#target_link_libraries(plGLClient ${PNG_LIBRARY})

install(
    TARGETS plGLClient
    DESTINATION client
)

source_group("Source Files" FILES ${plGLClient_SOURCES})
source_group("Header Files" FILES ${plGLClient_HEADERS})
