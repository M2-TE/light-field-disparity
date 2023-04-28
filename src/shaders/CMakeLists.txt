# provide target for building shaders
add_custom_target(shaders)

# gather all HLSL shader files
file(GLOB_RECURSE HLSL_FILES ${PROJECT_SOURCE_DIR}/src/shaders/*.hlsl) # purely for convenience
file(GLOB_RECURSE HLSL_VS_FILES ${PROJECT_SOURCE_DIR}/src/shaders/*_vs.hlsl)
file(GLOB_RECURSE HLSL_PS_FILES ${PROJECT_SOURCE_DIR}/src/shaders/*_ps.hlsl)
file(GLOB_RECURSE HLSL_CS_FILES ${PROJECT_SOURCE_DIR}/src/shaders/*_cs.hlsl)

# set properties on the shaders according to their names
set_source_files_properties(${HLSL_VS_FILES} PROPERTIES ShaderType "vs") # mark vs as vertex shaders
set_source_files_properties(${HLSL_PS_FILES} PROPERTIES ShaderType "ps") # mark ps as pixel shaders
set_source_files_properties(${HLSL_CS_FILES} PROPERTIES ShaderType "cs") # mark ps as compute shaders
set_source_files_properties(${HLSL_FILES} PROPERTIES ShaderModel "6_0") # the shader model to be used

# create and initialize main shader header
add_custom_command(TARGET shaders
    COMMAND cp shaders_template.hpp shaders.hpp
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    VERBATIM)

# compile shaders and add them to the main header
foreach(FILE ${HLSL_FILES})
    get_filename_component(FILE_WE ${FILE} NAME_WE)
    get_source_file_property(shadertype ${FILE} ShaderType)
    get_source_file_property(shadermodel ${FILE} ShaderModel)
    add_custom_command(TARGET shaders
        # compile hlsl to spir-v
        COMMAND dxc -spirv -T ${shadertype}_${shadermodel} -E main ${FILE_WE}.hlsl -Fh ${CMAKE_BINARY_DIR}/${FILE_WE}.hpp -Vn ${FILE_WE}
        # include in main shader header
        COMMAND echo "#include <${CMAKE_BINARY_DIR}/${FILE_WE}.hpp>" >> shaders.hpp
        MAIN_DEPENDENCY ${CMAKE_BINARY_DIR}/${FILE_WE}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        VERBATIM)
endforeach(FILE)