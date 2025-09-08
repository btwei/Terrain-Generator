cmake_minimum_required(VERSION 3.15)
project(terrainGen LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

set(CMAKE_CXX_STANDARD 20)

# Add SDL as a subdirectory
add_subdirectory(external/SDL EXCLUDE_FROM_ALL)

# Add vk-bootstrap as a subdirectory
add_subdirectory(external/vk-bootstrap)

# Add imgui as a library
# Add Dear ImGUI as a library 
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES})

target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
target_link_libraries(imgui PRIVATE SDL3::SDL3)

# Add Vulkan Memory Allocator as a library
add_library(VulkanMemoryAllocator INTERFACE)
target_include_directories(VulkanMemoryAllocator INTERFACE ${CMAKE_SOURCE_DIR}/external/VulkanMemoryAllocator/include)

add_subdirectory(src/core)
add_subdirectory(src/cli)
add_subdirectory(src/gui)

enable_testing()
add_subdirectory(tests)
