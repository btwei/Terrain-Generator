# src/cli/CMakeLists.txt

file(GLOB CLI_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/src/cli/*.cpp
)

add_executable(terrainGen-cli ${CLI_SOURCES})
target_link_libraries(terrainGen-cli PRIVATE terrainGenCore)
