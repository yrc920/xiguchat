#!/bin/bash
# 拉取最新代码 -> 重新编译 -> 用新编译产物重启服务。
# 编译失败时直接中止，已在运行的服务不受影响。
#
# 用法：docker exec <容器名> /opt/xiguchat/scripts/update.sh
set -uo pipefail

AUTO_PULL=1 bash /opt/xiguchat/scripts/prepare_code.sh
bash /opt/xiguchat/scripts/build.sh || { echo "[update] 编译失败，服务未做变更"; exit 1; }

echo "[update] 停止现有服务"
pkill -x GateServer   >/dev/null 2>&1
pkill -x StatusServer >/dev/null 2>&1
pkill -x ChatServer   >/dev/null 2>&1
pkill -x VerifyServer >/dev/null 2>&1
sleep 1

# 后台启动：避免服务长驻导致 docker exec 卡住不返回
nohup bash /opt/xiguchat/scripts/start_stack.sh > /var/log/xigu/start.log 2>&1 &
sleep 5
echo "[update] 已重启，完整日志：/var/log/xigu/start.log"
tail -n 15 /var/log/xigu/start.log
