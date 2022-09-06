cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

project(VulkanTexture
  DESCRIPTION "Learn Vulkan Texture pipeline"
  LANGUAGES CXX
  VERSION 1.0.0
  )

enable_clang_tidy()

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE
  main.cpp
  texture.cpp
  )

target_include_directories(${PROJECT_NAME} PUBLIC
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}
  )

target_link_libraries(${PROJECT_NAME} PUBLIC
  VulkanBase
  )

target_glsl_shaders(${PROJECT_NAME} PRIVATE
  FILES
  main.vert
  main.frag
  )
