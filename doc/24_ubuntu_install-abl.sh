#!/bin/bash
#ubuntu24.04安装通过

apt update
apt install git build-essential  ffmpeg  openjdk-11-jre maven nodejs npm redis mysql-server -y
#############################安装完成###############################
cd /opt
git clone https://gitee.com/kangweft/wvp-pro-abl.git

cd /opt/wvp-pro-abl/web_src
npm --registry=https://mirrors.cloud.tencent.com/npm/ install
npm run build
cd ..
mvn package
cp /opt/wvp-pro-abl/src/main/resources/application-dev.yml /opt/wvp-pro-abl/target/application-dev.yml
#############################下载ZLMediaKit#########################
cd /opt/
git clone https://gitee.com/kangweft/abl.git
mv abl ABLMediaServer
touch /opt/ABLMediaServer/abl.sh
cat > /opt/ABLMediaServer/abl.sh << EOF
#!bin/bash
cd /opt/ABLMediaServer/
bash start.sh
EOF
chmod +x /opt/ABLMediaServer/abl.sh
chmod +x /opt/ABLMediaServer/ABLMediaServer
touch /opt/wvp-pro-abl/target/wvp.sh
cat > /opt/wvp-pro-abl/target/wvp.sh << EOF
#!bin/bash
cd /opt/wvp-pro-abl/target/
java -jar wvp-*.jar --spring.config.location=application-dev.ylm &
EOF
chmod +x /opt/wvp-pro-abl/target/wvp.sh
cd /opt
service mysql restart
#1、配置数据库
#登录数据库
#mysql -u root -p    有密码的输入密码如果没密码直接回车
#ALTER USER 'root'@'localhost' IDENTIFIED BY 'Qq123456';     修改root密码Qq123456
#use mysql;           
#update user set host = '%' where user = 'root';    依次执行
#select user,plugin from user where user='root';    依次执行
#创建数据库名
#create DATABASE wvp_pro_abl;   （wvp_pro_abl）就是数据库名称 对应配置文件中的  url: jdbc:mysql://127.0.0.1:3306/wvp_pro_abl?useUnicode=true&characterEncoding=UTF8&rewriteBatchedStatements=true&serverTimezone=PRC&useSSL=false&allowMultiQueries=true 可自己随意设置和配置文件中对应即可
#进入数据库
#use wvp_pro_abl;
#导入数据库
#source /opt/wvp-pro-abl/sql/2.7.2/mysql-2.7.2.sql;
#刷新数据库
#flush privileges;
#更新密码
#alter user 'root'@'%' identified with mysql_native_password by 'Qq123456';
#退出数据库
#exit
#重启数据库
#service mysql restart
#
