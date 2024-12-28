#include "Mesh.hpp"

#include "common/Errors.hpp"
#include "common/Types.hpp"

using namespace VkTest1;

namespace
{

std::uint32_t findMemoryTypeIndex(
    const vk::PhysicalDevice& physicalDevice, std::uint32_t allowedTypes, vk::MemoryPropertyFlags propertyFlags)
{
    const auto memoryProperties{ physicalDevice.getMemoryProperties() };

    for (auto i{ 0u }; i != memoryProperties.memoryTypeCount; ++i)
    {
        if (Common::Flags::isFlagSet(allowedTypes, i) &&
            Common::Flags::isMaskSet(memoryProperties.memoryTypes[i].propertyFlags, propertyFlags))
        {
            return i;
        }
    }
    throw Common::RendererError{ "Cannot find memory type index." };
}

// This does not allocate memory.
vk::raii::Buffer createVertexBuffer(const vk::raii::Device& device, std::span<const Geometry::Vertex> vertices)
{
    const auto bufferSize{ sizeof(Geometry::Vertex) * vertices.size() };
    return device.createBuffer(vk::BufferCreateInfo{ /* flags */ {},
                                                     /* size */ bufferSize,
                                                     /* usage */ vk::BufferUsageFlagBits::eVertexBuffer,
                                                     // eExclusive means "no sharing".
                                                     /* sharingMode */ vk::SharingMode::eExclusive });
}

vk::raii::DeviceMemory allocateDeviceMemory(
    const vk::PhysicalDevice& physicalDevice, const vk::raii::Device& device, const vk::raii::Buffer& buffer)
{
    const auto memoryReqirements{ buffer.getMemoryRequirements() };
    auto deviceMemory{ device.allocateMemory(vk::MemoryAllocateInfo{
        /* allocationSize */ memoryReqirements.size,
        /* memoryTypeIndex */
        findMemoryTypeIndex(
            physicalDevice,
            /* allowedTypes */ memoryReqirements.memoryTypeBits,
            // HostVisible = CPU can access it.
            // HostCoherent = No need for manual flush (i.e. memory cache management).
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent) }) };
    return deviceMemory;
}

void bindMemoryAndCopyData(
    const vk::raii::Buffer& vertexBuffer, const vk::raii::DeviceMemory& deviceMemory,
    std::span<const Geometry::Vertex> vertices)
{
    // Bind memory to buffer.
    vertexBuffer.bindMemory(deviceMemory, /* memoryOffset */ 0);

    // Copy data to device memory.
    const auto bufferSize{ sizeof(Geometry::Vertex) * vertices.size() };
    auto* mappedVertices{ static_cast<Geometry::Vertex*>(
        deviceMemory.mapMemory(/* offset */ 0, /* size */ bufferSize, /* flags*/ {})) };
    std::copy_n(vertices.data(), vertices.size(), mappedVertices);
    deviceMemory.unmapMemory();
}

} // namespace

namespace VkTest1::Renderer
{

Mesh::Mesh(
    const vk::PhysicalDevice& physicalDevice, const vk::raii::Device& device,
    std::span<const Geometry::Vertex> vertices) :
    m_vertexCount{ vertices.size() },
    m_vertexBuffer{ createVertexBuffer(device, vertices) },
    m_vertexBufferMemory{ allocateDeviceMemory(physicalDevice, device, m_vertexBuffer) }
{
    bindMemoryAndCopyData(m_vertexBuffer, m_vertexBufferMemory, vertices);
}

} // namespace VkTest1::Renderer
