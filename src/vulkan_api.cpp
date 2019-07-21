#include "vulkan_api.h"
#include <Windows.h>
#include <map>
#include <set>

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

const std::vector<const char*> DEVICE_EXTENSIONS = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

}  // namespace gfx::vk_api

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
  vk_global_level_function(vkEnumerateInstanceExtensionProperties);
  std::cout << "Vulkan global level entry points loaded.\n";

  // Step 4: Checking Whether an Instance Extension Is Supported.

  uint32_t extensions_count = 0;
  if ((vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count,
                                              nullptr) != VK_SUCCESS) ||
      (extensions_count == 0)) {
    std::cerr << "Error occurred during instance extensions enumeration!"
              << std::endl;
    std::terminate();
  }
  std::vector<VkExtensionProperties> available_extensions(extensions_count);
  if (vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count,
                                             available_extensions.data()) !=
      VK_SUCCESS) {
    std::cerr << "Error occurred during instance extensions enumeration!"
              << std::endl;
    std::terminate();
  }

  std::vector<const char*> extensions = {
    VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#elif defined(VK_USE_PLATFORM_XCB_KHR)
    VK_KHR_XCB_SURFACE_EXTENSION_NAME
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
    VK_KHR_XLIB_SURFACE_EXTENSION_NAME
#endif
  };

  for (size_t i = 0; i < extensions.size(); ++i) {
    if (!check_extension_availability(extensions[i], available_extensions)) {
      std::cerr << "Could not find instance extension named \"" << extensions[i]
                << "\"!" << std::endl;
      std::terminate();
    }
  }

  // Step 5: Create the Vulkan Instance.
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
      VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,    // sType
      nullptr,                                   //  pNext
      0,                                         //  flags
      &application_info,                         // *pApplicationInfo
      0,                                         //  enabledLayerCount
      nullptr,                                   // *ppEnabledLayerNames
      static_cast<uint32_t>(extensions.size()),  //  enabledExtensionCount
      &extensions[0]                             // *ppEnabledExtensionNames
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
  vk_instance_level_function(vkEnumerateDeviceExtensionProperties);
  // Swap chain extensions functions.
  vk_instance_level_function(vkGetPhysicalDeviceSurfaceSupportKHR);
  vk_instance_level_function(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
  vk_instance_level_function(vkGetPhysicalDeviceSurfaceFormatsKHR);
  vk_instance_level_function(vkGetPhysicalDeviceSurfacePresentModesKHR);
  vk_instance_level_function(vkDestroySurfaceKHR);
#if defined(VK_USE_PLATFORM_WIN32_KHR)
  vk_instance_level_function(vkCreateWin32SurfaceKHR);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
  vk_instance_level_function(vkCreateXcbSurfaceKHR);
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
  vk_instance_level_function(vkCreateXlibSurfaceKHR);
#endif

  std::cout << "Vulkan instance level entry points loaded.\n";

  std::cout << "Vulkan api initialized.\n";
}

auto gfx::vk_api::destroy() -> void { vkDestroyInstance(VK_INSTANCE, nullptr); }

auto gfx::vk_api::create_device(const os::WindowParameters& window)
    -> VulkanDevice
{
  VulkanDevice device = {};

  // Step 1: create the surface.
  VkSurfaceKHR surface = create_window_surface(window);
  device.surface = surface;

  // Step 2: pick the most suitable physical device.
  device.physical_device = pick_best_physical_device_for_surface(surface);

  // Step 3: create the logical device.

  QueueFamilyIndices indices =
      find_queue_families(device.physical_device, surface);
  float queue_priority = 1.0f;

  // Create a set of all unique queue families that are necessary for the
  // required queues.
  std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
  std::set<uint32_t> unique_queue_families = {indices.graphics_family.value(),
                                              indices.present_family.value()};

  for (uint32_t queueFamily : unique_queue_families) {
    VkDeviceQueueCreateInfo queue_create_info = {};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = queueFamily;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priority;
    queue_create_infos.push_back(queue_create_info);
  }

  // Specifying used device features.
  VkPhysicalDeviceFeatures device_features = {};

  // Creating the logical device.
  VkDeviceCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info.queueCreateInfoCount =
      static_cast<uint32_t>(queue_create_infos.size());
  create_info.pQueueCreateInfos = queue_create_infos.data();
  create_info.pEnabledFeatures = &device_features;
  create_info.enabledExtensionCount =
      static_cast<uint32_t>(DEVICE_EXTENSIONS.size());
  create_info.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();

  if (vkCreateDevice(device.physical_device, &create_info, nullptr,
                     &device.logical_device) != VK_SUCCESS) {
    std::cerr << "failed to create logical device!\n";
    std::terminate();
  }

  // Load Device-Level functions.

#define vk_device_level_function(fun)                                       \
  if (!(device.fun =                                                        \
            (PFN_##fun)vkGetDeviceProcAddr(device.logical_device, #fun))) { \
    std::cerr << "Could not load device level function: " << #fun << "!"    \
              << std::endl;                                                 \
    std::terminate();                                                       \
  }

  vk_device_level_function(vkGetDeviceQueue);
  vk_device_level_function(vkDestroyDevice);
  vk_device_level_function(vkDeviceWaitIdle);
  vk_device_level_function(vkCreateSemaphore);
  vk_device_level_function(vkCreateCommandPool);
  vk_device_level_function(vkAllocateCommandBuffers);
  vk_device_level_function(vkBeginCommandBuffer);
  vk_device_level_function(vkCmdPipelineBarrier);
  vk_device_level_function(vkCmdClearColorImage);
  vk_device_level_function(vkEndCommandBuffer);
  vk_device_level_function(vkQueueSubmit);
  vk_device_level_function(vkFreeCommandBuffers);
  vk_device_level_function(vkDestroyCommandPool);
  vk_device_level_function(vkDestroySemaphore);
  vk_device_level_function(vkCreateSwapchainKHR);
  vk_device_level_function(vkGetSwapchainImagesKHR);
  vk_device_level_function(vkAcquireNextImageKHR);
  vk_device_level_function(vkQueuePresentKHR);
  vk_device_level_function(vkDestroySwapchainKHR);

#undef vk_device_level_function

  // Retrieving queue handles.
  device.vkGetDeviceQueue(device.logical_device,
                          indices.graphics_family.value(), 0,
                          &device.graphics_queue);
  device.vkGetDeviceQueue(device.logical_device, indices.present_family.value(),
                          0, &device.present_queue);

  // Create queue semapthores.
  device.image_available_semaphore = create_semaphore(device);
  device.rendering_finished_semaphore = create_semaphore(device);

  return device;
}

auto gfx::vk_api::create_device_swap_chain(VulkanDevice& device) -> void
{
  if (device.logical_device != VK_NULL_HANDLE) {
    device.vkDeviceWaitIdle(device.logical_device);
  }
  /*
   * Acquiring Surface Capabilities. Acquired capabilities contain important
   * information about ranges (limits) that are supported by the swap chain,
   * that is, minimal and maximal number of images, minimal and maximal
   * dimensions of images
   */
  VkSurfaceCapabilitiesKHR surface_capabilities;
  if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
          device.physical_device, device.surface, &surface_capabilities) !=
      VK_SUCCESS) {
    std::cerr << "Could not check presentation surface capabilities!"
              << std::endl;
    std::terminate();
  }
  // Acquiring Supported Surface Formats.
  uint32_t formats_count;
  if ((vkGetPhysicalDeviceSurfaceFormatsKHR(device.physical_device,
                                            device.surface, &formats_count,
                                            nullptr) != VK_SUCCESS) ||
      (formats_count == 0)) {
    std::cerr
        << "Error occurred during presentation surface formats enumeration!"
        << std::endl;
    std::terminate();
  }
  std::vector<VkSurfaceFormatKHR> surface_formats(formats_count);
  if (vkGetPhysicalDeviceSurfaceFormatsKHR(
          device.physical_device, device.surface, &formats_count,
          surface_formats.data()) != VK_SUCCESS) {
    std::cerr
        << "Error occurred during presentation surface formats enumeration!"
        << std::endl;
    std::terminate();
  }

  // Acquiring Supported Present Modes.
  uint32_t present_modes_count;
  if ((vkGetPhysicalDeviceSurfacePresentModesKHR(
           device.physical_device, device.surface, &present_modes_count,
           nullptr) != VK_SUCCESS) ||
      (present_modes_count == 0)) {
    std::cerr << "Error occurred during presentation surface present modes "
                 "enumeration!"
              << std::endl;
    std::terminate();
  }
  std::vector<VkPresentModeKHR> present_modes(present_modes_count);
  if (vkGetPhysicalDeviceSurfacePresentModesKHR(
          device.physical_device, device.surface, &present_modes_count,
          present_modes.data()) != VK_SUCCESS) {
    std::cerr << "Error occurred during presentation surface present modes "
                 "enumeration!"
              << std::endl;
    std::terminate();
  }

  // Selecting the Number of Swap Chain Images.
  uint32_t desired_number_of_images =
      get_swap_chain_num_images(surface_capabilities);
  // Selecting a Format for Swap Chain Images.
  VkSurfaceFormatKHR desired_format = get_swap_chain_format(surface_formats);
  // Selecting the Size of the Swap Chain Images.
  VkExtent2D desired_extent = get_swap_chain_extent(surface_capabilities);
  // Selecting Swap Chain Usage Flags.
  VkImageUsageFlags desired_usage =
      get_swap_chain_usage_flags(surface_capabilities);
  // Selecting Pre-Transformations.
  VkSurfaceTransformFlagBitsKHR desired_transform =
      get_swap_chain_transform(surface_capabilities);
  // Selecting Presentation Mode.
  VkPresentModeKHR desired_present_mode =
      get_swap_chain_present_mode(present_modes);

  VkSwapchainKHR old_swap_chain = device.swap_chain;

  if (static_cast<int>(desired_usage) == -1) {
    std::cerr << "Invalid swap chain desired usage." << std::endl;
    std::terminate();
  }
  if (static_cast<int>(desired_present_mode) == -1) {
    std::cerr << "Invalid swap chain desired present mode." << std::endl;
    std::terminate();
  }
  if ((desired_extent.width == 0) || (desired_extent.height == 0)) {
    // Current surface size is (0, 0) so we can't create a swap chain and render
    // anything (CanRender == false) But we don't wont to kill the application
    // as this situation may occur i.e. when window gets minimized
    // TODO!.
  }

  VkSwapchainCreateInfoKHR swap_chain_create_info = {
      VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,  // sType
      nullptr,                                      // *pNext
      0,                                            // flags
      device.surface,                               // surface
      desired_number_of_images,                     // minImageCount
      desired_format.format,                        // imageFormat
      desired_format.colorSpace,                    // imageColorSpace
      desired_extent,                               // imageExtent
      1,                                            // imageArrayLayers
      desired_usage,                                // imageUsage
      VK_SHARING_MODE_EXCLUSIVE,                    // imageSharingMode
      0,                                            // queueFamilyIndexCount
      nullptr,                                      // pQueueFamilyIndices
      desired_transform,                            //  preTransform
      VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,            // compositeAlpha
      desired_present_mode,                         // presentMode
      VK_TRUE,                                      // clipped
      old_swap_chain                                // oldSwapchain
  };

  if (device.vkCreateSwapchainKHR(device.logical_device,
                                  &swap_chain_create_info, nullptr,
                                  &device.swap_chain) != VK_SUCCESS) {
    std::cerr << "Could not create swap chain!" << std::endl;
    std::terminate;
  }
  if (old_swap_chain != VK_NULL_HANDLE) {
    device.vkDestroySwapchainKHR(device.logical_device, old_swap_chain,
                                 nullptr);
  }
}

auto gfx::vk_api::check_physical_device_extension_support(
    VkPhysicalDevice device) -> bool
{
  uint32_t extension_count;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count,
                                       nullptr);

  std::vector<VkExtensionProperties> available_extensions(extension_count);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count,
                                       available_extensions.data());

  std::set<std::string> required_extensions(DEVICE_EXTENSIONS.begin(),
                                            DEVICE_EXTENSIONS.end());

  for (const auto& extension : available_extensions) {
    required_extensions.erase(extension.extensionName);
  }

  return required_extensions.empty();
}

auto gfx::vk_api::pick_best_physical_device_for_surface(VkSurfaceKHR surface)
    -> VkPhysicalDevice
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
    int score = rate_physical_device_suitability(device, surface);
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

auto gfx::vk_api::rate_physical_device_suitability(VkPhysicalDevice device,
                                                   VkSurfaceKHR surface) -> int
{
  if (!is_physical_device_suitable_for_surface(device, surface)) return 0;

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

auto gfx::vk_api::is_physical_device_suitable_for_surface(
    VkPhysicalDevice device, VkSurfaceKHR surface) -> bool
{
  QueueFamilyIndices indices = find_queue_families(device, surface);
  bool extensions_supported = check_physical_device_extension_support(device);
  return indices.is_complete() && extensions_supported;
}

auto gfx::vk_api::find_queue_families(VkPhysicalDevice device,
                                      VkSurfaceKHR surface)
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

    VkBool32 present_support = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);

    if (queueFamily.queueCount > 0 && present_support) {
      indices.present_family = i;
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

auto gfx::vk_api::check_extension_availability(
    const char* extension_name,
    const std::vector<VkExtensionProperties>& available_extensions) -> bool
{
  for (size_t i = 0; i < available_extensions.size(); ++i) {
    if (strcmp(available_extensions[i].extensionName, extension_name) == 0) {
      return true;
    }
  }
  return false;
}

auto gfx::vk_api::create_window_surface(os::WindowParameters window)
    -> VkSurfaceKHR
{
  VkSurfaceKHR surface;

#if defined(VK_USE_PLATFORM_WIN32_KHR)

  VkWin32SurfaceCreateInfoKHR surface_create_info = {
      VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,  // sType
      nullptr,                                          // *pNext
      0,                                                // flags
      window.instance,                                  // hinstance
      window.handle                                     // hwnd
  };

  if (vkCreateWin32SurfaceKHR(VK_INSTANCE, &surface_create_info, nullptr,
                              &surface) != VK_SUCCESS) {
    std::cerr << "Error occurred during window surface creation." << std::endl;
    std::terminate();
  }
#elif defined(VK_USE_PLATFORM_XCB_KHR)
  VkXcbSurfaceCreateInfoKHR surface_create_info = {
      VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,  // sType
      nullptr,                                        // *pNext
      0,                                              // flags
      window.connection,                              // connection
      window.handle                                   // window
  };

  if (vkCreateXcbSurfaceKHR(VK_INSTANCE, &surface_create_info, nullptr,
                            &surface) != VK_SUCCESS) {
    std::cerr << "Error occurred during window surface creation." << std::endl;
    std::terminate();
  }
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
  VkXlibSurfaceCreateInfoKHR surface_create_info = {
      VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,  // sType
      nullptr,                                         // *pNext
      0,                                               // flags
      window.display_ptr,                              // *dpy
      window.handle                                    // window
  };
  if (vkCreateXlibSurfaceKHR(VK_INSTANCE, &surface_create_info, nullptr,
                             &surface) != VK_SUCCESS) {
    std::cerr << "Error occurred during window surface creation." << std::endl;
    std::terminate();
  }

#endif
  return surface;
}

auto gfx::vk_api::create_semaphore(VulkanDevice& device) -> VkSemaphore
{
  VkSemaphoreCreateInfo semaphore_create_info = {
      VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,  // sType
      nullptr,                                  // pNext
      0                                         // flags
  };

  VkSemaphore semaphore;

  if (device.vkCreateSemaphore(device.logical_device, &semaphore_create_info,
                               nullptr, &semaphore) != VK_SUCCESS) {
    std::cerr << "Could not create semaphore!" << std::endl;
    std::terminate();
  }

  return semaphore;
}

auto gfx::vk_api::get_swap_chain_num_images(
    VkSurfaceCapabilitiesKHR& surface_capabilities) -> uint32_t
{
  // Set of images defined in a swap chain may not always be available for
  // application to render to: One may be displayed and one may wait in a queue
  // to be presented If application wants to use more images at the same time it
  // must ask for more images
  uint32_t image_count = surface_capabilities.minImageCount + 1;
  if ((surface_capabilities.maxImageCount > 0) &&
      (image_count > surface_capabilities.maxImageCount)) {
    image_count = surface_capabilities.maxImageCount;
  }
  return image_count;
}

auto gfx::vk_api::get_swap_chain_extent(
    VkSurfaceCapabilitiesKHR& surface_capabilities) -> VkExtent2D
{
  // Special value of surface extent is width == height == -1
  // If this is so we define the size by ourselves but it must fit within
  // defined confines
  if (surface_capabilities.currentExtent.width == -1) {
    VkExtent2D swap_chain_extent = {1920, 1080};
    if (swap_chain_extent.width < surface_capabilities.minImageExtent.width) {
      swap_chain_extent.width = surface_capabilities.minImageExtent.width;
    }
    if (swap_chain_extent.height < surface_capabilities.minImageExtent.height) {
      swap_chain_extent.height = surface_capabilities.minImageExtent.height;
    }
    if (swap_chain_extent.width > surface_capabilities.maxImageExtent.width) {
      swap_chain_extent.width = surface_capabilities.maxImageExtent.width;
    }
    if (swap_chain_extent.height > surface_capabilities.maxImageExtent.height) {
      swap_chain_extent.height = surface_capabilities.maxImageExtent.height;
    }
    return swap_chain_extent;
  }

  // Most of the cases we define size of the swap_chain images equal to current
  // window's size
  return surface_capabilities.currentExtent;
}

auto gfx::vk_api::get_swap_chain_format(
    std::vector<VkSurfaceFormatKHR>& surface_formats) -> VkSurfaceFormatKHR
{
  // If the list contains only one entry with undefined format
  // it means that there are no preferred surface formats and any can be chosen
  if ((surface_formats.size() == 1) &&
      (surface_formats[0].format == VK_FORMAT_UNDEFINED)) {
    return {VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR};
  }

  // Check if list contains most widely used R8 G8 B8 A8 format
  // with nonlinear color space
  for (VkSurfaceFormatKHR& surface_format : surface_formats) {
    if (surface_format.format == VK_FORMAT_R8G8B8A8_UNORM) {
      return surface_format;
    }
  }

  // Return the first format from the list
  return surface_formats[0];
}

auto gfx::vk_api::get_swap_chain_usage_flags(
    VkSurfaceCapabilitiesKHR& surface_capabilities) -> VkImageUsageFlags
{
  // Color attachment flag must always be supported
  // We can define other usage flags but we always need to check if they are
  // supported
  if (surface_capabilities.supportedUsageFlags &
      VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
    return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
           VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  }
  std::cerr << "VK_IMAGE_USAGE_TRANSFER_DST image usage is not supported by "
               "the swap chain!"
            << std::endl
            << "Supported swap chain's image usages include:" << std::endl
            << (surface_capabilities.supportedUsageFlags &
                        VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                    ? "    VK_IMAGE_USAGE_TRANSFER_SRC\n"
                    : "")
            << (surface_capabilities.supportedUsageFlags &
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT
                    ? "    VK_IMAGE_USAGE_TRANSFER_DST\n"
                    : "")
            << (surface_capabilities.supportedUsageFlags &
                        VK_IMAGE_USAGE_SAMPLED_BIT
                    ? "    VK_IMAGE_USAGE_SAMPLED\n"
                    : "")
            << (surface_capabilities.supportedUsageFlags &
                        VK_IMAGE_USAGE_STORAGE_BIT
                    ? "    VK_IMAGE_USAGE_STORAGE\n"
                    : "")
            << (surface_capabilities.supportedUsageFlags &
                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                    ? "    VK_IMAGE_USAGE_COLOR_ATTACHMENT\n"
                    : "")
            << (surface_capabilities.supportedUsageFlags &
                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
                    ? "    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT\n"
                    : "")
            << (surface_capabilities.supportedUsageFlags &
                        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT
                    ? "    VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT\n"
                    : "")
            << (surface_capabilities.supportedUsageFlags &
                        VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
                    ? "    VK_IMAGE_USAGE_INPUT_ATTACHMENT"
                    : "")
            << std::endl;
  return static_cast<VkImageUsageFlags>(-1);
}

auto gfx::vk_api::get_swap_chain_transform(
    VkSurfaceCapabilitiesKHR& surface_capabilities)
    -> VkSurfaceTransformFlagBitsKHR
{
  // Sometimes images must be transformed before they are presented (i.e. due to
  // device's orienation being other than default orientation) If the specified
  // transform is other than current transform, presentation engine will
  // transform image during presentation operation; this operation may hit
  // performance on some platforms Here we don't want any transformations to
  // occur so if the identity transform is supported use it otherwise just use
  // the same transform as current transform
  if (surface_capabilities.supportedTransforms &
      VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
    return VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  }
  else {
    return surface_capabilities.currentTransform;
  }
}

auto gfx::vk_api::get_swap_chain_present_mode(
    std::vector<VkPresentModeKHR>& present_modes) -> VkPresentModeKHR
{
  // FIFO present mode is always available
  // MAILBOX is the lowest latency V-Sync enabled mode (something like
  // triple-buffering) so use it if available
  for (VkPresentModeKHR& present_mode : present_modes) {
    if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return present_mode;
    }
  }
  for (VkPresentModeKHR& present_mode : present_modes) {
    if (present_mode == VK_PRESENT_MODE_FIFO_KHR) {
      return present_mode;
    }
  }
  std::cout << "FIFO present mode is not supported by the swap chain!"
            << std::endl;
  return static_cast<VkPresentModeKHR>(-1);
}

auto gfx::load_backend() -> void { vk_api::initialize(); }

auto gfx::unload_backend() -> void { vk_api::destroy(); }

auto gfx::create_device(const os::WindowParameters& window) -> Device
{
  return vk_api::create_device(window);
}

auto gfx::print_device_name(vk_api::VulkanDevice device) -> void
{
  VkPhysicalDeviceProperties device_properties;
  vk_api::vkGetPhysicalDeviceProperties(device.physical_device,
                                        &device_properties);
  std::cout << "Device name: " << device_properties.deviceName << "\n";
}

auto gfx::destroy_device(vk_api::VulkanDevice device) -> void
{
  if (device.logical_device != VK_NULL_HANDLE) {
    device.vkDeviceWaitIdle(device.logical_device);
  }
  if (device.image_available_semaphore != VK_NULL_HANDLE) {
    device.vkDestroySemaphore(device.logical_device,
                              device.image_available_semaphore, nullptr);
  }
  if (device.rendering_finished_semaphore != VK_NULL_HANDLE) {
    device.vkDestroySemaphore(device.logical_device,
                              device.rendering_finished_semaphore, nullptr);
  }
  if (device.swap_chain != VK_NULL_HANDLE) {
    device.vkDestroySwapchainKHR(device.logical_device, device.swap_chain,
                                 nullptr);
  }

  device.vkDestroyDevice(device.logical_device, nullptr);
  vk_api::vkDestroySurfaceKHR(vk_api::VK_INSTANCE, device.surface, nullptr);
}

auto gfx::print_device_name(const gfx::Device& device) -> void
{
  device.self_->print_name_();
}

auto gfx::destroy_device(const Device& device) -> void
{
  device.self_->destroy_();
  std::cout << "Device destroyed.\n";
}

auto gfx::create_swap_chain(const Device& device) -> void {}
