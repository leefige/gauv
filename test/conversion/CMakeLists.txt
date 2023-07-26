# 用 GLOB 获取当前目录下所有的 cpp 文件
file(GLOB CPP_FILES "*.cpp")

# 遍历每个 cpp 文件，生成对应的可执行文件
foreach(CPP_FILE ${CPP_FILES})
    # 获取每个 cpp 文件的文件名（不含后缀）
    get_filename_component(EXECUTABLE_NAME ${CPP_FILE} NAME_WE)

    # 添加可执行文件，并命名为对应的文件名
    add_executable(${EXECUTABLE_NAME} ${CPP_FILE})

    # 设置 C++ 标准
    set_target_properties(${EXECUTABLE_NAME} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
    )
endforeach()
