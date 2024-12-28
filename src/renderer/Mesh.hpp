#pragma once

#include "geometry/Vertex.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <span>

namespace VkTest1::Renderer
{

class Mesh
{
public:
    explicit Mesh(
        const vk::PhysicalDevice& physicalDevice, const vk::raii::Device& device,
        std::span<const Geometry::Vertex> vertices);

    Mesh(const Mesh& other) = delete;
    Mesh& operator=(const Mesh& other) = delete;

    Mesh(Mesh&& other) = default;
    Mesh& operator=(Mesh&& other) = default;

    std::size_t getVertexCount() const
    {
        return m_vertexCount;
    }

    const vk::Buffer getVertexBuffer() const
    {
        return m_vertexBuffer;
    }

private:
    std::size_t m_vertexCount;
    vk::raii::Buffer m_vertexBuffer;
    vk::raii::DeviceMemory m_vertexBufferMemory;
};

} // namespace VkTest1::Renderer
