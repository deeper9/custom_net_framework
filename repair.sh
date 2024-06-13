#!/bin/bash

find . -type f -empty -delete -print

# 获取倒数第二条reflog的commit ID
commit_id=$(tail -n 2 .git/logs/refs/heads/main | head -n 1 | awk '{print $2}')

# 检查是否成功获取commit ID
if [ -z "$commit_id" ]; then
    echo "无法获取倒数第二条reflog的commit ID。"
    exit 1
fi

# 输出commit ID
echo "倒数第二条reflog的commit ID: $commit_id"

git update-ref HEAD $commit_id
git pull
