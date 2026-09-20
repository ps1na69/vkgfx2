add_executable(Sandbox
    src/main.cpp
)

target_link_libraries(Sandbox
    PRIVATE
        Engine
)

if(MSVC)
    target_link_options(Sandbox PRIVATE
        "$<$<CONFIG:Release>:/SUBSYSTEM:WINDOWS>"
        "$<$<CONFIG:Release>:/ENTRY:mainCRTStartup>"
    )
endif()

# Keep SDL3.dll next to Sandbox.exe for local runs on Windows.
# SDL3::SDL3 is an alias to SDL3-shared when SDL_SHARED is enabled.
if(TARGET SDL3-shared)
    add_custom_command(TARGET Sandbox POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:SDL3-shared>"
            "$<TARGET_FILE_DIR:Sandbox>"
        COMMENT "Copying SDL3 runtime next to Sandbox"
    )
endif()

add_custom_command(TARGET Sandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Sandbox>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${VKGFX2_SHADER_OUTPUT_DIRECTORY}"
        "$<TARGET_FILE_DIR:Sandbox>/shaders"
    COMMENT "Copying compiled shaders next to Sandbox"
)
