#pragma once
#if defined(VK_USE_PLATFORM_WIN32_KHR)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#endif

namespace os {

// ************************************************************ //
// WindowParameters                                             //
//                                                              //
// OS dependent window parameters                               //
// ************************************************************ //
struct WindowParameters {
#if defined(VK_USE_PLATFORM_WIN32_KHR)
  HINSTANCE instance;
  HWND handle;

#elif defined(VK_USE_PLATFORM_XCB_KHR)
  xcb_connection_t* connection;
  xcb_window_t handle;

  WindowParameters() : connection(), handle() {}

#elif defined(VK_USE_PLATFORM_XLIB_KHR)
  Display* display_ptr;
  Window handle;

  WindowParameters() : display_ptr(), handle() {}

#endif
};

class Window {
 public:
  Window();
  ~Window();
  auto create(const char* title) -> void;
  auto get_parameters() const -> WindowParameters;

 private:
  WindowParameters parameters_;
};

}  // namespace os
