cmake_minimum_required(VERSION 3.16)
project(npc)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

# set(CMAKE_CXX_FLAGS -rdynamic)

add_subdirectory(third/fmt)

include_directories(src third/fmt/include)

add_subdirectory(src)

add_subdirectory(test)
