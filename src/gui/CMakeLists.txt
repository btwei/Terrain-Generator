# src/gui/CMakeLists.txt

file(GLOB GUI_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/src/gui/*.cpp
)

add_executable(terrainGen-gui ${GUI_SOURCES})

# Find packages
find_package(Vulkan REQUIRED)

target_link_libraries(terrainGen-gui PRIVATE
    terrainGenCore
    ${Vulkan_LIBRARIES}
    SDL3::SDL3
    vk-bootstrap::vk-bootstrap
    imgui
)