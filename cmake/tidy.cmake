include_guard()

function(enable_clang_tidy)
  if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    return()
  endif()

  find_program(CLANG_TIDY_EXE clang-tidy)
  if(NOT CLANG_TIDY_EXE)
    message(FATAL_ERROR "clang-tidy requested but executable not found")
  endif()

  set(clang_tidy_command ${CLANG_TIDY_EXE} -header-filter=".*")
  set(CMAKE_CXX_CLANG_TIDY ${clang_tidy_command} PARENT_SCOPE)
  set(CMAKE_C_CLANG_TIDY ${clang_tidy_command} PARENT_SCOPE)
endfunction()

function(enable_include_what_you_use)
  if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    return()
  endif()

  find_program(INCLUDE_WHAT_YOU_USE_EXE include-what-you-use)
  if(INCLUDE_WHAT_YOU_USE_EXE)
    set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${INCLUDE_WHAT_YOU_USE_EXE} PARENT_SCOPE)
  else()
    message(FATAL_ERROR "include-what-you-use requested but executable not found")
  endif()
endfunction()
