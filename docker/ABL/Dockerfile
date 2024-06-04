FROM ubuntu:20.04

# 更新软件包列表并安装 tzdata  
#RUN apt-get update && apt-get install -y tzdata
# 使用RUN命令设置时区
#RUN cp /usr/share/zoneinfo/Asia/Shanghai /etc/localtime && echo 'Asia/Shanghai'>/etc/timezone

# 创建目标目录并将二进制文件复制到目标目录
RUN mkdir -p /opt/media/ABLMediaServer

# 将二进制文件复制到目标目录
ADD ./bin /opt/media/ABLMediaServer

# 设置工作目录
WORKDIR /opt/media/ABLMediaServer

# 确保 start.sh 是可执行的并设置其他文件权限
RUN chmod +x ./start.sh
RUN chmod +x ./ABLMediaServer
RUN chmod -R 777 /opt/media/ABLMediaServer

# 设置库路径环境变量
ENV LD_LIBRARY_PATH=/opt/media/ABLMediaServer:$LD_LIBRARY_PATH

# 指定入口点运行 ABLMediaServer
CMD ./start.sh