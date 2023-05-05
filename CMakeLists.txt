cmake_minimum_required(VERSION 3.16)
project(light-field-disparity VERSION 0.1.0)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_CXX_STANDARD 17)

# let cmake find vulkan
find_package(Vulkan REQUIRED)

# compile sdl as shared lib
set(SDL_SHARED ON CACHE BOOL "Build a shared version of the library")
set(SDL_STATIC OFF CACHE BOOL "Build a static version of the library")
set(SDL_TEST OFF CACHE BOOL "Build the SDL3_test library")
set(SDL3_INCLUDE_DIRS ext/sdl/include)
add_subdirectory(ext/sdl)

# compile imgui as shared lib
add_library(imgui STATIC
    ext/imgui/imgui.cpp
    ext/imgui/imgui_draw.cpp
    ext/imgui/imgui_tables.cpp
    ext/imgui/imgui_widgets.cpp
    ext/imgui/backends/imgui_impl_sdl3.cpp
    ext/imgui/backends/imgui_impl_vulkan.cpp)
target_include_directories(imgui
    PUBLIC ext/imgui
    PUBLIC ext/sdl/include) # TODO: make private?
install(TARGETS imgui DESTINATION ${CMAKE_INSTALL_LIBDIR})


# compile shaders into spir-v bytecode headers
add_subdirectory(include/shaders)

# TODO: REMOVE SDL2

# compile as executable
add_executable(${PROJECT_NAME})
add_dependencies(${PROJECT_NAME} 
    shaders
    imgui)
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
    PRIVATE ext/vma/VulkanMemoryAllocator/include)
target_link_libraries(${PROJECT_NAME} 
    ${Vulkan_LIBRARIES}
    SDL3-shared
    imgui)
install(TARGETS ${PROJECT_NAME} DESTINATION bin)