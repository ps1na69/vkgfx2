file(GLOB_RECURSE ENGINE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

file(GLOB_RECURSE ENGINE_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
)

find_program(VKGFX2_DXC_EXECUTABLE NAMES dxc REQUIRED)

set(VKGFX2_SHADER_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/shaders" CACHE INTERNAL
    "Directory containing compiled vkgfx2 shaders")
set(VKGFX2_VERTEX_SHADER "${VKGFX2_SHADER_OUTPUT_DIRECTORY}/triangle.vert.dxil")
set(VKGFX2_FRAGMENT_SHADER "${VKGFX2_SHADER_OUTPUT_DIRECTORY}/triangle.frag.dxil")

add_custom_command(
    OUTPUT "${VKGFX2_VERTEX_SHADER}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${VKGFX2_SHADER_OUTPUT_DIRECTORY}"
    COMMAND "${VKGFX2_DXC_EXECUTABLE}" -T vs_6_0 -E main -Fo "${VKGFX2_VERTEX_SHADER}"
            "${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle.vert.hlsl"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle.vert.hlsl"
    VERBATIM
)

add_custom_command(
    OUTPUT "${VKGFX2_FRAGMENT_SHADER}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${VKGFX2_SHADER_OUTPUT_DIRECTORY}"
    COMMAND "${VKGFX2_DXC_EXECUTABLE}" -T ps_6_0 -E main -Fo "${VKGFX2_FRAGMENT_SHADER}"
            "${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle.frag.hlsl"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle.frag.hlsl"
    VERBATIM
)

add_custom_target(vkgfx2_shaders DEPENDS
    "${VKGFX2_VERTEX_SHADER}"
    "${VKGFX2_FRAGMENT_SHADER}"
)

add_library(Engine STATIC
    ${ENGINE_SOURCES}
    ${ENGINE_HEADERS}
)

add_dependencies(Engine vkgfx2_shaders)

target_include_directories(Engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(Engine
    PUBLIC
        SDL3::SDL3
        spdlog::spdlog
)

if(MSVC)
    target_compile_options(Engine PRIVATE
        "$<$<CONFIG:Release>:/O2>"
        "$<$<CONFIG:Release>:/GL>"
    )
endif()
