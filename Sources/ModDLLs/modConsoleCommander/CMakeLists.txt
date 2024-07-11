set(modConsoleCommander_SOURCES
    dllmain.cpp
    pmConsoleCommander.cpp
)

set(modConsoleCommander_HEADERS
    pmConsoleCommander.h
    crow_all.h
)

plasma_library(modConsoleCommander SHARED SOURCES ${modConsoleCommander_SOURCES} ${modConsoleCommander_HEADERS})
target_link_libraries(
    modConsoleCommander
    PRIVATE
        CoreLib
        pnNucleusInc
)

set_target_properties(modConsoleCommander PROPERTIES PREFIX "")
set_target_properties(modConsoleCommander PROPERTIES OUTPUT_NAME "ConsoleCommander")
install(TARGETS modConsoleCommander DESTINATION client/ModDLL)

source_group("Source Files" FILES ${modConsoleCommander_SOURCES})
source_group("Header Files" FILES ${modConsoleCommander_HEADERS})
