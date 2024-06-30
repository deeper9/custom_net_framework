#!/bin/bash

get_second_last_reflog_commit_id() {
    # 局部变量，保存函数第一个参数
    local branch=$1
    # | 将第一个命令的输出作为第二个命令的输入
    # head -n 1表示显示问价你的第一行内容，与|结合的效果输出tail返回的第一行内容
    # awk 打印第二列内容
    local commit_id=$(tail -n 2 .git/logs/refs/heads/$branch | head -n 1 | awk '{print $2}')
    
    # 检查是否成功获取commit ID
    # if [condition]; then：表示条件判断开始
    # -z：条件判断，检查给定变量是否为空字符串
    # $commit_id用双引号括起来以防止变量的内容被空格或特殊字符分割
    if [ -z "$commit_id" ]; then
        echo "无法获取倒数第二条reflog的commit ID。"
        return 1
    fi
    
    echo $commit_id
    return 0
}
# find为查找文件或目录的命令，.表示当前目录
# -type f表示只匹配文件
# -empty 表示只匹配空文件
# -delete 表示会删除所有查到的空文件
# -print 表示输出的文件名
# 效果：
#   1.从当前目录及其子目录中查找所有空文件。
#   2.删除这些空文件。
#   3.在删除之前打印出这些文件的路径。
find . -type f -empty -delete -print

branch="main"
# get_second_last_reflog_commit_id $branch：表示函数调用，get_second_last_reflog_commit_id表示将要调用的函数，后续为函数参数
# $(...):表示替换命令，将标准输出作为字符串赋值给接收的变量
commit_id=$(get_second_last_reflog_commit_id $branch)

# 检查函数返回值
# $? 变量会包含上一个命令或函数的退出状态码
# -ne not equal
# -eq equal
if [ $? -ne 0 ]; then
    exit 1
fi

# 输出commit ID
echo "倒数第二条reflog的commit ID: $commit_id"

git update-ref HEAD $commit_id
git pull
