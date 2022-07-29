cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

# PRACTICE_LLVM_ROOT_DIR:STRING    -- spec the external llvm root directory
# PRACTICE_ENABLE_CACHE:BOOL       -- enable cache system
# PRACTICE_USE_LINKER:STRING       -- setup linker
# PRACTICE_WARNINGS_AS_ERRORS:BOOL -- warnings as errors

include(cmake/common.cmake)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")  # .a file
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")  # .so file
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")  # bin file

add_subdirectory(projects)
