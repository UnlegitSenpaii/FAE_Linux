cmake_minimum_required(VERSION 3.24)
project(FAE_Linux)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_C_FLAGS_DEBUG "-Wall -Wextra -O3 -march=core2 -mtune=native -fvisibility=hidden -flto -ggdb -DDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -std=c++2a -Wall -Wextra")

set(CMAKE_C_COMPILER "/usr/bin/gcc")
set(CMAKE_CXX_COMPILER "/usr/bin/g++")

set(CMAKE_C_FLAGS_RELEASE "-W -Wall -Wextra -O3 -march=native -mtune=native -s -fvisibility=hidden -flto -fwhole-program -Wl,--strip-all")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} --no-gnu-unique -fno-rtti -std=c++2a")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../out/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../out/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../out/bin)

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")

include_directories(/usr/local/include)
link_directories(/usr/local/lib)

add_executable(FAE_Linux ${SOURCES})
target_link_libraries(FAE_Linux pthread)