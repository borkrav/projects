cmake_minimum_required(VERSION 3.22)
set (CMAKE_CXX_STANDARD 23)

# This makefile is only for Windows for now
# MP flag enables multi-process compilation
# Need to disable the -j in the VSCode CMake plugin settings

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP${N}")

project(BR-vk)

include_directories("C:/Users/Boris/dev/VulkanSDK/1.3.204.1/Include")
include_directories("C:/Users/Boris/dev/VulkanSDK/1.3.204.1/Third-Party/Include")
include_directories("C:/Users/Boris/dev/glfw-3.3.6.bin.WIN64/include")

include_directories("vulkan")
include_directories("render")
include_directories("third-party/imgui")

link_directories("C:/Users/Boris/dev/VulkanSDK/1.3.204.1/Lib")
link_directories("C:/Users/Boris/dev/glfw-3.3.6.bin.WIN64/lib-vc2022")

add_subdirectory(shaders)

FILE(GLOB main *.cpp)
FILE(GLOB vulkan vulkan/*.cpp)
FILE(GLOB render render/*.cpp)
FILE(GLOB imgui third-party/imgui/*.cpp)


add_executable(vk-render ${main} ${vulkan} ${render} ${imgui})
target_link_libraries(vk-render vulkan-1.lib glfw3.lib)

target_precompile_headers(vk-render PUBLIC "C:/Users/Boris/dev/VulkanSDK/1.3.204.1/Include/vulkan/vulkan.hpp")
target_precompile_headers(vk-render PUBLIC "C:/Users/Boris/dev/glfw-3.3.6.bin.WIN64/include/GLFW/glfw3.h")
target_precompile_headers(vk-render PUBLIC "C:/Users/Boris/dev/VulkanSDK/1.3.204.1/Include/vulkan/vulkan_handles.hpp")


add_dependencies(vk-render shaders)