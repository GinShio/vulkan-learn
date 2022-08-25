cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

project(VulkanTest
  DESCRIPTION "Test Vulkan"
  LANGUAGES CXX
  VERSION 1.0.0
  )

add_executable(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  )

target_sources(${PROJECT_NAME} PRIVATE
  main.cpp
  )
