set(GLSL_VALIDATOR "${PROJECT_SOURCE_DIR}/third-party/glslang/bin/glslangValidator.exe")

file(GLOB_RECURSE GLSL_SOURCE_FILES
    "*.frag"
    "*.vert"
    "*.rgen"
    "*.rmiss"
    "*.rchit"
    )

foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${PROJECT_BINARY_DIR}/shaders/${FILE_NAME}.spv")
    add_custom_command(
      OUTPUT ${SPIRV}
      COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/shaders/"
      COMMAND ${GLSL_VALIDATOR} --target-env vulkan1.3 -V -o ${SPIRV} ${GLSL}
      DEPENDS ${GLSL})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(shaders ALL DEPENDS ${SPIRV_BINARY_FILES})