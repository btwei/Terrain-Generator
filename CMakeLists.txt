cmake_minimum_required(VERSION 3.15)
project(terrainGen LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

# Add SDL as a subdirectory
add_subdirectory(external/SDL EXCLUDE_FROM_ALL)

add_subdirectory(src/core)
add_subdirectory(src/cli)
add_subdirectory(src/gui)

enable_testing()
add_subdirectory(tests)
