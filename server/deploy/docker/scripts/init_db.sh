#!/bin/bash
# 初始化 MySQL：设置 root 口令、建库 xigu，库为空时导入 backup/xigu.sql
set -uo pipefail

MYSQL_DATA=/var/lib/mysql
ROOT_PWD=123456
SQL_FILE=/workspace/backup/xigu.sql

wait_ready() {
    for _ in $(seq 1 90); do
        mysqladmin ping >/dev/null 2>&1 && return 0
        sleep 1
    done
    return 1
}

# 数据目录为空说明是第一次启动
if [ -z "$(ls -A "${MYSQL_DATA}" 2>/dev/null)" ]; then
    echo "[mysql] 首次启动，初始化数据目录"
    chown -R mysql:mysql "${MYSQL_DATA}"
    mysqld --initialize-insecure --user=mysql --datadir="${MYSQL_DATA}"
fi

mkdir -p /var/run/mysqld && chown mysql:mysql /var/run/mysqld
mysqladmin ping >/dev/null 2>&1 \
  || mysqld --user=mysql --datadir="${MYSQL_DATA}" --daemonize

if ! wait_ready; then
    echo "[mysql] 错误：mysqld 未能启动"
    tail -n 40 /var/log/mysql/error.log 2>/dev/null
    exit 1
fi
echo "[mysql] mysqld 已就绪"

# root 在首次启动时还没有口令，之后再启动已经有了，两种情况都要通过校验
if mysql -uroot -e 'SELECT 1' >/dev/null 2>&1; then
    mysql -uroot <<'SQL'
ALTER USER 'root'@'localhost' IDENTIFIED WITH mysql_native_password BY '123456';
CREATE USER IF NOT EXISTS 'root'@'127.0.0.1' IDENTIFIED WITH mysql_native_password BY '123456';
GRANT ALL PRIVILEGES ON *.* TO 'root'@'127.0.0.1' WITH GRANT OPTION;
CREATE DATABASE IF NOT EXISTS xigu DEFAULT CHARACTER SET utf8mb4;
FLUSH PRIVILEGES;
SQL
fi
if ! mysql -uroot -p"${ROOT_PWD}" -e 'SELECT 1' >/dev/null 2>&1; then
    echo "[mysql] 错误：root 账号不可用"
    exit 1
fi

TABLES=$(mysql -uroot -p"${ROOT_PWD}" -N -B \
    -e "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema='xigu'")
if [ "${TABLES:-0}" = "0" ] && [ -f "${SQL_FILE}" ]; then
    echo "[mysql] 导入初始数据"
    mysql -uroot -p"${ROOT_PWD}" xigu < "${SQL_FILE}"
fi

XPORT=$(mysql -uroot -p"${ROOT_PWD}" -N -B -e "SELECT @@mysqlx_port")
echo "[mysql] 库已就绪，X 协议端口 ${XPORT}"
