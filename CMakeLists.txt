cmake_minimum_required(VERSION 3.16)
project(fractal-viewer)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

add_subdirectory(SDL EXCLUDE_FROM_ALL)

add_executable(fractal-viewer WIN32 main.c)

# Compile GLSL shaders to SPIR-V
set(SHADER_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(SHADERS
    ${SHADER_DIR}/vertex.glsl
    ${SHADER_DIR}/fragment.glsl
)

add_custom_target(compile_shaders ALL
    COMMAND glslc -fshader-stage=vertex ${SHADER_DIR}/vertex.glsl -o ${SHADER_DIR}/vertex.spv
    COMMAND glslc -fshader-stage=fragment ${SHADER_DIR}/fragment.glsl -o ${SHADER_DIR}/fragment.spv
    DEPENDS ${SHADERS}
    COMMENT "Compiling GLSL shaders to SPIR-V"
)

# Copy shaders to the executable output directory
add_custom_command(TARGET fractal-viewer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders
    $<TARGET_FILE_DIR:fractal-viewer>/shaders
)

target_link_libraries(fractal-viewer PRIVATE SDL3::SDL3)