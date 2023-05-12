cmake_minimum_required(VERSION 3.16)
project(light-field-disparity)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_CXX_STANDARD 17 CACHE STRING "Set C++ Standard")

# vulkan libraries and dxc
find_package(Vulkan REQUIRED COMPONENTS dxc)
# headers located at ext/vulkan-headers

# sdl
set(SDL_SHARED ON CACHE BOOL "Build a shared version of the library")
set(SDL_STATIC OFF CACHE BOOL "Build a static version of the library")
set(SDL_TEST OFF CACHE BOOL "Build the SDL3_test library")
set(SDL3_INCLUDE_DIRS ext/sdl/include)
add_subdirectory(ext/sdl)

# vma
set(VMA_STATIC_VULKAN_FUNCTIONS OFF CACHE BOOL "Enable static vulkan functions for VMA")
set(VMA_DYNAMIC_VULKAN_FUNCTIONS ON CACHE BOOL "Enable dynamic vulkan functions for VMA")
add_subdirectory(ext/vma)
set_target_properties(VulkanMemoryAllocator PROPERTIES CXX_STANDARD 17)
target_include_directories(VulkanMemoryAllocator
    PRIVATE ext/vulkan-headers/include)

# imgui
add_library(imgui STATIC
    ext/imgui/imgui.cpp
    ext/imgui/imgui_draw.cpp
    ext/imgui/imgui_tables.cpp
    ext/imgui/imgui_widgets.cpp
    ext/imgui/backends/imgui_impl_sdl3.cpp
    ext/imgui/backends/imgui_impl_vulkan.cpp)
target_include_directories(imgui
    PRIVATE ext/imgui
    PRIVATE ext/sdl/include
    PRIVATE ${Vulkan_INCLUDE_DIRS})

# compile shaders into spir-v bytecode headers
add_subdirectory(include/shaders)

# compile as executable
add_executable(${PROJECT_NAME})
target_precompile_headers(${PROJECT_NAME} 
    PRIVATE include/pch.hpp)
target_sources(${PROJECT_NAME}
    PRIVATE src/pch.cpp
    PRIVATE src/main.cpp
    PRIVATE src/disparity_compute.cpp
    PRIVATE src/swapchain_write.cpp)
target_include_directories(${PROJECT_NAME}
    PRIVATE include
    PRIVATE ext
    PRIVATE ext/imgui
    PRIVATE ext/sdl/include
    PRIVATE ext/vma/include
    PRIVATE ext/vulkan-headers/include)
add_dependencies(${PROJECT_NAME}
    shaders)
target_link_libraries(${PROJECT_NAME} 
    ${Vulkan_LIBRARIES}
    VulkanMemoryAllocator
    SDL3-shared
    imgui)