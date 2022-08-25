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
pkg_search_module(SDL REQUIRED IMPORTED_TARGET sdl2)

include_directories(
  ${VULKAN_INCLUDE_DIRS}
  ${SDL_INCLUDE_DIRS}
  )

add_compile_options(
  ${VULKAN_CFLAGS_OTHER}
  ${SDL_CFLAGS_OTHER}
  )

link_libraries(
  PkgConfig::VULKAN
  PkgConfig::SDL
  )

if(WIN32)
  add_compile_definitions(SDL_MAIN_HANDLED)
endif()


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
