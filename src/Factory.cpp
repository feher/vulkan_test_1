#include "Factory.hpp"

#include "window/GlfwWindow.hpp"
#include "renderer/VulkanRenderer.hpp"

namespace VkTest1
{

std::unique_ptr<Window::IWindow> Factory::createWindow()
{
    return std::make_unique<Window::Detail::GlfwWindow>(800, 600, "Hello");
}

std::unique_ptr<Renderer::IRenderer> Factory::createRenderer(Common::NotNull<Window::IWindow*> window)
{
    return std::make_unique<Renderer::Detail::VulkanRenderer>(window);
}

} // namespace VkTest1
