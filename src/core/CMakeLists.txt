# src/core/CMakeLists.txt

file(GLOB CORE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/src/core/*.cpp
)

add_library(terrainGenCore ${CORE_SOURCES})

target_include_directories(terrainGenCore PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(terrainGenCore PRIVATE
    glm
)