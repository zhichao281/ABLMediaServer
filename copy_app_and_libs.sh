#!/bin/bash

# 要拷贝的应用程序路径
APP_PATH=/home/admin-nx/ABLMediaServer/Bin/ABLMediaServer

# 目标文件夹路径，您可以根据需要修改
DEST_FOLDER=/home/admin-nx/ABLMediaServer/Bin

# 获取应用程序所依赖的库列表
LIBS_LIST=$(ldd "$APP_PATH" | awk '{print $3}')

# 创建目标文件夹
mkdir -p "$DEST_FOLDER"

# 拷贝应用程序及其依赖的库到目标文件夹
cp "$APP_PATH" "$DEST_FOLDER"
for lib in $LIBS_LIST; do
    if [ -f "$lib" ]; then
        cp "$lib" "$DEST_FOLDER"
    fi
done

echo "拷贝完成！"
