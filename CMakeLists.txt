cmake_minimum_required(VERSION 3.16)
project(NPP)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

set(CMAKE_CXX_FLAGS_DEBUG -DDEBUG)

add_subdirectory(third/fmt)
include_directories(src third/fmt/include)
add_subdirectory(third/oneTBB)
include_directories(third/oneTBB/include)

add_subdirectory(src)

add_subdirectory(test)
