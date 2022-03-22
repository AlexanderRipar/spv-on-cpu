cmake_minimum_required(VERSION 3.22)

project(spirv-on-cpu)

find_package(Vulkan REQUIRED)

add_library(spvcpu SHARED spvcpu.cpp spvcpu.hpp spv_helpers.cpp spv_helpers.hpp spv_defs.hpp)

target_link_libraries(spvcpu PRIVATE ${Vulkan_LIBRARY})

target_include_directories(spvcpu PRIVATE ${Vulkan_INCLUDE_DIR})



add_executable(tests tests.cpp spvcpu.hpp)

target_link_libraries(tests PRIVATE spvcpu ${Vulkan_LIBRARY})

target_include_directories(tests PRIVATE ${Vulkan_INCLUDE_DIR})



add_executable(spv-data-builder spv_data_builder_main.cpp spv_data_builder_strings.hpp)
