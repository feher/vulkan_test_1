#include "renderer/VulkanRenderer.hpp"

#include "common/Cast.hpp"
#include "common/Errors.hpp"
#include "renderer/DebugUtilsMessenger.hpp"

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <print>
#include <ranges>
#include <span>
#include <unordered_set>

using namespace VkTest1;

namespace
{

const std::array<const char* const, 1> s_requiredInstanceLayers{ "VK_LAYER_KHRONOS_validation" };
const std::array<const char* const, 1> s_requiredPhysicalDeviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

constexpr unsigned int s_maxFrameCountInQueue = 2;

std::vector<const char*> getInstanceExtensions(const Window::IWindow& window)
{
    auto extensions = window.getRendererInstanceExtensions();

    // To set up a callback in the program to handle messages and the associated details,
    // we have to set up a debug messenger with a callback using the VK_EXT_debug_utils extension.
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return extensions;
}

bool areExtensionsSupported(
    std::span<const vk::ExtensionProperties> propsList, std::span<const char* const> extensionNames)
{
    const auto isExtensionSupported = [&propsList](const char* const extensionName)
    {
        std::println("Vulkan: Checking extension '{}'", extensionName);
        return std::ranges::any_of(
            propsList,
            [extensionName](const vk::ExtensionProperties& props)
            {
                return std::string_view{ props.extensionName } == std::string_view{ extensionName };
            });
    };
    return std::ranges::all_of(extensionNames, isExtensionSupported);
}

bool areInstanceExtensionsSupported(std::span<const char* const> extensionNames)
{
    std::println("Vulkan: Checking instance extension support:");
    return areExtensionsSupported(vk::enumerateInstanceExtensionProperties(nullptr), extensionNames);
}

bool arePhysicalDeviceExtensionsSupported(
    const vk::raii::PhysicalDevice& physicalDevice, std::span<const char* const> extensionNames)
{
    std::println("Vulkan: Checking physical device extension support:");
    return areExtensionsSupported(physicalDevice.enumerateDeviceExtensionProperties(), extensionNames);
}

bool areInstanceLayersSupported(std::span<const char* const> layerNames)
{
    std::println("Vulkan: Checking instance layer support:");
    const auto propsList = vk::enumerateInstanceLayerProperties();
    const auto isLayerSupported = [&propsList](const char* const layerName)
    {
        std::println("Vulkan: Checking layer '{}'", layerName);
        return std::ranges::any_of(
            propsList,
            [layerName](const vk::LayerProperties& props)
            {
                return std::string_view{ props.layerName } == std::string_view{ layerName };
            });
    };
    return std::ranges::all_of(layerNames, isLayerSupported);
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL debugUtilsMessengerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    std::println("Vulkan: Validation message: {}", pCallbackData->pMessage);
    return vk::False;
}

vk::raii::Instance createInstance(const vk::raii::Context& context, const Window::IWindow& window)
{
    const vk::ApplicationInfo applicationInfo{ "Vulkan test app", 1, "Custom engine", 1, VK_API_VERSION_1_1 };

    const auto extensions = getInstanceExtensions(window);

    if (!areInstanceExtensionsSupported(extensions))
    {
        throw Common::RendererError{ "Some required Vulkan instance extensions are not supported." };
    }

    if (!areInstanceLayersSupported(s_requiredInstanceLayers))
    {
        throw Common::RendererError{ "Some required Vulkan instance layers are not supported." };
    }

    const vk::InstanceCreateInfo instanceCreateInfo{
        /* flags */ {},
        /* app info */ &applicationInfo,
        /* enabled layer count */ Common::NarrowCast<uint32_t>(s_requiredInstanceLayers.size()),
        /* enabled layer names */ s_requiredInstanceLayers.data(),
        /* extension count */ Common::NarrowCast<uint32_t>(extensions.size()),
        /* extension names */ extensions.data()
    };

    return vk::raii::Instance{ context, instanceCreateInfo };
}

vk::raii::DebugUtilsMessengerEXT createDebugMessenger(const vk::raii::Instance& instance)
{
    Renderer::Details::initDebugUtilsMessengerExtension(instance);

    const vk::DebugUtilsMessengerCreateInfoEXT messengerCreateInfo{
        /* flags */ {},
        /* severity flags */ vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo,
        /* message type flags */ vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding,
        /* callback */ debugUtilsMessengerCallback
    };

    return instance.createDebugUtilsMessengerEXT(messengerCreateInfo);
}

vk::SurfaceFormatKHR chooseSwapchainFormat(std::span<const vk::SurfaceFormatKHR> formats)
{
    // This is a special case.
    // It means that every format is available. So, we can freely pick any.
    if (formats.size() == 1 && formats[0].format == vk::Format::eUndefined)
    {
        return { vk::Format::eR8G8B8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };
    }

    // Try to find our required surface format.
    const auto it = std::ranges::find_if(
        formats,
        [](const vk::SurfaceFormatKHR& format)
        {
            return (format.format == vk::Format::eR8G8B8A8Unorm || format.format == vk::Format::eB8G8R8A8Unorm) &&
                format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
        });
    if (it != std::end(formats))
    {
        return *it;
    }

    // Fallback to the first one. Whatever that is.
    std::println("Vulkan: Cannot find required surface format. Fallback to the first available one.");
    return formats[0];
}

vk::PresentModeKHR chooseSwapchainPresentationMode(std::span<const vk::PresentModeKHR> presentationModes)
{
    const auto it = std::ranges::find_if(
        presentationModes,
        [](const vk::PresentModeKHR& mode)
        {
            return mode == vk::PresentModeKHR::eMailbox;
        });
    if (it != std::end(presentationModes))
    {
        return *it;
    }

    // Fallback to FIFO mode. It should always be present according to the Vulkan spec.
    std::println("Vulkan: Cannot find mailbox presentation mode. Fallback to FIFO.");
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D chooseSwapchainImageExtent(
    const vk::SurfaceCapabilitiesKHR& surfaceCapabilities, const Window::IWindow& window)
{
    if (surfaceCapabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max())
    {
        // The current extent is the same size as our window.
        return surfaceCapabilities.currentExtent;
    }

    // The current extent is not set. We must get it from the window manually.
    auto windowSize{ window.getSize() };
    windowSize.first = std::min(windowSize.first, surfaceCapabilities.maxImageExtent.width);
    windowSize.first = std::max(windowSize.first, surfaceCapabilities.minImageExtent.width);
    windowSize.second = std::min(windowSize.second, surfaceCapabilities.maxImageExtent.width);
    windowSize.second = std::max(windowSize.second, surfaceCapabilities.minImageExtent.width);
    return { windowSize.first, windowSize.second };
}

std::uint32_t chooseSwapchainImageCount(const vk::SurfaceCapabilitiesKHR& surfaceCapabilities)
{
    // +1 because we want to allow triple buffering.
    const auto imageCount{ surfaceCapabilities.minImageCount + 1 };
    // If max image count it 0 then there is no max limit.
    return (surfaceCapabilities.maxImageCount == 0)
        ? imageCount
        : std::clamp(imageCount, surfaceCapabilities.minImageCount, surfaceCapabilities.maxImageCount);
}

vk::raii::ImageView createImageView(
    const vk::raii::Device& logicalDevice, const vk::Image& image, vk::Format format, vk::ImageAspectFlags aspectFlags)
{
    const vk::ImageViewCreateInfo imageViewCreateInfo{ /* flags */ {},
                                                       image,
                                                       vk::ImageViewType::e2D,
                                                       format,
                                                       /* component mapping */ {},
                                                       { aspectFlags,
                                                         /* base mipmap level */ 0,
                                                         /* mipmap level count */ 1,
                                                         /* base array layer */ 0,
                                                         /* array layer count */ 1 } };
    return vk::raii::ImageView{ logicalDevice, imageViewCreateInfo };
}

Renderer::Detail::Swapchain createSwapchain(
    const Window::IWindow& window, const vk::raii::SurfaceKHR& surface,
    const Renderer::Detail::PhysicalDevice& physicalDevice, const vk::raii::Device& logicalDevice)
{
    const auto surfaceCapabilities{ physicalDevice.device.getSurfaceCapabilitiesKHR(surface) };

    const auto format{ chooseSwapchainFormat(physicalDevice.device.getSurfaceFormatsKHR(surface)) };
    const auto presentationMode{ chooseSwapchainPresentationMode(
        physicalDevice.device.getSurfacePresentModesKHR(surface)) };
    const auto imageExtent{ chooseSwapchainImageExtent(surfaceCapabilities, window) };
    const auto imageCount{ chooseSwapchainImageCount(surfaceCapabilities) };

    vk::SwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.setSurface(surface);
    swapchainCreateInfo.setImageFormat(format.format);
    swapchainCreateInfo.setImageColorSpace(format.colorSpace);
    swapchainCreateInfo.setPresentMode(presentationMode);
    swapchainCreateInfo.setImageExtent(imageExtent);
    swapchainCreateInfo.setMinImageCount(imageCount);
    // Number of layers for each image in the swapchain.
    swapchainCreateInfo.setImageArrayLayers(1);
    swapchainCreateInfo.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);
    swapchainCreateInfo.setPreTransform(surfaceCapabilities.currentTransform);
    // Clip parts of the image not being in view (e.g. by another OS window).
    swapchainCreateInfo.setClipped(true);

    if (physicalDevice.queueFamilyInfo.graphicsQueueFamilyIndex !=
        physicalDevice.queueFamilyInfo.presentationQueueFamilyIndex)
    {
        // We need to share images between the graphics and the presentation queue.
        swapchainCreateInfo.setImageSharingMode(vk::SharingMode::eConcurrent);
        swapchainCreateInfo.setQueueFamilyIndexCount(2);
        const std::array<std::uint32_t, 2> indices{
            physicalDevice.queueFamilyInfo.graphicsQueueFamilyIndex.value(),
            physicalDevice.queueFamilyInfo.presentationQueueFamilyIndex.value()
        };
        swapchainCreateInfo.setQueueFamilyIndices(indices);
    }

    vk::raii::SwapchainKHR swapchain{ logicalDevice, swapchainCreateInfo };

    std::vector<Renderer::Detail::SwapchainImage> imageAndViewList{};
    for (const auto& image : swapchain.getImages())
    {
        imageAndViewList.emplace_back(
            image, createImageView(logicalDevice, image, format.format, vk::ImageAspectFlagBits::eColor));
    }

    return Renderer::Detail::Swapchain{ std::move(swapchain), format.format, imageExtent, std::move(imageAndViewList) };
}

Renderer::Detail::QueueFamilyInfo getQueueFamilyInfo(
    const vk::raii::PhysicalDevice& physicalDevice, const vk::raii::SurfaceKHR& surface)
{
    Renderer::Detail::QueueFamilyInfo qfInfo{};
    const auto queuePropsList = physicalDevice.getQueueFamilyProperties();
    for (int i = 0; i < queuePropsList.size(); ++i)
    {
        const vk::QueueFamilyProperties& props = queuePropsList[i];

        // Check if this queue family supports Graphics operations.
        if (props.queueCount > 0 && (props.queueFlags & vk::QueueFlagBits::eGraphics))
        {
            qfInfo.graphicsQueueFamilyIndex = i;
        }

        // Check if this queue family supports presentation to a
        // surface (i.e. if it's a Presentation Queue Family).
        if (props.queueCount > 0 && physicalDevice.getSurfaceSupportKHR(i, *surface))
        {
            qfInfo.presentationQueueFamilyIndex = i;
        }
    }
    return qfInfo;
}

void printPhysicalDeviceInfo(const vk::raii::PhysicalDevice& device)
{
    const auto props = device.getProperties();
    std::println("Vulkan: Physical device name: {}", std::string_view{ props.deviceName });
}

Renderer::Detail::PhysicalDevice getPhysicalDevice(
    const vk::raii::Instance& instance, const vk::raii::SurfaceKHR& surface)
{
    const auto physicalDevices = instance.enumeratePhysicalDevices();
    for (auto i = 0u; i != physicalDevices.size(); ++i)
    {
        const auto& physicalDevice{ physicalDevices[i] };
        const auto queueFamilyInfo = getQueueFamilyInfo(physicalDevice, surface);
        if (queueFamilyInfo.graphicsQueueFamilyIndex.has_value() &&
            queueFamilyInfo.presentationQueueFamilyIndex.has_value() &&
            arePhysicalDeviceExtensionsSupported(physicalDevice, s_requiredPhysicalDeviceExtensions) &&
            !physicalDevice.getSurfacePresentModesKHR(surface).empty() &&
            !physicalDevice.getSurfaceFormatsKHR(surface).empty())
        {
            return { physicalDevice, queueFamilyInfo, i };
        }
    }
    throw Common::RendererError{ "Cannot find suitable physical device." };
}

std::unordered_set<uint32_t> getUniqueQueueFamilyIndices(const Renderer::Detail::QueueFamilyInfo& qfInfo)
{
    std::unordered_set<uint32_t> indices{};
    if (qfInfo.graphicsQueueFamilyIndex.has_value())
    {
        indices.insert(*qfInfo.graphicsQueueFamilyIndex);
    }
    if (qfInfo.presentationQueueFamilyIndex.has_value())
    {
        indices.insert(*qfInfo.presentationQueueFamilyIndex);
    }
    return indices;
}

vk::raii::Device createLogicalDevice(const Renderer::Detail::PhysicalDevice& physicalDevice)
{
    const std::array<float, 1> queuePriorities{ 1.0f };

    // Some of the queue family indices may point to the same queue family (i.e. the index is the same).
    // So, we make sure we create exactly one DeviceQueueCreateInfo per queue family.
    // Otherwise Vulkan may crash when creating the logical device.
    const auto uniqueQueueFamilyIndices{ getUniqueQueueFamilyIndices(physicalDevice.queueFamilyInfo) };
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos{};
    for (const auto queueFamilyIndex : uniqueQueueFamilyIndices)
    {
        queueCreateInfos.push_back(
            vk::DeviceQueueCreateInfo{ /* flags */ {},
                                       /* queueFamilyIndex */ queueFamilyIndex,
                                       /* queueCount */ Common::NarrowCast<uint32_t>(queuePriorities.size()),
                                       /* queuePriorities */ queuePriorities.data() });
    }

    const vk::DeviceCreateInfo deviceCreateInfo{
        /* flags */ {},
        /* queue create info count */ Common::NarrowCast<uint32_t>(queueCreateInfos.size()),
        /* queue create infos */ queueCreateInfos.data(),
        /* enabled layer count */ 0,
        /* enabled layer names */ nullptr,
        /* extension count */ s_requiredPhysicalDeviceExtensions.size(),
        /* extension names */ s_requiredPhysicalDeviceExtensions.data()
    };
    return physicalDevice.device.createDevice(deviceCreateInfo);
}

vk::raii::ShaderModule createShaderModule(const vk::raii::Device& device, std::span<const std::byte> spirvBinary)
{
    vk::ShaderModuleCreateInfo createInfo{ /* flags */ {},
                                           spirvBinary.size(),
                                           reinterpret_cast<const uint32_t*>(spirvBinary.data()) };

    return vk::raii::ShaderModule{ device, createInfo };
}

vk::raii::RenderPass createRenderPass(const vk::raii::Device& device, vk::Format colorAttachmentFormat)
{
    //
    // The color attachment image layout goes through the following conversions during the
    // render pass:
    //
    // 1. Begin render pass: Expected as Undefined (no conversion).
    // 2. Begin sub pass: Converted to ColorAttachmentOptimal.
    // 3. End render pass: Converted to PresentSrcKHR.
    //

    const std::array<vk::AttachmentDescription, 1> attachmentDescriptions{
        // Color attachment description of the entire render pass.
        vk::AttachmentDescription{ /* flags */ {},
                                   /* format */ colorAttachmentFormat,
                                   // Number of samples to write for multisampling.
                                   /* samples */ vk::SampleCountFlagBits::e1,
                                   // What to do with color attachment when the render pass starts.
                                   // The clear-value is defined by RenderPassBeginInfo.
                                   /* loadOp */ vk::AttachmentLoadOp::eClear,
                                   // What to do with color attachment when the render pass ends.
                                   /* storeOp */ vk::AttachmentStoreOp::eStore,
                                   // What to do with stencil attachment when the render pass starts.
                                   /* loadOp */ vk::AttachmentLoadOp::eDontCare,
                                   // What to do with stencil attachment when the render pass ends.
                                   /* storeOp */ vk::AttachmentStoreOp::eDontCare,
                                   // Image data layout before render pass. We don't care.
                                   /* initialLayout_ */ vk::ImageLayout::eUndefined,
                                   // Image data layout after render pass. Will be the source for presentation.
                                   /* finalLayout_ */ vk::ImageLayout::ePresentSrcKHR }
    };

    const std::array<vk::AttachmentReference, 1> colorAttachmentsRefs{ vk::AttachmentReference{
        // References an attachment by index from the render pass.
        /* attachment */ 0,
        /* layout */ vk::ImageLayout::eColorAttachmentOptimal } };

    const std::array<vk::SubpassDescription, 1> subpasses{
        // Subpass 0
        vk::SubpassDescription{ /* flags */ {},
                                // The pipeline type of the subpass.
                                /* pipelineBindPoint */ vk::PipelineBindPoint::eGraphics,
                                /* pInputAttachments */ {},
                                /* pColorAttachments */ colorAttachmentsRefs }
    };

    //
    // To see what pipeline stages a certain access can happen in, see:
    // https://registry.khronos.org/vulkan/specs/latest/man/html/VkAccessFlagBits.html
    //
    const std::array<vk::SubpassDependency, 2> subpassDependencies{
        // When going from subpass "External" to subpass 0,
        // the conversion from Undefined to ColorAttachmentOptimal
        // has to happen after (stage: BottomOfPipe, access: MemoryRead)
        // but before (stage: ColorAttachmentOutput, access: ColorAttachmentRead | ColorAttachmentWrite).
        vk::SubpassDependency{ /* srcSubpass */ vk::SubpassExternal,
                               // 0 is the index of the first subpass.
                               /* dstSubpass */ 0,
                               // BottomOfPipe means the end of the pipeline.
                               /* srcStageMask */ vk::PipelineStageFlagBits::eBottomOfPipe,
                               /* dstStageMask */ vk::PipelineStageFlagBits::eColorAttachmentOutput,
                               /* srcAccessMask */ vk::AccessFlagBits::eMemoryRead,
                               /* dstAccessMask */ vk::AccessFlagBits::eColorAttachmentRead |
                                   vk::AccessFlagBits::eColorAttachmentWrite },
        // When going from subpass 0 to subpass "External",
        // the conversion from ColorAttachmentOptimal to PresentSrcKHR
        // has to happen after (stage: ColorAttachmentOutput, access: ColorAttachmentRead | ColorAttachmentWrite)
        // but before (stage: eBottomOfPipe, access: MemoryRead).
        vk::SubpassDependency{ /* srcSubpass */ 0,
                               /* dstSubpass */ vk::SubpassExternal,
                               /* srcStageMask */ vk::PipelineStageFlagBits::eColorAttachmentOutput,
                               // BottomOfPipe means the end of the pipeline.
                               /* dstStageMask */ vk::PipelineStageFlagBits::eBottomOfPipe,
                               /* srcAccessMask */ vk::AccessFlagBits::eColorAttachmentRead |
                                   vk::AccessFlagBits::eColorAttachmentWrite,
                               /* dstAccessMask */ vk::AccessFlagBits::eMemoryRead }
    };

    const vk::RenderPassCreateInfo renderPassCI{ /* flags */ {},
                                                 /* pAttachments */ attachmentDescriptions,
                                                 /* pSubpasses */ subpasses,
                                                 /* pDependencies */ subpassDependencies };

    return device.createRenderPass(renderPassCI);
}

vk::raii::PipelineLayout createPipelineLayout(const vk::raii::Device& device)
{
    // Apply descriptor set layouts.
    const vk::PipelineLayoutCreateInfo layoutCI{};
    return device.createPipelineLayout(layoutCI);
}

vk::raii::Pipeline createPipeline(
    Common::IFileSystem& fileSystem, const vk::raii::Device& device, const vk::Extent2D& viewportSize,
    const vk::raii::RenderPass& renderPass, const vk::raii::PipelineLayout& pipelineLayout)
{
    // -- SHADER MODULES

    const auto vertexShaderSpv{ fileSystem.readFile("./renderer/shaders/vert.spv") };
    const auto fragmentShaderSpv{ fileSystem.readFile("./renderer/shaders/frag.spv") };

    // The shader modules don't need to be retained.
    auto vertexShaderModule{ createShaderModule(device, vertexShaderSpv) };
    auto fragmentShaderModule{ createShaderModule(device, fragmentShaderSpv) };

    const std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStageCIs{
        vk::PipelineShaderStageCreateInfo{ /* flags */ {},
                                           /* stage */ vk::ShaderStageFlagBits::eVertex,
                                           vertexShaderModule,
                                           "main" },
        vk::PipelineShaderStageCreateInfo{ /* flags */ {},
                                           /* stage */ vk::ShaderStageFlagBits::eFragment,
                                           fragmentShaderModule,
                                           "main" }
    };

    // -- VERTEX INPUT

    // Vertex Binding Descriptions: Data spacing, stride.
    // Vertex Attribute Descriptions: Data format, where to bind to/from.
    const vk::PipelineVertexInputStateCreateInfo vertexInputStateCI{};

    // -- INPUT ASSEMBLY

    const vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCI{
        /* flags */ {},
        /* topology */ vk::PrimitiveTopology::eTriangleList,
        /* primitiveRestartEnable */ false
    };

    // -- VIEWPORT & SCISSOR

    const std::array<vk::Viewport, 1> viewports{ vk::Viewport{
        /* x */ 0,
        /* y */ 0,
        /* width */ Common::NarrowCast<float>(viewportSize.width),
        /* height */ Common::NarrowCast<float>(viewportSize.height),
        /* minDepth */ 0.0f,
        /* maxDepth */ 1.0f } };
    const std::array<vk::Rect2D, 1> scissors{ vk::Rect2D{ /* offset */ { 0, 0 }, /* extent */ viewportSize } };
    const vk::PipelineViewportStateCreateInfo viewportStateCI{ /* flags */ {},
                                                               /* viewports */ viewports,
                                                               /* scissors */ scissors };

    // -- DYNAMIC STATE

    // We don't use dynamic states for now.
    // This could be used for example to resize the viewport/scissor with a command
    // in the command buffer. For example, when the OS window is being resized.

    // -- RASTERIZER

    const vk::PipelineRasterizationStateCreateInfo rasterizationStateCI{
        /* flags */ {},
        /* depthClampEnable */ false,

        // The rasterizerDiscardEnable disables the fragment shader.
        // The fragment shader will not run. Fragments will not be created.
        // If you set this to true, then remember to request also the corrsponding GPU feature.
        /* rasterizerDiscardEnable */ false,

        // Other values than eFill also need a GPU feature.
        /* polygonMode */ vk::PolygonMode::eFill,

        /* cullMode */ vk::CullModeFlagBits::eBack,
        /* frontFace */ vk::FrontFace::eClockwise,

        // Whether to add depth bias to fragments. Good for stopping "shadow acne" in shadow mapping.
        /* depthBiasEnable */ false,
        /* depthBiasConstantFactor */ {},
        /* depthBiasClamp */ {},
        /* depthBiasSlopeFactor */ {},

        /* lineWidth */ 1.0f
    };

    // -- MULTISAMPLING

    // We don't use multisampling. It's created as disabled by default.
    const vk::PipelineMultisampleStateCreateInfo multisampleStateCI{
        /* flags */ {},
        /* rasterizationSamples */ vk::SampleCountFlagBits::e1,
        /* sampleShadingEnable */ false
    };

    // -- COLOR BLENDING

    // newDstColor = (srcColorBlendFactor * srcColor) colorBlendOp (dstColorBlendFactor * dstColor)
    // newDstColor = (srcAlpha * srcColor) + ((1 - srcAlpha) * dstColor)
    //
    // newDstAlpha = (srcAlphaBlendFactor * srcAlpha) alphaBlendOp (dstAlphaBlendFactor * dstAlpha)
    // newDstAlpha = (1 * srcAlpha) + (0 * dstAlpha)
    const std::array<vk::PipelineColorBlendAttachmentState, 1> colorBlendAttachmentStates{
        vk::PipelineColorBlendAttachmentState{ /* blendEnable */ true,
                                               /* srcColorBlendFactor */ vk::BlendFactor::eSrcAlpha,
                                               /* dstColorBlendFactor */ vk::BlendFactor::eOneMinusSrcAlpha,
                                               /* colorBlendOp */ vk::BlendOp::eAdd,
                                               /* srcAlphaBlendFactor */ vk::BlendFactor::eOne,
                                               /* dstAlphaBlendFactor */ vk::BlendFactor::eZero,
                                               /* alphaBlendOp */ vk::BlendOp::eAdd,
                                               /* colorWriteMask */ vk::ColorComponentFlagBits::eR |
                                                   vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
                                                   vk::ColorComponentFlagBits::eA }
    };

    const vk::PipelineColorBlendStateCreateInfo colorBlendStateCI{ /* flags */ {},
                                                                   /* logicOpEnable */ false,
                                                                   /* logicOp */ vk::LogicOp::eClear,
                                                                   /* pAttachments */ colorBlendAttachmentStates };

    // -- DEPTH STENCIL TESTING

    // TODO

    // == CREATE THE PIPELINE

    // We have to create a separate pipeline for each subpass of the render pass.
    // Here we create a pipeline for subpass 0.
    const vk::GraphicsPipelineCreateInfo gfxPipelineCI{
        /* flags */ {},
        /* stages */ shaderStageCIs,
        /* pVertexInputState */ &vertexInputStateCI,
        /* pInputAssemblyState */ &inputAssemblyStateCI,
        /* pTessellationState */ nullptr,
        /* pViewportState */ &viewportStateCI,
        /* pRasterizationState */ &rasterizationStateCI,
        /* pMultisampleState */ &multisampleStateCI,
        /* pDepthStencilState */ nullptr,
        /* pColorBlendState */ &colorBlendStateCI,
        /* pDynamicState */ nullptr,
        /* layout */ pipelineLayout,
        // Tell what kind of Render Pass this Pipeline is compatible with.
        // It's NOT going to store a reference to this specific Render Pass.
        /* renderPass */ renderPass,
        /* subpass */ 0
    };

    return device.createGraphicsPipeline(nullptr, gfxPipelineCI);
}

std::vector<vk::raii::Framebuffer> createFramebuffers(
    const vk::raii::Device& device, const Renderer::Detail::Swapchain& swapchain,
    const vk::raii::RenderPass& renderPass)
{
    std::vector<vk::raii::Framebuffer> framebuffers;
    framebuffers.reserve(swapchain.images.size());

    for (const auto& swapchainImage : swapchain.images)
    {
        // These attachments must match with the attachment descriptions of the Render Pass.
        const std::array<vk::ImageView, 1> imageViewAttachments{ swapchainImage.imageView };

        const vk::FramebufferCreateInfo framebufferCI{
            /* flags */ {},
            // Tell what kind of Render Pass this Framebuffer is compatible with.
            // It's NOT going to store a reference to this specific Render Pass.
            /* renderPass */ renderPass,
            /* pAttachments */ imageViewAttachments,
            /* width */ swapchain.imageExtent.width,
            /* height */ swapchain.imageExtent.height,
            /* layers */ 1
        };

        framebuffers.push_back(device.createFramebuffer(framebufferCI));
    }

    return framebuffers;
}

vk::raii::CommandPool createGraphicsCommandPool(const vk::raii::Device& device, std::uint32_t graphicsQueueFamilyIndex)
{
    const vk::CommandPoolCreateInfo commandPoolCI{ /* flags */ {},
                                                   /* queueFamilyIndex */ graphicsQueueFamilyIndex };
    return device.createCommandPool(commandPoolCI);
}

std::vector<vk::raii::CommandBuffer> createCommandBuffers(
    const vk::raii::Device& device, const vk::raii::CommandPool& commandPool, std::uint32_t count)
{
    const vk::CommandBufferAllocateInfo commandBufferAI{
        /* commandPool */ commandPool,
        // A Primary command buffer can be executed directly from a queue.
        // A Secondary command buffer can be executed only from a Primary command buffer.
        /* level */ vk::CommandBufferLevel::ePrimary,
        /* commandBufferCount */ count
    };
    return device.allocateCommandBuffers(commandBufferAI);
}

void recordCommands(
    std::span<const vk::raii::CommandBuffer> commandBuffers, const vk::raii::RenderPass& renderPass,
    std::span<const vk::raii::Framebuffer> framebuffers, const vk::Extent2D& swapchainImageExtent,
    const vk::raii::Pipeline& pipeline)
{
    assert(commandBuffers.size() == framebuffers.size());

    const vk::CommandBufferBeginInfo cmdBufferBI{
        // eSimultaneousUse means this command buffer may be in the queue (submitted) multiple times.
        // We don't set that because only one instance of this command buffer will be in the queue at once.
        // We use fences to ensure that (i.e. m_drawFence).
        /* flags */ vk::CommandBufferUsageFlags{}
    };

    const std::array<vk::ClearValue, 1> clearValues{ // Clear value for the color attachment.
                                                     vk::ClearValue{ vk::ClearColorValue{ 0.5f, 0.5f, 0.5f, 0.5f } }
    };

    // We have a separate Command Buffer for each Framebuffer / Swapchain Image.
    for (auto i = 0u; i != commandBuffers.size(); ++i)
    {
        auto& commandBuffer{ commandBuffers[i] };
        commandBuffer.begin(cmdBufferBI);

        {
            commandBuffer.beginRenderPass(
                vk::RenderPassBeginInfo{ /* renderPass */ renderPass,
                                         /* framebuffer */ framebuffers[i],
                                         /* renderArea */ vk::Rect2D{ vk::Offset2D{ 0, 0 }, swapchainImageExtent },
                                         /* pClearValues */ clearValues },
                // eInline specifies that the contents of the subpasses will be recorded inline in the primary command
                // buffer, and secondary command buffers must not be executed within the subpass.
                vk::SubpassContents::eInline);

            commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

            commandBuffer.draw(3, 1, 0, 0);

            commandBuffer.endRenderPass();
        }

        commandBuffer.end();
    }
}

std::vector<vk::raii::Semaphore> createSemaphores(const vk::raii::Device& device, std::size_t count)
{
    const vk::SemaphoreCreateInfo semaphoreCI{};
    std::vector<vk::raii::Semaphore> semaphores{};
    semaphores.reserve(count);
    for (auto i{ 0u }; i != count; ++i)
    {
        semaphores.emplace_back(device.createSemaphore(semaphoreCI));
    }
    return semaphores;
}

std::vector<vk::raii::Fence> createFences(const vk::raii::Device& device, std::size_t count)
{
    const vk::FenceCreateInfo fenceCI{ /* flags */ vk::FenceCreateFlagBits::eSignaled };
    std::vector<vk::raii::Fence> fences{};
    fences.reserve(count);
    for (auto i{ 0u }; i != count; ++i)
    {
        fences.emplace_back(device.createFence(fenceCI));
    }
    return fences;
}

} // namespace

namespace VkTest1::Renderer::Detail
{

VulkanRenderer::VulkanRenderer(
    Common::NotNull<Common::IFileSystem*> fileSystem, Common::NotNull<Window::IWindow*> window) :
    m_fileSystem{ fileSystem },
    m_window{ window },
    m_instance{ createInstance(m_context, *m_window) },
    m_debugMessenger{ createDebugMessenger(m_instance) },
    m_surface{ vk::raii::SurfaceKHR{ m_instance,
                                     static_cast<VkSurfaceKHR>(m_window->createSurface(m_instance.operator*())) } },
    m_physicalDevice{ getPhysicalDevice(m_instance, m_surface) },
    m_device{ createLogicalDevice(m_physicalDevice) },
    m_swapchain{ createSwapchain(*m_window, m_surface, m_physicalDevice, m_device) },
    m_graphicsQueue{ m_device.getQueue(
        m_physicalDevice.queueFamilyInfo.graphicsQueueFamilyIndex.value(), /* queueIndex */ 0) },
    m_presentationQueue{ m_device.getQueue(
        m_physicalDevice.queueFamilyInfo.presentationQueueFamilyIndex.value(), /* queueIndex */ 0) },
    m_renderPass{ createRenderPass(m_device, m_swapchain.imageFormat) },
    m_pipelineLayout{ createPipelineLayout(m_device) },
    m_pipeline{ createPipeline(*m_fileSystem, m_device, m_swapchain.imageExtent, m_renderPass, m_pipelineLayout) },
    m_framebuffers{ createFramebuffers(m_device, m_swapchain, m_renderPass) },
    m_graphicsCommandPool{ createGraphicsCommandPool(
        m_device, m_physicalDevice.queueFamilyInfo.graphicsQueueFamilyIndex.value()) },
    m_commandBuffers{ createCommandBuffers(m_device, m_graphicsCommandPool, m_swapchain.images.size()) },
    m_imageAvailable{ createSemaphores(m_device, s_maxFrameCountInQueue) },
    m_renderFinished{ createSemaphores(m_device, s_maxFrameCountInQueue) },
    m_drawFence{ createFences(m_device, s_maxFrameCountInQueue) }
{
    printPhysicalDeviceInfo(m_physicalDevice.device);
    recordCommands(m_commandBuffers, m_renderPass, m_framebuffers, m_swapchain.imageExtent, m_pipeline);
}

VulkanRenderer::~VulkanRenderer()
{
    // Wait for all the work to finish before destroying Vulkan objects.
    m_device.waitIdle();
}

void VulkanRenderer::draw()
{
    //
    // In the queue, we allow only s_maxFrameCountInQueue frames at once.
    // We start rendering the frame only if its fence is open.
    //
    // Rendering frame 0:
    //
    // 1. Sync: Wait for fence F0.
    // 2. Async: Acquire a swapchain image.
    //    - The image will be available only when the pipeline is already running (signaled via the imageAvailable
    //      semaphore).
    // 3. Async: Submit the command buffer.
    //    - Starts the render pass immediately.
    //    - Sometime during the render pass, it will signal F0 and renderFinished.
    // 4. Async: Present the image.
    //    - will happen only when renderFinished.
    //
    // |----[______________F0]-------------------------------[____________F1]----------->
    //        |                                                |
    //        RenderPass                                       RenderPass
    //          |
    //          SubPass
    //            |
    //            Pipeline
    //              |   +-- ...
    //              |   +-- ColorOutputStage --wait-for--+
    //              |                                    |
    //              Framebuffer_0                        |
    //                |                                  |
    //                Swapchain_Image_0 <-imageAvailable-+
    //

    // -- RATE LIMIT

    // Wait for fence.
    const std::array<vk::Fence, 1> fences{ m_drawFence[m_currentFrame] };
    auto result{ m_device.waitForFences(fences, true, std::numeric_limits<uint64_t>::max()) };
    if (result != vk::Result::eSuccess)
    {
        throw Common::RendererError{ "Cannot wait for fences." };
    }
    m_device.resetFences(fences);

    // -- REQUEST SWAPCHAIN IMAGE

    const auto imageIndexResult{ m_device.acquireNextImage2KHR(
        vk::AcquireNextImageInfoKHR{ /* swapchain */ m_swapchain.swapchain,
                                     /* timeout */ std::numeric_limits<std::uint64_t>::max(),
                                     /* semaphore */ m_imageAvailable[m_currentFrame],
                                     /* fence */ {},
                                     /* deviceMask */ 1 /*1u << m_physicalDevice.deviceIndex*/ }) };
    if (imageIndexResult.first != vk::Result::eSuccess)
    {
        throw Common::RendererError{ "Cannot acquire next image from swapchain." };
    }
    const auto imageIndex{ imageIndexResult.second };

    // -- SUBMIT COMMAND BUFFER

    // Let the pipeline run until it reaches the Color Attachment Output stage.
    // At that point, it has to wait for the "image available" signal before continuing.
    const std::array<vk::Semaphore, 1> waitSemaphores{ m_imageAvailable[m_currentFrame] };
    const std::array<vk::PipelineStageFlags, 1> waitStageFlags{ vk::PipelineStageFlagBits::eColorAttachmentOutput };

    const std::array<vk::CommandBuffer, 1> commandBuffers{ m_commandBuffers[imageIndex] };

    // After the command buffer has finished execution, we ask it to signal "render finished".
    const std::array<vk::Semaphore, 1> signalSemaphores{ m_renderFinished[m_currentFrame] };

    m_graphicsQueue.submit(
        std::array<vk::SubmitInfo, 1>{ vk::SubmitInfo{ /* pWaitSemaphores */ waitSemaphores,
                                                       /* pWaitDstStageMask */ waitStageFlags,
                                                       /* pCommandBuffers */ commandBuffers,
                                                       /* pSignalSemaphores */ signalSemaphores } },
        m_drawFence[m_currentFrame]);

    // -- REQUEST PRESENT IMAGE

    const std::array<vk::SwapchainKHR, 1> swapchains{ m_swapchain.swapchain };
    const std::array<std::uint32_t, 1> imageIndices{ imageIndex };
    result = m_graphicsQueue.presentKHR(vk::PresentInfoKHR{ // Wait for the "render finished" signal before presenting.
                                                            /* pWaitSemaphores */ signalSemaphores,
                                                            /* pSwapchains */ swapchains,
                                                            /* pImageIndices */ imageIndices });
    if (result != vk::Result::eSuccess)
    {
        throw Common::RendererError{ "Cannot present image." };
    }

    m_currentFrame = (m_currentFrame + 1) % s_maxFrameCountInQueue;
}

} // namespace VkTest1::Renderer::Detail
