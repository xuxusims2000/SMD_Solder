# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS components) #Here we tell CMake where to look for extra components that our project might use.

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(SMD_Solder) 

# Include directories
# Specify the directories to search for header files
#include_directories(
 #   ${CMAKE_SOURCE_DIR}/include

 # Source directories
# Add subdirectories for source code, libraries, tests, and tools
#add_subdirectory(src)
#add_subdirectory(lib)
#add_subdirectory(tests)
#add_subdirectory(tools)
  