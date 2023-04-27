cmake_minimum_required(VERSION 3.16)
project(lightfields VERSION 0.1.0)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

find_package(Vulkan REQUIRED)
find_package(SDL2 REQUIRED)

# compile imgui as static object library
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

# compile VulkanMemoryAllocator with hpp headers
add_subdirectory(ext/vma)
set_target_properties(VulkanMemoryAllocator PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_subdirectory(src/shaders)

# compile lightfields as executable
add_executable(${PROJECT_NAME})
add_dependencies(${PROJECT_NAME} 
    shaders 
    imgui 
    VulkanMemoryAllocator)
target_precompile_headers(${PROJECT_NAME} 
    PRIVATE src/pch/pch.hpp)
target_sources(${PROJECT_NAME} 
    PRIVATE src/pch/pch.cpp
    PRIVATE src/main.cpp)
target_include_directories(${PROJECT_NAME}
    PRIVATE src
    PUBLIC ext/imgui
    PUBLIC ext/vma/include
    PUBLIC ext/vma/VulkanMemoryAllocator/include
    PUBLIC ${SDL2_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} 
    ${Vulkan_LIBRARIES}
    ${SDL2_LIBRARIES}
    imgui
    VulkanMemoryAllocator)
install(TARGETS ${PROJECT_NAME} DESTINATION bin)