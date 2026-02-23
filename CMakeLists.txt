cmake_minimum_required(VERSION 3.22)
project(vulkan_render)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(Vulkan REQUIRED)

option(KTX_FEATURE_VULKAN "Enable Vulkan support in KTX" ON)
option(KTX_FEATURE_STATIC_LIBRARY "Build KTX as static" ON)

add_subdirectory(external/glm)
add_subdirectory(external/SDL)
add_subdirectory(external/volk)
add_subdirectory(external/tinyobjloader)
add_subdirectory(external/VMA)
add_subdirectory(external/KTX)

add_executable(${PROJECT_NAME}
    source/main.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE
    source)

target_link_libraries(${PROJECT_NAME} PRIVATE
    volk::volk
    SDL3::SDL3-shared
    glm::glm
    tinyobjloader
    GPUOpen::VulkanMemoryAllocator
    ktx)
