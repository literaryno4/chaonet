cmake_minimum_required(VERSION 3.10)
project(chaonet)
set(CMAKE_CXX_STANDARD 17)

if(EXISTS "/usr/bin/clang")
    set(CMAKE_CXX_COMPILER /usr/bin/clang++)
    set(CMAKE_C_COMPILER /usr/bin/clang)
endif()
message(${CMAKE_CXX_COMPILER})

if (NOT TARGET spdlog)
    find_package(spdlog REQUIRED)
endif()

add_subdirectory(chaonet)
add_subdirectory(tests)
add_subdirectory(examples)
