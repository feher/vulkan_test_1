cmake_minimum_required(VERSION 3.20)

project("vulkan_test_01" LANGUAGES CXX)
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

add_subdirectory(src)
