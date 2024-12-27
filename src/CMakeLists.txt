set(myTargetName "vulkan_test_01")

################################################################################
#
# Build shaders
#

set(shaderBinaries
    "${CMAKE_CURRENT_BINARY_DIR}/renderer/shaders/vert.spv"
    "${CMAKE_CURRENT_BINARY_DIR}/renderer/shaders/frag.spv")

add_custom_command(
    OUTPUT ${shaderBinaries}
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/renderer/shaders/vert.glsl"
        "${CMAKE_CURRENT_SOURCE_DIR}/renderer/shaders/frag.glsl"
    COMMAND Vulkan::glslc
    ARGS
        --target-env=vulkan -fshader-stage=vertex
        -o "${CMAKE_CURRENT_BINARY_DIR}/renderer/shaders/vert.spv"
        "${CMAKE_CURRENT_SOURCE_DIR}/renderer/shaders/vert.glsl"
    COMMAND Vulkan::glslc
    ARGS
        --target-env=vulkan -fshader-stage=fragment
        -o "${CMAKE_CURRENT_BINARY_DIR}/renderer/shaders/frag.spv"
        "${CMAKE_CURRENT_SOURCE_DIR}/renderer/shaders/frag.glsl"
)

add_custom_target(${myTargetName}_shaders ALL DEPENDS ${shaderBinaries})

################################################################################
#
# Build program
#

add_executable(${myTargetName}
    "main.cpp"
    "Factory.hpp"
    "Factory.cpp"

    "common/Cast.hpp"
    "common/Errors.hpp"
    "common/Types.hpp"
    "common/IFileSystem.hpp"
    "common/FileSystem.hpp"
    "common/FileSystem.cpp"

    "renderer/DebugUtilsMessenger.cpp"
    "renderer/DebugUtilsMessenger.hpp"
    "renderer/IRenderer.hpp"
    "renderer/VulkanRenderer.cpp"
    "renderer/VulkanRenderer.hpp"

    "window/GlfwWindow.cpp"
    "window/GlfwWindow.hpp"
    "window/IWindow.hpp"
)

target_include_directories(${myTargetName} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${myTargetName} PRIVATE
    Vulkan::Vulkan
    glfw
    glm::glm
)

set_target_properties(${myTargetName} PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_dependencies(${myTargetName} ${myTargetName}_shaders)

add_custom_command(TARGET ${myTargetName}
    POST_BUILD
    COMMENT "Copying shaders to $<TARGET_FILE_DIR:${myTargetName}>/renderer/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_BINARY_DIR}/renderer/shaders" "$<TARGET_FILE_DIR:${myTargetName}>/renderer/shaders")
