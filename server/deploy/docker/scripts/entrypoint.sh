#!/bin/bash
# 容器入口：启动 Redis -> 准备代码 -> 初始化数据库 -> 编译 -> 启动服务
#
# 可用环境变量：
#   PUBLIC_HOST   客户端实际访问本服务的地址
#   EMAIL_*       验证码邮件的发送账号，逐项替换 VerifyServer 模板里的占位符
#   XIGU_REPO     代码仓库地址
#   XIGU_BRANCH   代码分支（默认 main）
#   AUTO_PULL     启动时是否拉取最新代码，1=拉，0=不拉
#   SKIP_BUILD    是否跳过编译，1=跳过，直接用现有产物启动
set -uo pipefail

export PUBLIC_HOST="${PUBLIC_HOST:-127.0.0.1}"
export EMAIL_HOST="${EMAIL_HOST:-smtp.qq.com}"
export EMAIL_PORT="${EMAIL_PORT:-465}"
export EMAIL_USER="${EMAIL_USER:-}"
export EMAIL_PASS="${EMAIL_PASS:-}"
export EMAIL_FROM="${EMAIL_FROM:-${EMAIL_USER}}"
export XIGU_REPO="${XIGU_REPO:-https://gitee.com/shishangyiwugu/xiguchat.git}"
export XIGU_BRANCH="${XIGU_BRANCH:-main}"
export AUTO_PULL="${AUTO_PULL:-1}"
export SKIP_BUILD="${SKIP_BUILD:-0}"

SCRIPT_DIR=/opt/xiguchat/scripts

echo "============================================================"
echo " XiguChat 后端启动"
echo "   对外地址  : ${PUBLIC_HOST}"
echo "   代码分支  : ${XIGU_BRANCH}"
echo "   自动拉代码: ${AUTO_PULL}"
echo "============================================================"

# Redis 端口与密码与项目配置保持一致（6380 / 123456）
if ! redis-cli -p 6380 ping >/dev/null 2>&1; then
    redis-server --port 6380 --requirepass 123456 --daemonize yes \
                 --dir /var/lib/redis --appendonly no
fi
sleep 1
echo "[entrypoint] redis ping: $(redis-cli -p 6380 -a 123456 ping 2>/dev/null)"

echo "[entrypoint] 1/4 准备代码"
bash "${SCRIPT_DIR}/prepare_code.sh"

echo "[entrypoint] 2/4 初始化数据库"
bash "${SCRIPT_DIR}/init_db.sh"

if [ "${SKIP_BUILD}" = "1" ]; then
    echo "[entrypoint] 3/4 跳过编译（SKIP_BUILD=1）"
else
    echo "[entrypoint] 3/4 编译"
    bash "${SCRIPT_DIR}/build.sh"
fi

echo "[entrypoint] 4/4 启动服务"
exec bash "${SCRIPT_DIR}/start_stack.sh"
