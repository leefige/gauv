#!/bin/bash

# 请在 exp/ 下面跑这个脚本
# 在使用之前，请确保有 `../build/Release`，并且里面已经编译出了 `test/bgw/test_scalable` 和 `test/bgw/test_bin2arith`，可以用如下指令编译
#   cmake --build Release --target test_bin2arith
#   cmake --build Release --target test_scalable

OUTPUT_FILE="experiment_results_bin2arith.csv"
TASK_FILE="completed_tasks_bin2arith.txt"
TIMEOUT=1500  # 设置超时时间为1500s

touch $OUTPUT_FILE
touch $TASK_FILE

# 初始化CSV文件标题
if [[ ! -f $OUTPUT_FILE ]]; then
    echo "Program,Parameter t,Parameter n,Parameter m,Duration,Initial Nodes,Initial Edges,Number of Cases" > $OUTPUT_FILE
fi

# 遍历参数并运行实验
for executable in "../../build/Release/test/conversion/test_bin2arith"; do
    for n in $(seq 3 9); do
        for t in $(seq 1 $(( (n - 1) / 2 ))); do
            timeout_occurred=false
            
            args="I $t $n $m"
            
            # 检查任务是否已完成
            if grep -q "${executable} ${args}" "$TASK_FILE"; then
                echo "Skipping ${executable} ${args} as it's already done"
                continue
            fi
            
            # 运行实验并设置超时检查
            start_time=$(date +%s.%N)
            output=$( (timeout $TIMEOUT time $executable $args) 2>&1 )
            
            # 检查是否超时
            if [[ $? == 124 ]]; then
                timeout_occurred=true
                echo "Timeout occurred for ${executable} ${args}. Skipping larger m values."
                continue
            fi

            # 计算时间
            end_time=$(date +%s.%N)
            duration=$(echo "$end_time - $start_time" | bc)
            
            # 提取信息
            initial_nodes=$(echo "$output" | grep "Initial graph has" | awk '{print $9}')
            initial_edges=$(echo "$output" | grep "Initial graph has" | awk '{print $11}')
            considered_cases=$(echo "$output" | grep "Consider the case that the corrupted parties are" | wc -l)
            
            # 写入CSV
            echo "$executable,$t,$n,$m,$duration,$initial_nodes,$initial_edges,$considered_cases" >> $OUTPUT_FILE

            # 记录已完成任务
            echo "${executable} ${args}" >> $TASK_FILE
        done
    done
done

echo "All tasks completed."
