#pragma once

#include "common/Types.hpp"

#include <memory>

namespace VkTest1
{

namespace Window
{
class IWindow;
}

namespace Renderer
{
class IRenderer;
}

class Factory
{
public:
    std::unique_ptr<Window::IWindow> createWindow();
    std::unique_ptr<Renderer::IRenderer> createRenderer(Common::NotNull<Window::IWindow*> window);
};

} // namespace VkTest1
