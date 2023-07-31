cmake_minimum_required(VERSION 3.12)
project(mpc_verifier)  # 设置项目名称

# 设置编译器
# set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
set(CMAKE_CXX_STANDARD 20)  # 设置 C++ 标准为 C++20

# 设置编译选项
if (MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /DEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2")
    add_compile_options("/W4")
else()
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2")
    add_compile_options("-Wall -Wextra -MMD")
endif()

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/immer")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/spdlog/include")

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/test/bgw")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/test/conversion")
