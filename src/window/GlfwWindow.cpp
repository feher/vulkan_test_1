#include "window/GlfwWindow.hpp"

#include "common/Errors.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <algorithm>
#include <ranges>

namespace VkTest1::Window::Detail
{

GlfwWindow::GlfwWindow(Common::Uint width, Common::Uint height, const char* title)
{
    glfwInit();

    // We don't want GLFW to use any API by default. We will use Vulkan API.
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // We don't want to handle re-seize events for now.
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
}

GlfwWindow::~GlfwWindow()
{
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

bool GlfwWindow::shouldClose() const
{
    return glfwWindowShouldClose(m_window);
}

void GlfwWindow::handleEvents()
{
    glfwPollEvents();
}

std::vector<const char*> GlfwWindow::getRendererInstanceExtensions() const
{
    uint32_t extensionCount{ 0 };
    const char** extensionNames = glfwGetRequiredInstanceExtensions(&extensionCount);

    std::vector<const char*> extensions{};
    extensions.reserve(extensionCount);
    std::ranges::copy_n(extensionNames, extensionCount, std::back_inserter(extensions));

    return extensions;
}

IWindow::OpaqueSurface GlfwWindow::createSurface(void* rendererInstance)
{
    // Note: GlfwWindow should not know that we're using Vulkan.
    // But unfortunately it NEEDS to know.
    VkSurfaceKHR surface{ nullptr };
    auto instance = static_cast<VkInstance>(rendererInstance);
    const auto result = glfwCreateWindowSurface(instance, m_window, nullptr, &surface);
    if (result != VK_SUCCESS)
    {
        throw Common::WindowError{ "Cannot create surface." };
    }
    return surface;
}

} // namespace VkTest1::Window::Detail
