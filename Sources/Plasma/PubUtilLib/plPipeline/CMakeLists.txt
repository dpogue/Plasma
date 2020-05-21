set(plPipeline_SOURCES
    hsG3DDeviceSelector.cpp
    pl3DPipeline.cpp
    plCaptureRender.cpp
    plCubicRenderTargetModifier.cpp
    plCullTree.cpp
    plDebugText.cpp
    plDTProgressMgr.cpp
    plDynamicEnvMap.cpp
    plFogEnvironment.cpp
    plPipelineViewSettings.cpp
    plPlateProgressMgr.cpp
    plPlates.cpp
    plRenderTarget.cpp
    plStatusLogDrawer.cpp
    plTextFont.cpp
    plTextGenerator.cpp
    plTransitionMgr.cpp
)

set(plDXPipeline_SOURCES
    DX/hsGDirect3D.cpp
    DX/plDXDevice.cpp
    DX/plDXDeviceRefs.cpp
    DX/plDXEnumerate.cpp
    DX/plDXPipeline.cpp
    DX/plDXPixelShader.cpp
    DX/plDXShader.cpp
    DX/plDXTextFont.cpp
    DX/plDXVertexShader.cpp
)

set(plGLPipeline_SOURCES
    GL/plGLDevice.cpp
    GL/plGLDeviceRefs.cpp
    GL/plGLMaterialShaderRef.cpp
    GL/plGLPipeline.cpp
    GL/plGLPlateManager.cpp
    GL/plShaderNode.cpp
)

set(plPipeline_HEADERS
    hsFogControl.h
    hsG3DDeviceSelector.h
    hsWinRef.h
    pl3DPipeline.h
    plCaptureRender.h
    plCubicRenderTarget.h
    plCubicRenderTargetModifier.h
    plCuller.h
    plCullTree.h
    plDebugText.h
    plDrawPrim.h
    plDTProgressMgr.h
    plDynamicEnvMap.h
    plFogEnvironment.h
    plPipelineCreatable.h
    plPipelineCreate.h
    plPipelineViewSettings.h
    plPlateProgressMgr.h
    plPlates.h
    plRenderTarget.h
    plStatusLogDrawer.h
    plStencil.h
    plTextFont.h
    plTextGenerator.h
    plTransitionMgr.h
)

set(plDXPipeline_HEADERS
    DX/hsGDirect3D.h
    DX/plDXBufferRefs.h
    DX/plDXDevice.h
    DX/plDXDeviceRef.h
    DX/plDXEnumerate.h
    DX/plDXLightRef.h
    DX/plDXPipeline.h
    DX/plDXPixelShader.h
    DX/plDXRenderTargetRef.h
    DX/plDXSettings.h
    DX/plDXShader.h
    DX/plDXTextFont.h
    DX/plDXTextureRef.h
    DX/plDXVertexShader.h
)

set(plGLPipeline_HEADERS
    GL/plGLConstants.h
    GL/plGLDevice.h
    GL/plGLDeviceRef.h
    GL/plGLMaterialShaderRef.h
    GL/plGLPipeline.h
    GL/plGLPlateManager.h
    GL/plShaderNode.h
)


if(APPLE AND ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(plGLPipeline_SOURCES ${plGLPipeline_SOURCES}
                                GL/OSX/egl_api.c
                                GL/OSX/eglCocoa.mm
        )
endif(APPLE AND ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")


if(PLASMA_PIPELINE STREQUAL "DirectX")
    add_library(plPipeline STATIC ${plPipeline_SOURCES} ${plDXPipeline_SOURCES} ${plPipeline_HEADERS} ${plDXPipeline_HEADERS})

    target_link_libraries(plPipeline PUBLIC ${DirectX_LIBRARIES})
endif(PLASMA_PIPELINE STREQUAL "DirectX")

if(PLASMA_PIPELINE STREQUAL "OpenGL")
    add_library(plPipeline STATIC ${plPipeline_SOURCES} ${plGLPipeline_SOURCES} ${plPipeline_HEADERS} ${plGLPipeline_HEADERS})

    if(APPLE AND ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        target_include_directories(plPipeline PRIVATE "GL/OSX")
        target_link_libraries(plPipeline PUBLIC "-framework OpenGL")
    else(APPLE AND ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        target_include_directories(plPipeline PRIVATE ${EGL_INCLUDE_DIRS} ${OpenGL4_INCLUDE_DIRS})
        target_link_libraries(plPipeline PUBLIC ${OpenGL4_LIBRARIES} ${EGL_LIBRARIES})
    endif(APPLE AND ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
endif(PLASMA_PIPELINE STREQUAL "OpenGL")

target_include_directories(plPipeline PRIVATE "${PLASMA_SOURCE_ROOT}/FeatureLib")
target_link_libraries(plPipeline
    PUBLIC
        CoreLib
        pnKeyedObject
        #pnNucleusInc
        #pnSceneObject
        plClientResMgr
        plDrawable
        plGLight
        #plScene
        #plSurface
)

source_group("Source Files" FILES ${plPipeline_SOURCES})
source_group("Header Files" FILES ${plPipeline_HEADERS})

if(PLASMA_PIPELINE STREQUAL "DirectX")
    source_group("DX\\Source Files" FILES ${plDXPipeline_SOURCES})
    source_group("DX\\Header Files" FILES ${plDXPipeline_HEADERS})
endif(PLASMA_PIPELINE STREQUAL "DirectX")

if(PLASMA_PIPELINE STREQUAL "OpenGL")
    source_group("GL\\Source Files" FILES ${plGLPipeline_SOURCES})
    source_group("GL\\Header Files" FILES ${plGLPipeline_HEADERS})
endif(PLASMA_PIPELINE STREQUAL "OpenGL")
