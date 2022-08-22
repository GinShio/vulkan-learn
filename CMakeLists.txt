cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

# PRACTICE_LLVM_ROOT_DIR:STRING    -- spec the external llvm root directory
# PRACTICE_ENABLE_CACHE:BOOL       -- enable cache system
# PRACTICE_USE_LINKER:STRING       -- setup linker
# PRACTICE_WARNINGS_AS_ERRORS:BOOL -- warnings as errors

include(cmake/common.cmake)
include(cmake/shaders.cmake)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")    # .a files
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")    # .so files
set(CMAKE_SHADERS_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/shader") # spirv files
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")    # bin files

add_subdirectory(projects)
