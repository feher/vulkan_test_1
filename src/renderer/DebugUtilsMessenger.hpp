#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_raii.hpp>

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pMessenger);

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(
    VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* pAllocator);

namespace VkTest1::Renderer::Details
{

extern PFN_vkCreateDebugUtilsMessengerEXT g_VkCreateDebugUtilsMessengerEXT;
extern PFN_vkDestroyDebugUtilsMessengerEXT g_VkDestroyDebugUtilsMessengerEXT;

/// <summary>
/// Initializes the Debug Utils Messenger extension.
///
/// <para>
/// It initializes the function pointers to
/// <list>
///     <item>vkCreateDebugUtilsMessengerEXT and</item>
///     <item>vkDestroyDebugUtilsMessengerEXT.</item>
/// </list>
/// </para>
///
/// </summary>
/// <param name="instance"></param>
void initDebugUtilsMessengerExtension(const vk::raii::Instance& instance);

} // namespace VkTest1::Renderer::Details
