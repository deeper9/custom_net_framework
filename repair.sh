#!/bin/bash

get_second_last_reflog_commit_id() {
    local branch=$1
    local commit_id=$(tail -n 2 .git/logs/refs/heads/$branch | head -n 1 | awk '{print $2}')
    
    # 检查是否成功获取commit ID
    if [ -z "$commit_id" ]; then
        echo "无法获取倒数第二条reflog的commit ID。"
        return 1
    fi
    
    echo $commit_id
    return 0
}

find . -type f -empty -delete -print

branch="main"
commit_id=$(get_second_last_reflog_commit_id $branch)

# 检查函数返回值
if [ $? -ne 0 ]; then
    exit 1
fi

# 输出commit ID
echo "倒数第二条reflog的commit ID: $commit_id"

git update-ref HEAD $commit_id
git pull
