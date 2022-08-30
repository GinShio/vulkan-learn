cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

set(TARGET_NAME "VulkanBase")

enable_clang_tidy()

add_library(${TARGET_NAME})

target_include_directories(${TARGET_NAME} PUBLIC
  ${CMAKE_SOURCE_DIR}/include
  )

target_sources(${TARGET_NAME} PRIVATE
  window.cpp
  create.cpp
  )

target_link_libraries(${TARGET_NAME} PRIVATE
  )
