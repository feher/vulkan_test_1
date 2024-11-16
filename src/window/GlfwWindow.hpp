#pragma once

#include "common/Types.hpp"
#include "window/IWindow.hpp"

struct GLFWwindow;

namespace VkTest1::Window::Detail
{

class GlfwWindow : public IWindow
{
public:
    explicit GlfwWindow(Common::Uint width, Common::Uint height, const char* title);
    ~GlfwWindow() override;

    bool shouldClose() const override;
    void handleEvents() override;

    std::vector<const char*> getRendererInstanceExtensions() const override;

    OpaqueSurface createSurface(void* rendererInstance) override;

private:
    Common::NotNull<GLFWwindow*> m_window;
};

} // namespace VkTest1::Window::Detail
