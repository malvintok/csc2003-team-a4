# set minimum required version of CMake
cmake_minimum_required(VERSION 3.12)

# sdk
# include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include(pico_sdk_import.cmake)

# set name of project 
project(uAndA)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})

# creates a pico sdk subdirectory in our project for the libaries
pico_sdk_init()

# include(example_auto_set_url.cmake)

add_subdirectory(ultrasonic)
add_subdirectory(accelerometer)
