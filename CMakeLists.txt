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

# Build HLSL shaders
add_custom_target(shaders)

# gather all HLSL shader files
file(GLOB_RECURSE HLSL_FILES ${PROJECT_SOURCE_DIR}/src/shaders/*.hlsl) # this one is a bit unnecessary
file(GLOB_RECURSE HLSL_VS_FILES ${PROJECT_SOURCE_DIR}/src/shaders/*_vs.hlsl)
file(GLOB_RECURSE HLSL_PS_FILES ${PROJECT_SOURCE_DIR}/src/shaders/*_ps.hlsl)
set_source_files_properties(${HLSL_VS_FILES} PROPERTIES ShaderType "vs") # mark vs as pixel shaders
set_source_files_properties(${HLSL_PS_FILES} PROPERTIES ShaderType "ps") # mark ps as pixel shaders
set_source_files_properties(${HLSL_FILES} PROPERTIES ShaderModel "6_0") # the shader model to be used
set(SHADER_MASTER src/shaders/shaders.hpp)
add_custom_command(TARGET shaders
    COMMAND echo "#pragma once" > ${SHADER_MASTER} # empty out the file contents and start header file
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    VERBATIM)
foreach(FILE ${HLSL_FILES})
    get_filename_component(FILE_WE ${FILE} NAME_WE)
    get_source_file_property(shadertype ${FILE} ShaderType)
    get_source_file_property(shadermodel ${FILE} ShaderModel)
    add_custom_command(TARGET shaders
        # compile hlsl to spir-v
        COMMAND dxc -spirv -T ${shadertype}_${shadermodel} -E main src/shaders/${FILE_WE}.hlsl -Fh ${CMAKE_BINARY_DIR}/${FILE_WE}.hpp -Vn ${FILE_WE}
        # collect all headers in one giant header
        COMMAND echo "#include <${CMAKE_BINARY_DIR}/${FILE_WE}.hpp>" >> ${SHADER_MASTER} 
        MAIN_DEPENDENCY ${CMAKE_BINARY_DIR}/${FILE_WE}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        VERBATIM)
endforeach(FILE)

# compile lightfields as executable
add_executable(${PROJECT_NAME})
add_dependencies(${PROJECT_NAME} shaders)
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