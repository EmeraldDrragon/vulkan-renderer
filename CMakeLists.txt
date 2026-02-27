cmake_minimum_required(VERSION 3.22)
project(vulkan_render)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(NOT DEFINED VULKAN_SDK_PATH)
    set(VULKAN_SDK_PATH $ENV{VULKAN_SDK})
endif()

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
    source
    SYSTEM ${VULKAN_SDK_PATH}/include)

target_link_libraries(${PROJECT_NAME} PRIVATE
    volk::volk
    SDL3::SDL3-shared
    glm::glm
    tinyobjloader
    GPUOpen::VulkanMemoryAllocator
    ktx
    ${VULKAN_SDK_PATH}/lib/libslang.so)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_directory
${CMAKE_CURRENT_SOURCE_DIR}/source/assets
${CMAKE_CURRENT_BINARY_DIR}/bin/assets)
