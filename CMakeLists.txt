cmake_minimum_required(VERSION 3.12)
project(vulkan-learning VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER _builds)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)


#Create the target.
add_executable(vulkan-learning src/main.cpp)