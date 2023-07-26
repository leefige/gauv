cmake_minimum_required(VERSION 3.12)
project(mpc_verifier)  # 设置项目名称

# 设置编译选项
add_compile_options(
    -Wall         # 开启所有警告
    -Wextra       # 开启更多警告
    -O2           # 优化等级为 2
    -std=c++20    # 使用 C++20 标准
    -MMD          # 生成依赖关系文件
)

include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}/immer"
)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/test/bgw")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/test/conversion")
