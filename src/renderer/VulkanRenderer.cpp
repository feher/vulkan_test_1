#include "renderer/VulkanRenderer.hpp"

#include "common/Cast.hpp"
#include "common/Errors.hpp"
#include "renderer/DebugUtilsMessenger.hpp"

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <print>
#include <span>
#include <unordered_set>

using namespace VkTest1;

namespace
{

const std::array<const char* const, 1> s_requiredInstanceLayers{ "VK_LAYER_KHRONOS_validation" };
const std::array<const char* const, 1> s_requiredPhysicalDeviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

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
    for (const auto& physicalDevice : physicalDevices)
    {
        const auto queueFamilyInfo = getQueueFamilyInfo(physicalDevice, surface);
        if (queueFamilyInfo.graphicsQueueFamilyIndex.has_value() &&
            queueFamilyInfo.presentationQueueFamilyIndex.has_value() &&
            arePhysicalDeviceExtensionsSupported(physicalDevice, s_requiredPhysicalDeviceExtensions) &&
            !physicalDevice.getSurfacePresentModesKHR(surface).empty() &&
            !physicalDevice.getSurfaceFormatsKHR(surface).empty())
        {
            return { physicalDevice, queueFamilyInfo };
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

} // namespace

namespace VkTest1::Renderer::Detail
{

VulkanRenderer::VulkanRenderer(Common::NotNull<Window::IWindow*> window) :
    m_window{ window },
    m_instance{ createInstance(m_context, *m_window) },
    m_surface{ vk::raii::SurfaceKHR{ m_instance,
                                     static_cast<VkSurfaceKHR>(m_window->createSurface(m_instance.operator*())) } },
    m_debugMessenger{ createDebugMessenger(m_instance) },
    m_physicalDevice{ getPhysicalDevice(m_instance, m_surface) },
    m_device{ createLogicalDevice(m_physicalDevice) },
    m_graphicsQueue{ m_device.getQueue(
        m_physicalDevice.queueFamilyInfo.graphicsQueueFamilyIndex.value(), /* queueIndex */ 0) },
    m_presentationQueue{ m_device.getQueue(
        m_physicalDevice.queueFamilyInfo.presentationQueueFamilyIndex.value(), /* queueIndex */ 0) }
{
    printPhysicalDeviceInfo(m_physicalDevice.device);
}

VulkanRenderer::~VulkanRenderer()
{
}

} // namespace VkTest1::Renderer::Detail
