cmake_minimum_required(VERSION 3.25)
project(FAE_Linux)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/bin)

include_directories(src/helpers)

add_executable(FAE_Linux
    src/helpers/filehelper.cpp
    src/helpers/filehelper.hpp
    src/helpers/logging.cpp
    src/helpers/logging.hpp
    src/helpers/patching.cpp
    src/helpers/patching.hpp
    src/library.cpp)
