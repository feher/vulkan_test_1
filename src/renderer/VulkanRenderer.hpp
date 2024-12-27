#pragma once

#include "common/IFileSystem.hpp"
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
    std::uint32_t deviceIndex;
};

struct SwapchainImage
{
    // This image is owned by the swapchain, so no need for raii here.
    vk::Image image;
    vk::raii::ImageView imageView;
};

struct Swapchain
{
    vk::raii::SwapchainKHR swapchain;
    vk::Format imageFormat;
    vk::Extent2D imageExtent;
    std::vector<SwapchainImage> images;
};

class VulkanRenderer : public IRenderer
{
public:
    explicit VulkanRenderer(Common::NotNull<Common::IFileSystem*> fileSystem, Common::NotNull<Window::IWindow*> window);

    ~VulkanRenderer() override;

    void draw() override;

private:
    unsigned int m_currentFrame{ 0 };
    Common::NotNull<Common::IFileSystem*> m_fileSystem{};
    Common::NotNull<Window::IWindow*> m_window{};
    vk::raii::Context m_context{};
    vk::raii::Instance m_instance;
    vk::raii::DebugUtilsMessengerEXT m_debugMessenger;
    vk::raii::SurfaceKHR m_surface;
    PhysicalDevice m_physicalDevice;
    vk::raii::Device m_device;
    Swapchain m_swapchain;
    vk::raii::Queue m_graphicsQueue;
    vk::raii::Queue m_presentationQueue;
    vk::raii::RenderPass m_renderPass;
    vk::raii::PipelineLayout m_pipelineLayout;
    vk::raii::Pipeline m_pipeline;
    std::vector<vk::raii::Framebuffer> m_framebuffers;
    vk::raii::CommandPool m_graphicsCommandPool;
    std::vector<vk::raii::CommandBuffer> m_commandBuffers;
    std::vector<vk::raii::Semaphore> m_imageAvailable;
    std::vector<vk::raii::Semaphore> m_renderFinished;
    std::vector<vk::raii::Fence> m_drawFence;
};

} // namespace VkTest1::Renderer::Detail
