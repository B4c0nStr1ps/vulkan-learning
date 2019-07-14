#include "vulkan_api.h"
#include <Windows.h>
#include <map>
#include <vector>

#define load_proc_address GetProcAddress

#define vk_load_exported_function(fun)                               \
  if (!(fun = (PFN_##fun)load_proc_address(VULKAN_LIBRARY, #fun))) { \
    std::cerr << "Could not load exported function: " << #fun << "!" \
              << std::endl;                                          \
    std::terminate();                                                \
  }

#define vk_global_level_function(fun)                                    \
  if (!(fun = (PFN_##fun)vkGetInstanceProcAddr(nullptr, #fun))) {        \
    std::cerr << "Could not load global level function: " << #fun << "!" \
              << std::endl;                                              \
    std::terminate();                                                    \
  }

#define vk_instance_level_function(fun)                                    \
  if (!(fun = (PFN_##fun)vkGetInstanceProcAddr(VK_INSTANCE, #fun))) {      \
    std::cerr << "Could not load instance level function: " << #fun << "!" \
              << std::endl;                                                \
    std::terminate();                                                      \
  }

namespace gfx::vk_api {

// Vulkan dynamic library handle.
typedef HMODULE LibraryHandle;
LibraryHandle VULKAN_LIBRARY;
// Vulkan instance definition.
VkInstance VK_INSTANCE;

}  // namespace gfx::vk_api

// auto gfx::print_device_name(VkPhysicalDevice device) -> void
//{
//  VkPhysicalDeviceProperties device_properties;
//  vkGetPhysicalDeviceProperties(device, &device_properties);
//  std::cout << "Device name: " << device_properties.deviceName << "\n";
//}

auto gfx::print_device_name(const gfx::Device& device) -> void
{
  device.self_->print_name_();
}

auto gfx::vk_api::initialize() -> void
{
  // Step 1: Load Vulkan library:
  VULKAN_LIBRARY = LoadLibrary("vulkan-1.dll");
  if (VULKAN_LIBRARY == nullptr) {
    std::cerr << "Could not load Vulkan library!\n";
    std::terminate();
  }
  std::cout << "Vulkan library loaded.\n";

  // Step 2: Load the exported entry point.
  vk_load_exported_function(vkGetInstanceProcAddr);
  std::cout << "Vulkan exported entry point loaded.\n";

  // Step 3: Load global level entry points.
  vk_global_level_function(vkCreateInstance);
  std::cout << "Vulkan global level entry points loaded.\n";

  // Step 4: Create the Vulkan Instance.
  // The Vulkan Instance stores all per-application states.

  // This data is technically optional, but it may provide some useful
  // information to the driver to optimize for our specific application, for
  // example because it uses a well-known graphics engine with certain special
  // behavior.
  VkApplicationInfo application_info = {
      VK_STRUCTURE_TYPE_APPLICATION_INFO,  // sType
      nullptr,                             // *pNext
      "vulkan-learning",                   // *pApplicationName
      VK_MAKE_VERSION(1, 0, 0),            // applicationVersion
      "No Engine",                         // *pEngineName
      VK_MAKE_VERSION(1, 0, 0),            // engineVersion
      VK_MAKE_VERSION(1, 0, 0)             // apiVersion
  };

  // This struct is not optional and tells the Vulkan driver which global
  // extensions and validation layers we want to use. Global here means that
  // they apply to the entire program and not a specific device.
  VkInstanceCreateInfo instance_create_info = {
      VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,  // sType
      nullptr,                                 //  pNext
      0,                                       //  flags
      &application_info,                       // *pApplicationInfo
      0,                                       //  enabledLayerCount
      nullptr,                                 // *ppEnabledLayerNames
      0,                                       //  enabledExtensionCount
      nullptr                                  // *ppEnabledExtensionNames
  };

  // Try create the vulkan instance.
  if (vkCreateInstance(&instance_create_info, nullptr, &VK_INSTANCE) !=
      VK_SUCCESS) {
    std::cerr << "Could not create Vulkan instance!" << std::endl;
    std::terminate();
  }
  std::cout << "Vulkan Instance created.\n";

  // Step 5: Load instance level entry points.
  vk_instance_level_function(vkEnumeratePhysicalDevices);
  vk_instance_level_function(vkGetPhysicalDeviceProperties);
  vk_instance_level_function(vkGetPhysicalDeviceFeatures);
  vk_instance_level_function(vkGetPhysicalDeviceQueueFamilyProperties);
  vk_instance_level_function(vkCreateDevice);
  vk_instance_level_function(vkGetDeviceProcAddr);
  vk_instance_level_function(vkDestroyInstance);
  std::cout << "Vulkan instance level entry points loaded.\n";

  std::cout << "Vulkan api initialized.\n";
}

auto gfx::vk_api::destroy() -> void { vkDestroyInstance(VK_INSTANCE, nullptr); }

auto gfx::vk_api::create_device() -> VkPhysicalDevice
{
  VkPhysicalDevice physical_device = pick_best_physical_device();
  return physical_device;
}

auto gfx::vk_api::pick_best_physical_device() -> VkPhysicalDevice
{
  VkPhysicalDevice physical_device = VK_NULL_HANDLE;

  uint32_t device_count = 0;
  if (vkEnumeratePhysicalDevices(VK_INSTANCE, &device_count, nullptr) !=
      VK_SUCCESS) {
    std::cerr << "Error occurred during physical devices enumeration!"
              << std::endl;
    std::terminate();
  }
  if (device_count == 0) {
    std::cerr << "failed to find GPUs with Vulkan support!\n";
    std::terminate();
  }

  std::vector<VkPhysicalDevice> devices(device_count);
  vkEnumeratePhysicalDevices(VK_INSTANCE, &device_count, devices.data());

  if ((vkEnumeratePhysicalDevices(VK_INSTANCE, &device_count, nullptr) !=
       VK_SUCCESS) ||
      (device_count == 0)) {
    std::cerr << "Error occurred during physical devices enumeration!"
              << std::endl;
    std::terminate();
  }

  // Use an ordered map to automatically sort candidates by increasing score
  std::multimap<int, VkPhysicalDevice> candidates;

  for (const auto& device : devices) {
    int score = rate_physical_device_suitability(device);
    candidates.insert(std::make_pair(score, device));
  }
  // Check if the best candidate is suitable at all
  if (candidates.rbegin()->first > 0) {
    physical_device = candidates.rbegin()->second;
  }
  else {
    std::cerr << "Failed to find a suitable GPU!" << std::endl;
    std::terminate();
  }

  return physical_device;
}

auto gfx::vk_api::rate_physical_device_suitability(VkPhysicalDevice device)
    -> int
{
  if (!is_physical_device_suitable(device)) return 0;

  VkPhysicalDeviceProperties device_properties;
  VkPhysicalDeviceFeatures device_features;
  vkGetPhysicalDeviceProperties(device, &device_properties);
  vkGetPhysicalDeviceFeatures(device, &device_features);

  int score = 0;

  // Discrete GPUs have a significant performance advantage
  if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
    score += 1000;
  }
  // Maximum possible size of textures affects graphics quality
  score += device_properties.limits.maxImageDimension2D;
  // Application can't function without geometry shaders
  if (!device_features.geometryShader) {
    return 0;
  }

  return score;
}

auto gfx::vk_api::is_physical_device_suitable(VkPhysicalDevice device) -> bool
{
  QueueFamilyIndices indices = find_queue_families(device);
  return indices.is_complete();
}

auto gfx::vk_api::find_queue_families(VkPhysicalDevice device)
    -> QueueFamilyIndices
{
  QueueFamilyIndices indices;

  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count,
                                           nullptr);

  std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count,
                                           queue_families.data());

  int i = 0;
  for (const auto& queueFamily : queue_families) {
    if (queueFamily.queueCount > 0 &&
        queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphics_family = i;
    }

    if (indices.is_complete()) {
      break;
    }

    i++;
  }

  return indices;
}

auto gfx::vk_api::enumerate_all_physical_devices() -> void
{
  uint32_t device_count = 0;
  if (vkEnumeratePhysicalDevices(VK_INSTANCE, &device_count, nullptr) !=
      VK_SUCCESS) {
    std::cerr << "Error occurred during physical devices enumeration!"
              << std::endl;
    return;
  }
  std::vector<VkPhysicalDevice> devices(device_count);
  vkEnumeratePhysicalDevices(VK_INSTANCE, &device_count, devices.data());

  if ((vkEnumeratePhysicalDevices(VK_INSTANCE, &device_count, nullptr) !=
       VK_SUCCESS) ||
      (device_count == 0)) {
    return;
  }

  for (const auto& device : devices) {
    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(device, &device_properties);
    std::cout << "Device name: " << device_properties.deviceName << "\n";
  }
}

auto gfx::load_backend() -> void { vk_api::initialize(); }

auto gfx::unload_backend() -> void { vk_api::destroy(); }

auto gfx::create_device() -> Device { return vk_api::create_device(); }

auto gfx::print_device_name(VkPhysicalDevice device) -> void
{
  // std::cout << "Device\n";
  VkPhysicalDeviceProperties device_properties;
  vk_api::vkGetPhysicalDeviceProperties(device, &device_properties);
  std::cout << "Device name: " << device_properties.deviceName << "\n";
}
