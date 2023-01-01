plasma_executable(plIniTest TOOL SOURCES main.cpp)
target_link_libraries(
    plIniTest
    PRIVATE
        CoreLib
        plContainer
)

