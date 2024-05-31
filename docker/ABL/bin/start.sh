#!/bin/bash

# 获取当前脚本所在路径
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# 设置 LD_LIBRARY_PATH 为当前路径
export LD_LIBRARY_PATH="$SCRIPT_DIR"

# 设置默认权限掩码，确保新建的文件和文件夹具有读写权限
umask 000

# 添加当前目录及其子目录的读写权限
chmod -R u+rwx "$SCRIPT_DIR"

# 切换到 ABLServer 可执行程序所在目录
cd "$SCRIPT_DIR"

# 设置 ABLMediaServer 的执行权限
chmod +x ABLMediaServer

# 打印日志信息
echo "Starting ABLMediaServer from $SCRIPT_DIR"

# 执行 ABLMediaServer 可执行程序，并将输出重定向到日志文件
./ABLMediaServer

# 获取 ABLMediaServer 的PID
ABL_PID=$!

# 打印日志信息
echo "ABLMediaServer started with PID $ABL_PID"
