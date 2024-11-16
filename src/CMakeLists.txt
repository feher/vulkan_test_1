add_executable(vulkan_test_01
	"main.cpp"
	"Factory.hpp"
	"Factory.cpp"

	"common/Cast.hpp"
	"common/Errors.hpp"
	"common/Types.hpp"

	"renderer/DebugUtilsMessenger.cpp"
	"renderer/DebugUtilsMessenger.hpp"
	"renderer/IRenderer.hpp"
	"renderer/VulkanRenderer.cpp"
	"renderer/VulkanRenderer.hpp"

	"window/GlfwWindow.cpp"
	"window/GlfwWindow.hpp"
	"window/IWindow.hpp"
)

target_include_directories(vulkan_test_01 PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(vulkan_test_01 PRIVATE
	Vulkan::Vulkan
	glfw
	glm::glm
)

set_target_properties(vulkan_test_01 PROPERTIES
	CXX_STANDARD 23
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
)
