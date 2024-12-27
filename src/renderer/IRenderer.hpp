#pragma once

namespace VkTest1::Renderer
{

class IRenderer
{
public:
    virtual ~IRenderer() = default;

    virtual void draw() = 0;
};

} // namespace VkTest1::Renderer
