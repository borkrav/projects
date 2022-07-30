cmake_minimum_required(VERSION 3.22)
set (CMAKE_CXX_STANDARD 20)

project(BR-vk)

include_directories("C:/Users/Boris/dev/VulkanSDK/1.3.204.1/Include")
include_directories("C:/Users/Boris/dev/VulkanSDK/1.3.204.1/Third-Party/Include")
include_directories("C:/Users/Boris/dev/glfw-3.3.6.bin.WIN64/include")

include_directories("vulkan")
include_directories("render")

link_directories("C:/Users/Boris/dev/VulkanSDK/1.3.204.1/Lib")
link_directories("C:/Users/Boris/dev/glfw-3.3.6.bin.WIN64/lib-vc2022")

add_subdirectory(shaders)
add_subdirectory(vulkan)
add_subdirectory(render)

FILE(GLOB sources *.cpp)

add_executable(vk-render ${sources})
target_link_libraries(vk-render vulkan-1.lib glfw3.lib vulkan render)
add_dependencies(vk-render shaders)