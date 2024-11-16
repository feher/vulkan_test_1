#include "renderer/DebugUtilsMessenger.hpp"

#include "common/Errors.hpp"

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pMessenger)
{
    return VkTest1::Renderer::Details::g_VkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pMessenger);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(
    VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* pAllocator)
{
    return VkTest1::Renderer::Details::g_VkDestroyDebugUtilsMessengerEXT(instance, messenger, pAllocator);
}

namespace VkTest1::Renderer::Details
{

PFN_vkCreateDebugUtilsMessengerEXT g_VkCreateDebugUtilsMessengerEXT{ nullptr };
PFN_vkDestroyDebugUtilsMessengerEXT g_VkDestroyDebugUtilsMessengerEXT{ nullptr };

void initDebugUtilsMessengerExtension(const vk::raii::Instance& instance)
{
    if (g_VkCreateDebugUtilsMessengerEXT && g_VkDestroyDebugUtilsMessengerEXT)
    {
        return;
    }

    g_VkCreateDebugUtilsMessengerEXT =
        reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(instance.getProcAddr("vkCreateDebugUtilsMessengerEXT"));
    if (!g_VkCreateDebugUtilsMessengerEXT)
    {
        throw Common::RendererError{ "GetInstanceProcAddr: Unable to find vkCreateDebugUtilsMessengerEXT function." };
    }

    g_VkDestroyDebugUtilsMessengerEXT =
        reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(instance.getProcAddr("vkDestroyDebugUtilsMessengerEXT"));
    if (!g_VkDestroyDebugUtilsMessengerEXT)
    {
        throw Common::RendererError{ "GetInstanceProcAddr: Unable to find vkDestroyDebugUtilsMessengerEXT function." };
    }
}

} // namespace VkTest1::Renderer::Details
