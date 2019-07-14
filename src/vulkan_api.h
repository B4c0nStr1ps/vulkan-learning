#pragma once

#include <vulkan/vulkan.h>
#include <iostream>
#include <memory>
#include <optional>

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

// ************************************************************ //
// Global level functions                                       //
//                                                              //
// They allow checking what instance extensions are available   //
// and allow creation of a Vulkan Instance.                     //
// ************************************************************ //
vk_function_definition(vkEnumeratePhysicalDevices);
vk_function_definition(vkGetPhysicalDeviceProperties);
vk_function_definition(vkGetPhysicalDeviceFeatures);
vk_function_definition(vkGetPhysicalDeviceQueueFamilyProperties);
vk_function_definition(vkCreateDevice);
vk_function_definition(vkGetDeviceProcAddr);
vk_function_definition(vkDestroyInstance);

struct QueueFamilyIndices {
  std::optional<uint32_t> graphics_family;

  bool is_complete() { return graphics_family.has_value(); }
};

struct VulkanDevice {
  VkPhysicalDevice physical_device;
  VkDevice logical_device;
  VkQueue graphics_queue;

  // ************************************************************ //
  // Device level functions                                       //
  //                                                              //
  // These functions are used mainly for drawing                  //
  // ************************************************************ //
  vk_device_function_definition(vkGetDeviceQueue);
  vk_device_function_definition(vkDeviceWaitIdle);
  vk_device_function_definition(vkDestroyDevice);
};

// Api.
auto initialize() -> void;
auto destroy() -> void;
auto create_device() -> VulkanDevice;
auto is_physical_device_suitable(VkPhysicalDevice device) -> bool;
auto enumerate_all_physical_devices() -> void;
auto pick_best_physical_device() -> VkPhysicalDevice;
auto rate_physical_device_suitability(VkPhysicalDevice device) -> int;
auto find_queue_families(VkPhysicalDevice device) -> QueueFamilyIndices;

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

 private:
  struct Concept {
    virtual ~Concept() = default;
    virtual auto print_name_() -> void = 0;
    virtual auto destroy_() -> void = 0;
  };

  template <typename T>
  struct Model : Concept {
    Model() = delete;
    Model(T user_model) : user_model_(user_model) {}

    auto print_name_() -> void override { print_device_name(user_model_); }
    auto destroy_() -> void override { destroy_device(user_model_); }

    T user_model_;
  };

  std::unique_ptr<Concept> self_;
};

// api.
auto load_backend() -> void;
auto unload_backend() -> void;
auto create_device() -> Device;

}  // namespace gfx

namespace gfx {

auto print_device_name(vk_api::VulkanDevice device) -> void;
auto destroy_device(vk_api::VulkanDevice device) -> void;

}  // namespace gfx
