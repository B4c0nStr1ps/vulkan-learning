#include <iostream>
#include "vulkan_api.h"

int main()
{
  std::cout << "Vulkan learning!\n";

  gfx::load_backend();
  std::cout << "Vulkan backend loaded.\n";

  std::cout << "\nEnumerate all physical devices.\n";
  gfx::vk_api::enumerate_all_physical_devices();

  std::cout << "\nCreate the device.\n";
  gfx::Device device = gfx::create_device();
  device.print_name();

  std::cout << "\n\n*********LOOP*********\n\n\n";

  gfx::unload_backend();
  std::cout << "Vulkan backend unloaded.\n";
  return 0;
}
