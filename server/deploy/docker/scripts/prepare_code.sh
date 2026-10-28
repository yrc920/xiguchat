#!/bin/bash
# 准备源码：/workspace 为空则从 XIGU_REPO 克隆，已有仓库则拉取 XIGU_BRANCH 的最新代码
set -uo pipefail

REPO="${XIGU_REPO:-https://gitee.com/shishangyiwugu/xiguchat.git}"
BRANCH="${XIGU_BRANCH:-main}"
AUTO_PULL="${AUTO_PULL:-1}"
WORK=/workspace

if [ ! -d "${WORK}/.git" ]; then
    mkdir -p "${WORK}"
    echo "[code] 克隆 ${REPO}（分支 ${BRANCH}）"
    git clone --branch "${BRANCH}" --depth 1 "${REPO}" "${WORK}/.clone" >/dev/null || exit 1
    shopt -s dotglob
    mv "${WORK}/.clone"/* "${WORK}/"
    rmdir "${WORK}/.clone"
    shopt -u dotglob
elif [ "${AUTO_PULL}" = "1" ]; then
    echo "[code] 拉取最新代码"
    git -C "${WORK}" fetch --depth 1 origin "${BRANCH}" >/dev/null \
      && git -C "${WORK}" reset --hard "origin/${BRANCH}" >/dev/null \
      || echo "[code] 拉取失败，沿用现有代码"
fi

if [ ! -f "${WORK}/server/CMakeLists.txt" ]; then
    echo "[code] 错误：${WORK}/server 下没有 CMakeLists.txt"
    exit 1
fi
echo "[code] 代码就绪：$(git -C "${WORK}" rev-parse --short HEAD 2>/dev/null || echo unknown)"
