#!/bin/bash

# 获取当前脚本所在路径
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# 设置 LD_LIBRARY_PATH 为当前路径
export LD_LIBRARY_PATH="$SCRIPT_DIR"

# 添加当前目录及其子目录的读写权限
chmod -R +rw "$SCRIPT_DIR"

# 切换到 ABLServer 可执行程序所在目录（假设在当前目录下）
cd "$SCRIPT_DIR"

# 执行 ABLServer 可执行程序
./ABLMediaServer

