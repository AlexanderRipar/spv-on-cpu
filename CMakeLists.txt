cmake_minimum_required(VERSION 3.22)

project(spirv-on-cpu)

find_package(Vulkan REQUIRED)

add_library(spv-on-cpu SHARED spv_viewer.cpp spv_viewer.hpp spird_accessor.cpp spird_accessor.hpp spird_hashing.cpp spird_hashing.hpp spird_names.cpp spird_names.hpp spv_defs.hpp spird_defs.hpp id_data.hpp)

target_link_libraries(spv-on-cpu PRIVATE ${Vulkan_LIBRARY})

target_include_directories(spv-on-cpu PRIVATE ${Vulkan_INCLUDE_DIR})

target_compile_features(spv-on-cpu PRIVATE cxx_std_17)



add_executable(tests tests.cpp spv_viewer.hpp spird_defs.hpp spird_accessor.cpp spird_accessor.hpp spird_hashing.cpp spird_hashing.hpp spird_names.cpp spird_names.hpp)

target_link_libraries(tests PRIVATE spv-on-cpu ${Vulkan_LIBRARY})

target_include_directories(tests PRIVATE ${Vulkan_INCLUDE_DIR})

target_compile_features(tests PRIVATE cxx_std_17)



add_executable(spird-builder spird_builder_main.cpp spird_builder_strings.hpp spird_defs.hpp spird_hashing.cpp spird_hashing.hpp spird_names.cpp spird_names.hpp)

target_compile_features(spird-builder PRIVATE cxx_std_17)



add_executable(asm-test assembly_testing.cpp)

target_compile_features(asm-test PRIVATE cxx_std_17)
