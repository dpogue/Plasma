set(pfDXPipeline_SOURCES
    hsGDirect3D.cpp
    plDXDevice.cpp
    plDXDeviceRefs.cpp
    plDXEnumerate.cpp
    plDXPipeline.cpp
    plDXPixelShader.cpp
    plDXShader.cpp
    plDXTextFont.cpp
    plDXVertexShader.cpp
)

set(pfDXPipeline_HEADERS
    hsGDirect3D.h
    plDXBufferRefs.h
    plDXDevice.h
    plDXDeviceRef.h
    plDXEnumerate.h
    plDXLightRef.h
    plDXPipeline.h
    plDXPixelShader.h
    plDXRenderTargetRef.h
    plDXSettings.h
    plDXShader.h
    plDXTextFont.h
    plDXTextureRef.h
    plDXVertexShader.h
)

plasma_library(pfDXPipeline
    FOLDER FeatureLib
    SOURCES ${pfDXPipeline_SOURCES} ${pfDXPipeline_HEADERS}
    PRECOMPILED_HEADERS Pch.h
)
plasma_target_simd_sources(pfDXPipeline SSE3 SSE41 plDXPipeline.cpp)
target_link_libraries(pfDXPipeline
    PUBLIC
        CoreLib
        pnNucleusInc
        plDrawable
        plPipeline
        plSurface
    PRIVATE
        pnMessage
        pnNetCommon
        pnSceneObject
        plAvatar
        plGImage
        plGLight
        plMessage
        plMessageBox
        plResMgr
        plScene
        plStatusLog
        pfCamera # plDXPipeline
        ${DirectX_LIBRARIES}
    INTERFACE
        pnFactory
)

source_group("Source Files" FILES ${pfDXPipeline_SOURCES})
source_group("Header Files" FILES ${pfDXPipeline_HEADERS})
