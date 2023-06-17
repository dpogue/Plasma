set(testModDLL_SOURCES
    dllmain.cpp
    pmDebugMgr.cpp
)

set(testModDLL_HEADERS
    pmDebugMgr.h
)

plasma_library(testModDLL SHARED SOURCES ${testModDLL_SOURCES} ${testModDLL_HEADERS})
target_link_libraries(
    testModDLL
    PRIVATE
        CoreLib
        pnNucleusInc
)

set_target_properties(testModDLL PROPERTIES PREFIX "")
set_target_properties(testModDLL PROPERTIES OUTPUT_NAME "testMod")
install(TARGETS testModDLL DESTINATION client/ModDLL)

source_group("Source Files" FILES ${testModDLL_SOURCES})
source_group("Header Files" FILES ${testModDLL_HEADERS})
