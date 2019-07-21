#pragma once

#include <vulkan/vulkan.h>
#include <iostream>
#include <memory>
#include <optional>
#include <vector>
#include "platform.h"

namespace gfx::vk_api {

#define vk_function_definition(fun) inline PFN_##fun fun
#define vk_device_function_definition(fun) PFN_##fun fun

// Vulkan functions definitions.
// ************************************************************ //
// Exported functions                                           //
//                                                              //
// These functions are always exposed by vulkan libraries.      //
// ************************************************************ //
vk_function_definition(vkGetInstanceProcAddr);
// ************************************************************ //
// Global level functions                                       //
//                                                              //
// They allow checking what instance extensions are available   //
// and allow creation of a Vulkan Instance.                     //
// ************************************************************ //
vk_function_definition(vkCreateInstance);
vk_function_definition(vkEnumerateInstanceExtensionProperties);

// ************************************************************ //
// Instance level functions                                     //
//                                                              //
// These functions allow for device queries and creation.       //
// They help choose which device is well suited for our needs.  //
// ************************************************************ //
vk_function_definition(vkEnumeratePhysicalDevices);
vk_function_definition(vkGetPhysicalDeviceProperties);
vk_function_definition(vkGetPhysicalDeviceFeatures);
vk_function_definition(vkGetPhysicalDeviceQueueFamilyProperties);
vk_function_definition(vkCreateDevice);
vk_function_definition(vkGetDeviceProcAddr);
vk_function_definition(vkDestroyInstance);
vk_function_definition(vkEnumerateDeviceExtensionProperties);
// Swap chain extensions.
vk_function_definition(vkGetPhysicalDeviceSurfaceSupportKHR);
vk_function_definition(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
vk_function_definition(vkGetPhysicalDeviceSurfaceFormatsKHR);
vk_function_definition(vkGetPhysicalDeviceSurfacePresentModesKHR);
vk_function_definition(vkDestroySurfaceKHR);
#if defined(VK_USE_PLATFORM_WIN32_KHR)
vk_function_definition(vkCreateWin32SurfaceKHR);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
vk_function_definition(vkCreateXcbSurfaceKHR);
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
vk_function_definition(vkCreateXlibSurfaceKHR);
#endif

struct QueueFamilyIndices {
  std::optional<uint32_t> graphics_family;
  std::optional<uint32_t> present_family;

  bool is_complete()
  {
    return graphics_family.has_value() && present_family.has_value();
  }
};

struct VulkanDevice {
  VkPhysicalDevice physical_device;
  VkDevice logical_device;
  VkQueue graphics_queue;
  VkQueue present_queue;
  VkSurfaceKHR surface;
  VkSemaphore image_available_semaphore;
  VkSemaphore rendering_finished_semaphore;
  VkSwapchainKHR swap_chain;

  // ************************************************************ //
  // Device level functions                                       //
  //                                                              //
  // These functions are used mainly for drawing                  //
  // ************************************************************ //
  vk_device_function_definition(vkGetDeviceQueue);
  vk_device_function_definition(vkDeviceWaitIdle);
  vk_device_function_definition(vkDestroyDevice);

  vk_device_function_definition(vkCreateSemaphore);
  vk_device_function_definition(vkCreateCommandPool);
  vk_device_function_definition(vkAllocateCommandBuffers);
  vk_device_function_definition(vkBeginCommandBuffer);
  vk_device_function_definition(vkCmdPipelineBarrier);
  vk_device_function_definition(vkCmdClearColorImage);
  vk_device_function_definition(vkEndCommandBuffer);
  vk_device_function_definition(vkQueueSubmit);
  vk_device_function_definition(vkFreeCommandBuffers);
  vk_device_function_definition(vkDestroyCommandPool);
  vk_device_function_definition(vkDestroySemaphore);
  // Swap chain extensions.
  vk_device_function_definition(vkCreateSwapchainKHR);
  vk_device_function_definition(vkGetSwapchainImagesKHR);
  vk_device_function_definition(vkAcquireNextImageKHR);
  vk_device_function_definition(vkQueuePresentKHR);
  vk_device_function_definition(vkDestroySwapchainKHR);
};

// Api.
auto initialize() -> void;
auto destroy() -> void;
auto create_device(const os::WindowParameters& window) -> VulkanDevice;
auto is_physical_device_suitable_for_surface(VkPhysicalDevice device,
                                             VkSurfaceKHR surface) -> bool;
auto check_physical_device_extension_support(VkPhysicalDevice device) -> bool;
auto enumerate_all_physical_devices() -> void;
auto pick_best_physical_device_for_surface(VkSurfaceKHR surface)
    -> VkPhysicalDevice;
auto rate_physical_device_suitability(VkPhysicalDevice device,
                                      VkSurfaceKHR surface) -> int;
auto find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface)
    -> QueueFamilyIndices;
auto check_extension_availability(
    const char* extension_name,
    const std::vector<VkExtensionProperties>& available_extensions) -> bool;
auto create_window_surface(os::WindowParameters window) -> VkSurfaceKHR;
auto create_semaphore(VulkanDevice& device) -> VkSemaphore;
auto create_device_swap_chain(VulkanDevice& device) -> void;
auto get_swap_chain_num_images(VkSurfaceCapabilitiesKHR& surface_capabilities)
    -> uint32_t;
auto get_swap_chain_format(std::vector<VkSurfaceFormatKHR>& surface_formats)
    -> VkSurfaceFormatKHR;
auto get_swap_chain_extent(VkSurfaceCapabilitiesKHR& surface_capabilities)
    -> VkExtent2D;
auto get_swap_chain_usage_flags(VkSurfaceCapabilitiesKHR& surface_capabilities)
    -> VkImageUsageFlags;
auto get_swap_chain_transform(VkSurfaceCapabilitiesKHR& surface_capabilities)
    -> VkSurfaceTransformFlagBitsKHR;
auto get_swap_chain_present_mode(std::vector<VkPresentModeKHR>& present_modes)
    -> VkPresentModeKHR;

}  // namespace gfx::vk_api

#undef vk_function_definition

namespace gfx {

class Device {
 public:
  template <typename T>
  Device(T device) : self_(std::make_unique<Model<T>>(device))
  {
  }

  friend auto print_device_name(const Device& device) -> void;
  friend auto destroy_device(const Device& device) -> void;
  friend auto create_swap_chain(const Device& device) -> void;

 private:
  struct Concept {
    virtual ~Concept() = default;
    virtual auto print_name_() -> void = 0;
    virtual auto destroy_() -> void = 0;
    virtual auto create_swap_chain_() -> void = 0;
  };

  template <typename T>
  struct Model : Concept {
    Model() = delete;
    Model(T user_model) : user_model_(user_model) {}

    auto print_name_() -> void override { print_device_name(user_model_); }
    auto destroy_() -> void override { destroy_device(user_model_); }
    auto create_swap_chain_() -> void override
    {
      create_swap_chain(user_model_);
    }

    T user_model_;
  };

  std::unique_ptr<Concept> self_;
};

// api.

auto load_backend() -> void;
auto unload_backend() -> void;
auto create_device(const os::WindowParameters& window) -> Device;

}  // namespace gfx

namespace gfx {

auto print_device_name(vk_api::VulkanDevice device) -> void;
auto destroy_device(vk_api::VulkanDevice device) -> void;

}  // namespace gfx
