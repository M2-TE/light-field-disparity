cmake_minimum_required(VERSION 3.16)
project(lightfields VERSION 0.1.0)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

find_package(Vulkan REQUIRED)
find_package(SDL2 REQUIRED)

# compile imgui as shared object library
add_library(imgui SHARED
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

# compile lightfields as executable
add_executable(lightfields)
target_precompile_headers(lightfields 
    PRIVATE src/pch/pch.hpp)
target_sources(lightfields 
    PRIVATE src/pch/pch.cpp
    PRIVATE src/main.cpp)
target_include_directories(lightfields
    PUBLIC src
    PUBLIC ext
    PUBLIC ${SDL2_INCLUDE_DIRS})
target_link_libraries(lightfields 
    ${Vulkan_LIBRARIES}
    ${SDL2_LIBRARIES}
    imgui)
install(TARGETS lightfields DESTINATION bin)