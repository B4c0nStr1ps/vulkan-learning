cmake_minimum_required(VERSION 3.12)
project(vulkan-learning VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER _builds)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if( NOT USE_PLATFORM MATCHES "VK_USE_PLATFORM_.*" )
	if( WIN32 )
		set( USE_PLATFORM "VK_USE_PLATFORM_WIN32_KHR" )
	elseif( UNIX )
		set( USE_PLATFORM "VK_USE_PLATFORM_XCB_KHR" )
	endif()
endif()

if( USE_PLATFORM STREQUAL "VK_USE_PLATFORM_WIN32_KHR" )
	#set( CMAKE_C_FLAGS_RELWITHDEBINFO    "${CMAKE_C_FLAGS_RELWITHDEBINFO} /MT" )
	#set( CMAKE_C_FLAGS_RELEASE           "${CMAKE_C_FLAGS_RELEASE} /MT" )
	#set( CMAKE_C_FLAGS_DEBUG             "${CMAKE_C_FLAGS_DEBUG} /MTd" )
	#set( CMAKE_CXX_FLAGS_RELWITHDEBINFO  "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MT" )
	#set( CMAKE_CXX_FLAGS_RELEASE         "${CMAKE_CXX_FLAGS_RELEASE} /MT" )
	#set( CMAKE_CXX_FLAGS_DEBUG           "${CMAKE_CXX_FLAGS_DEBUG} /MTd" )
	set( PLATFORM_LIBRARY                "" )
	#add_definitions( -DNOMINMAX )
elseif( USE_PLATFORM STREQUAL "VK_USE_PLATFORM_XCB_KHR" )
	#add_definitions( -std=c++11 )
	set( PLATFORM_LIBRARY dl xcb )
elseif( USE_PLATFORM STREQUAL "VK_USE_PLATFORM_XLIB_KHR" )
	#add_definitions( -std=c++11 )
	set( PLATFORM_LIBRARY dl X11 )
endif()


add_definitions( -D${USE_PLATFORM} )

#Create the target.
add_executable(vulkan-learning src/main.cpp src/vulkan_api.h src/vulkan_api.cpp src/platform.h src/platform.cpp)
target_include_directories(vulkan-learning PRIVATE "external")
#add platform library.
target_link_libraries( vulkan-learning ${PLATFORM_LIBRARY} )