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

if(WIN32)
  # add_link_options(
  #   -LC:/VulkanSDK/1.3.216.0/Lib
  #   )
  set(ENV{VULKAN_SDK} "C:/VulkanSDK/1.3.216.0")
  find_package(Vulkan REQUIRED)
  message("${Vulkan_LIBRARIES}")
  add_compile_options(
    -I$ENV{VULKAN_SDK}/Include
    )
  add_link_options(
    -L$ENV{VULKAN_SDK}/Lib
    )
else()
  # UNIX
  add_link_options(
    )
endif()

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

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_definitions(
    DEBUG
    )
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
elseif(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  add_compile_definitions(
    NDEBUG
    )
  if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  endif()
endif()
