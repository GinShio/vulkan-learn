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

target_sources(${PROJECT_NAME} PUBLIC
  main.cc
  )

if(WIN32)
  target_link_libraries(${PROJECT_NAME} PUBLIC
    vulkan-1
    glfw3
    )
else()
  target_link_libraries(${PROJECT_NAME} PUBLIC
    glfw
    vulkan
    dl
    pthread
    X11
    Xxf86vm
    Xrandr
    Xi
    )
endif()
