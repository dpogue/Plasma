set(pnUUID_SOURCES
    pnUUID.cpp
    $<$<PLATFORM_ID:Windows>:pnUUID_Win32.cpp>
    $<$<NOT:$<PLATFORM_ID:Windows>>:pnUUID_Unix.cpp>
)

set(pnUUID_HEADERS
    pnUUID.h
)

plasma_library(pnUUID
    FOLDER NucleusLib
    SOURCES ${pnUUID_SOURCES} ${pnUUID_HEADERS}
)
target_link_libraries(
    pnUUID
    PUBLIC
        CoreLib
    PRIVATE
        $<$<AND:$<NOT:$<PLATFORM_ID:Windows>>,$<NOT:$<PLATFORM_ID:Darwin>>>:Uuid::Uuid>
        $<$<PLATFORM_ID:Windows>:rpcrt4>
)

source_group("Source Files" FILES ${pnUUID_SOURCES})
source_group("Header Files" FILES ${pnUUID_HEADERS})
