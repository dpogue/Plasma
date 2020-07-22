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

if(MOVIE_AVAILABLE)
    include_directories(${VPX_INCLUDE_DIR})
    include_directories(${Opus_INCLUDE_DIR})
endif()

# Test for Python Interpreter, which will be used for extra build scripts if available
find_package(PythonInterp)
set_package_properties(PythonInterp PROPERTIES
    URL "http://www.python.org"
    DESCRIPTION "Python Scripting language interpreter"
    TYPE OPTIONAL
    PURPOSE "Used to (re-)build some client resource files"
)

if(PYTHONINTERP_FOUND)
    include(FindPythonModule)

    # Test for Python modules needed to build resource.dat
    find_python_module(cairosvg)
    find_python_module(PIL)

    if((DEFINED PY_CAIROSVG) AND (DEFINED PY_PIL))
        set(CAN_BUILD_RESOURCE_DAT ON)
    endif((DEFINED PY_CAIROSVG) AND (DEFINED PY_PIL))
endif(PYTHONINTERP_FOUND)

set(plGLClient_HEADERS
    plClientLoader.h
    plGLClient.h
)

set(plGLClient_SOURCES
    plClientLoader.cpp
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

#if(CAN_BUILD_RESOURCE_DAT AND PLASMA_BUILD_RESOURCE_DAT)
#    add_subdirectory(external)
#    add_dependencies(plGLClient externalResources)
#endif()

target_link_libraries(plGLClient string_theory)
# Plasma Libs
target_link_libraries(plGLClient CoreLib)
target_link_libraries(plGLClient pnDispatch)
target_link_libraries(plGLClient pnEncryption)
target_link_libraries(plGLClient pnFactory)
target_link_libraries(plGLClient pnInputCore)
target_link_libraries(plGLClient pnKeyedObject)
target_link_libraries(plGLClient pnMessage)
target_link_libraries(plGLClient pnModifier)
target_link_libraries(plGLClient pnNetBase)
target_link_libraries(plGLClient pnNetCommon)
target_link_libraries(plGLClient pnNucleusInc)
target_link_libraries(plGLClient pnSceneObject)
target_link_libraries(plGLClient pnTimer)
target_link_libraries(plGLClient pnUUID)
target_link_libraries(plGLClient plAgeDescription)
target_link_libraries(plGLClient plAgeLoader)
target_link_libraries(plGLClient plAnimation)
target_link_libraries(plGLClient plAudible)
target_link_libraries(plGLClient plAudio)
target_link_libraries(plGLClient plAudioCore)
target_link_libraries(plGLClient plClientResMgr)
target_link_libraries(plGLClient plCompression)
target_link_libraries(plGLClient plContainer)
target_link_libraries(plGLClient plDrawable)
target_link_libraries(plGLClient plFile)
target_link_libraries(plGLClient plGImage)
target_link_libraries(plGLClient plGLight)
target_link_libraries(plGLClient plInputCore)
target_link_libraries(plGLClient plInterp)
target_link_libraries(plGLClient plIntersect)
target_link_libraries(plGLClient plMath)
target_link_libraries(plGLClient plMessage)
target_link_libraries(plGLClient plModifier)
target_link_libraries(plGLClient plNetClient)
target_link_libraries(plGLClient plNetTransport)
target_link_libraries(plGLClient plParticleSystem)
target_link_libraries(plGLClient plPipeline)
target_link_libraries(plGLClient plProgressMgr)
target_link_libraries(plGLClient plResMgr)
target_link_libraries(plGLClient plScene)
target_link_libraries(plGLClient plSDL)
target_link_libraries(plGLClient plStatGather)
target_link_libraries(plGLClient plStatusLog)
target_link_libraries(plGLClient plSurface)
target_link_libraries(plGLClient plTransform)
target_link_libraries(plGLClient plUnifiedTime)
target_link_libraries(plGLClient pfAnimation)
target_link_libraries(plGLClient pfAudio)
target_link_libraries(plGLClient pfCamera)
target_link_libraries(plGLClient pfConsoleCore)
target_link_libraries(plGLClient pfConsole)
target_link_libraries(plGLClient pfGameGUIMgr)
target_link_libraries(plGLClient pfLocalizationMgr)
target_link_libraries(plGLClient pfMessage)
target_link_libraries(plGLClient pfMoviePlayer)
target_link_libraries(plGLClient pfPasswordStore)

# System Libs
if(APPLE AND ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_link_libraries(plGLClient "-framework Cocoa")
elseif(UNIX)
    target_link_libraries(plGLClient xcb)
    target_link_libraries(plGLClient xcb-xfixes)
    target_link_libraries(plGLClient X11)
endif()

if(PLASMA_USE_OPUS)
    target_link_libraries(plGLClient ${Opus_LIBRARIES})
endif()
if(PLASMA_USE_SPEEX)
    target_link_libraries(plGLClient ${Speex_LIBRARY})
endif()
if(PLASMA_USE_VPX)
    target_link_libraries(plGLClient ${VPX_LIBRARY})
endif()

source_group("Source Files" FILES ${plGLClient_SOURCES})
source_group("Header Files" FILES ${plGLClient_HEADERS})
