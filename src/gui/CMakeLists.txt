# src/gui/CMakeLists.txt

# Compile shaders
set(SHADER_DIR "${CMAKE_SOURCE_DIR}/shaders")
set(SPIRV_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders_spv")

file(MAKE_DIRECTORY ${SPIRV_DIR})

set(SHADER_FILES
    "default.slang|vertex:mainVert,fragment:mainFrag"
)

set(SPIRV_FILES "")

foreach(SHADER_META ${SHADER_FILES})
    string(REGEX MATCHALL "[^|]+" SHADER_PARTS "${SHADER_META}")
    list(GET SHADER_PARTS 0 SHADER_FILE)
    list(GET SHADER_PARTS 1 COMBINED_ENTRY_POINTS)

    string(REGEX MATCHALL "[^,]+" ENTRY_POINTS "${COMBINED_ENTRY_POINTS}")

    set(ENTRY_ARGS "")
    foreach(ENTRY ${ENTRY_POINTS})
        string(REGEX MATCHALL "[^:]+" ENTRY_SPLIT ${ENTRY})
        list(GET ENTRY_SPLIT 0 STAGE)
        list(GET ENTRY_SPLIT 1 ENTRY_NAME)

        list(APPEND ENTRY_ARGS -entry ${ENTRY_NAME} -stage ${STAGE})
    endforeach()

    get_filename_component(BASE_NAME ${SHADER_FILE} NAME_WE)
    set(SPIRV_FILE "${SPIRV_DIR}/${BASE_NAME}.spv")

    add_custom_command(
        OUTPUT ${SPIRV_FILE}
        COMMAND slangc ${SHADER_DIR}/${SHADER_FILE} -target spirv -o ${SPIRV_FILE} ${ENTRY_ARGS}
        DEPENDS ${SHADER_DIR}/${SHADER_FILE}
        COMMENT "Compiling ${BASE_NAME} stage ${STAGE} entry ${ENTRY_NAME}"
        VERBATIM
    )
    list(APPEND SPIRV_FILES ${SPIRV_FILE})
    
endforeach()

add_custom_target(Shaders ALL DEPENDS ${SPIRV_FILES})

# Compile executable
file(GLOB GUI_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/src/gui/*.cpp
)

add_executable(terrainGen-gui ${GUI_SOURCES})
add_dependencies(terrainGen-gui Shaders)

# Find packages
find_package(Vulkan REQUIRED)

target_link_libraries(terrainGen-gui PRIVATE
    terrainGenCore
    ${Vulkan_LIBRARIES}
    SDL3::SDL3
    vk-bootstrap::vk-bootstrap
    imgui
    VulkanMemoryAllocator
)