cmake_minimum_required(VERSION 3.12)
project(mpc_verifier)  # 设置项目名称

# 设置编译器
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
set(CMAKE_CXX_STANDARD 20)  # 设置 C++ 标准为 C++11

# 设置编译选项
add_compile_options(
    -Wall         # 开启所有警告
    -Wextra       # 开启更多警告
    -O2           # 优化等级为 2
    -MMD          # 生成依赖关系文件
)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/test/bgw")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/test/conversion")
