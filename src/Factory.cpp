#include "Factory.hpp"

#include "common/FileSystem.hpp"
#include "renderer/VulkanRenderer.hpp"
#include "window/GlfwWindow.hpp"

namespace VkTest1
{

std::unique_ptr<Common::IFileSystem> Factory::createFileSystem()
{
    return std::make_unique<Common::FileSystem>();
}

std::unique_ptr<Window::IWindow> Factory::createWindow()
{
    return std::make_unique<Window::Detail::GlfwWindow>(800, 600, "Hello");
}

std::unique_ptr<Renderer::IRenderer> Factory::createRenderer(
    Common::NotNull<Common::IFileSystem*> fileSystem, Common::NotNull<Window::IWindow*> window)
{
    return std::make_unique<Renderer::Detail::VulkanRenderer>(fileSystem, window);
}

} // namespace VkTest1
