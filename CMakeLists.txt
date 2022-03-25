cmake_minimum_required(VERSION 3.22)

project(spirv-on-cpu)

find_package(Vulkan REQUIRED)

add_library(spv-on-cpu SHARED spv_viewer.cpp spv_viewer.hpp spv_data_accessor.cpp spv_data_accessor.hpp spv_data_hashing.cpp spv_data_hashing.hpp spv_defs.hpp spv_data_defs.hpp)

target_link_libraries(spv-on-cpu PRIVATE ${Vulkan_LIBRARY})

target_include_directories(spv-on-cpu PRIVATE ${Vulkan_INCLUDE_DIR})



add_executable(tests tests.cpp spv_viewer.hpp spv_data_defs.hpp spv_data_accessor.cpp spv_data_accessor.hpp spv_data_hashing.cpp spv_data_hashing.hpp)

target_link_libraries(tests PRIVATE spv-on-cpu ${Vulkan_LIBRARY})

target_include_directories(tests PRIVATE ${Vulkan_INCLUDE_DIR})



add_executable(spv-data-builder spv_data_builder_main.cpp spv_data_builder_strings.hpp spv_data_defs.hpp spv_data_hashing.cpp spv_data_hashing.hpp)
