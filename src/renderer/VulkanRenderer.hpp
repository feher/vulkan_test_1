#pragma once

#include "common/Types.hpp"
#include "renderer/IRenderer.hpp"
#include "window/IWindow.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <optional>

namespace VkTest1::Renderer::Detail
{

struct QueueFamilyInfo
{
    std::optional<std::uint32_t> graphicsQueueFamilyIndex{};
    std::optional<std::uint32_t> presentationQueueFamilyIndex{};
};

struct PhysicalDevice
{
    vk::raii::PhysicalDevice device;
    QueueFamilyInfo queueFamilyInfo{};
};

class VulkanRenderer : public IRenderer
{
public:
    explicit VulkanRenderer(Common::NotNull<Window::IWindow*> window);

    ~VulkanRenderer() override;

private:
    Common::NotNull<Window::IWindow*> m_window{};
    vk::raii::Context m_context{};
    vk::raii::Instance m_instance;
    vk::raii::SurfaceKHR m_surface;
    vk::raii::DebugUtilsMessengerEXT m_debugMessenger;
    Detail::PhysicalDevice m_physicalDevice;
    vk::raii::Device m_device;
    vk::raii::Queue m_graphicsQueue;
    vk::raii::Queue m_presentationQueue;
};

} // namespace VkTest1::Renderer::Detail
