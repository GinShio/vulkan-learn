cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

project(VulkanTriangle
  DESCRIPTION "Draw a triangle by Vulkan CXX"
  LANGUAGES CXX
  VERSION 1.0.0
  )

add_executable(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}
  )

target_sources(${PROJECT_NAME} PRIVATE
  main.cpp
  triangle.cpp
  )

target_link_libraries(${PROJECT_NAME} PUBLIC
  VulkanBase
  )

target_glsl_shaders(${PROJECT_NAME} PRIVATE
  main.vert
  main.frag

  COMPILE_OPTIONS --target-env vulkan1.0
  )
