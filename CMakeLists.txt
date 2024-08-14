cmake_minimum_required(VERSION 3.10)
project(OreoBuild)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(src)

add_executable(oreobuild
    src/main.cpp
    src/core/build_system.cpp
    src/core/config.cpp
    src/core/compiler_gcc.cpp
    src/core/file_utils.cpp
    src/core/thread_pool.cpp
    src/cli_handler.cpp
    src/color.cpp
)

target_link_libraries(oreobuild PRIVATE pthread dl)
