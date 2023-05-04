cmake_minimum_required(VERSION 3.16)
project(light-field-disparity VERSION 0.1.0)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_CXX_STANDARD 17)

find_package(Vulkan REQUIRED)
find_package(SDL2 REQUIRED)

# compile imgui as shared object library
add_library(imgui STATIC
    ext/imgui/imgui.cpp
    ext/imgui/imgui_draw.cpp
    ext/imgui/imgui_tables.cpp
    ext/imgui/imgui_widgets.cpp
    ext/imgui/backends/imgui_impl_sdl2.cpp
    ext/imgui/backends/imgui_impl_vulkan.cpp)
target_include_directories(imgui
    PUBLIC ext/imgui
    PUBLIC ${SDL2_INCLUDE_DIRS})
install(TARGETS imgui DESTINATION lib)

# compile shaders into spir-v bytecode headers
add_subdirectory(include/shaders)

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
    PRIVATE ext/vma/include
    PRIVATE ext/vma/VulkanMemoryAllocator/include
    PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} 
    ${Vulkan_LIBRARIES}
    ${SDL2_LIBRARIES}
    imgui)
install(TARGETS ${PROJECT_NAME} DESTINATION bin)