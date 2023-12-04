cmake_minimum_required(VERSION 3.12)
project(mpc_verifier)  # 设置项目名称

option(USE_OPENMP "Use OpenMP" ON)

# 设置编译器
if(WIN32)
    # use default compiler
else()
    set(CMAKE_CXX_COMPILER "clang++")
endif()

set(CMAKE_CXX_STANDARD 20)  # 设置 C++ 标准为 C++20

# 设置编译选项
if(MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2")
    add_compile_options(/W4)
else()
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2")

    if(USE_OPENMP)
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fopenmp")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fopenmp")
    endif()

    add_compile_options(-Wall -Wextra -MMD)
endif()

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/immer")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/spdlog/include")

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/example/bgw")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/example/conversion")

if(USE_OPENMP)
    message(">> Using OpenMP")
    find_package(OpenMP REQUIRED)
    link_libraries(OpenMP::OpenMP_CXX)
endif()
