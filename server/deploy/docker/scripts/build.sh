#!/bin/bash
# 编译项目。第三方依赖已预编译，这里只编译自己的代码
set -uo pipefail

SRC=/workspace/server
JOBS=$(nproc)

cd "${SRC}" || exit 1

echo "[build] 配置 CMake"
cmake --preset linux-docker || exit 1

echo "[build] 编译（-j ${JOBS}）"
cmake --build --preset linux-docker --parallel "${JOBS}" || exit 1

echo "[build] 产物："
ls -1 "${SRC}/build/linux-docker/bin"
