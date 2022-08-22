include_guard()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if("${CMAKE_BUILD_TYPE}" STREQUAL "")
  message(STATUS "Setting build type to 'Debug' as none was specified.")
  set(CMAKE_BUILD_TYPE
    "Debug"
    CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui, ccmake
  set_property(
    CACHE CMAKE_BUILD_TYPE
    PROPERTY STRINGS
    "Debug"
    "Release"
    "MinSizeRel"
    "RelWithDebInfo")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(DEFINED PKG_CONFIG_PATH)
  set(ENV{PKG_CONFIG_PATH} ${PKG_CONFIG_PATH})
endif()

find_package(PkgConfig REQUIRED)
pkg_search_module(VULKAN REQUIRED IMPORTED_TARGET vulkan)

include_directories(
  ${VULKAN_INCLUDE_DIRS}
  )

add_compile_options(
  ${VULKAN_CFLAGS_OTHER}
  )

link_libraries(
  PkgConfig::VULKAN
  )

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
    add_compile_options(-ferror-limit=5)
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-fmax-errors=5)
  endif()
  add_compile_options(
    -fno-stack-protector
    -fno-common
    -Wall
    -march=native
    )
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_definitions(
    DEBUG
    )
  if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # from lua Makefile
    add_compile_options(
      -fno-inline
      -Weffc++
      # from lua Makefile
      -Wdisabled-optimization
      -Wdouble-promotion
      -Wextra
      -Wmissing-declarations
      -Wredundant-decls
      -Wshadow
      -Wsign-compare
      -Wundef
      # -Wfatal-errors
      )
  endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  add_compile_definitions(
    DEBUG
    )
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  add_compile_definitions(
    NDEBUG
    )
endif()

function(target_add_window_library
    TARGET_NAME
    )
  set(options INTERFACE PUBLIC PRIVATE GLFW SDL)
  set(one_value_args)
  set(multi_value_args)
  cmake_parse_arguments(WINDOW_LIB "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})
  set(WINDOW_LIB_PROPERTY)
  if(WINDOW_LIB_PRIVATE)
    set(WINDOW_LIB_PROPERTY "PRIVATE")
  elseif(WINDOW_LIB_INTERFACE)
    set(WINDOW_LIB_PROPERTY "INTERFACE")
  else()
    set(WINDOW_LIB_PROPERTY "PUBLIC")
  endif()
  if(WINDOW_LIB_GLFW)
    pkg_search_module(GLFW REQUIRED IMPORTED_TARGET glfw3)
    target_include_directories(${TARGET_NAME} PUBLIC ${GLFW_INCLUDE_DIRS})
    target_compile_options(${TARGET_NAME} ${WINDOW_LIB_PROPERTY} ${GLFW_CFLAGS_OTHER})
    target_link_libraries(${TARGET_NAME} ${WINDOW_LIB_PROPERTY} PkgConfig::GLFW)
    target_compile_definitions(${TARGET_NAME} PUBLIC GLFW_INCLUDE_VULKAN)
  endif()
  if(WINDOW_LIB_SDL)
    pkg_search_module(SDL REQUIRED IMPORTED_TARGET sdl2)
    target_include_directories(${TARGET_NAME} PUBLIC ${SDL_INCLUDE_DIRS})
    target_compile_options(${TARGET_NAME} ${WINDOW_LIB_PROPERTY} ${SDL_CFLAGS_OTHER})
    target_link_libraries(${TARGET_NAME} ${WINDOW_LIB_PROPERTY} PkgConfig::SDL)
    if(WIN32)
      target_compile_definitions(${TARGET_NAME} PUBLIC SDL_MAIN_HANDLED)
    endif()
  endif()
endfunction()
