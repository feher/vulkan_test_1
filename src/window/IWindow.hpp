#pragma once

#include <vector>
#include <utility> // pair

namespace VkTest1::Window
{

class IWindow
{
public:
    using OpaqueSurface = void*;

    virtual ~IWindow() = default;

    virtual bool shouldClose() const = 0;
    virtual void handleEvents() = 0;

    virtual std::vector<const char*> getRendererInstanceExtensions() const = 0;

    virtual OpaqueSurface createSurface(void* rendererInstance) = 0;

    virtual std::pair<Common::Uint, Common::Uint> getSize() const = 0;
};

} // namespace VkTest1::Window
