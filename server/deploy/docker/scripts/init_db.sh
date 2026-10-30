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

# 首次启动时 root 尚无口令，先补上口令
if mysql -uroot -e 'SELECT 1' >/dev/null 2>&1; then
    mysql -uroot <<'SQL'
ALTER USER 'root'@'localhost' IDENTIFIED WITH mysql_native_password BY '123456';
SQL
fi

# 远程账号：经发布端口从容器外连进来时源地址不是 localhost，需要单独授权。
# 每次启动都执行，幂等。
mysql -uroot -p"${ROOT_PWD}" <<'SQL'
CREATE USER IF NOT EXISTS 'root'@'%' IDENTIFIED WITH mysql_native_password BY '123456';
GRANT ALL PRIVILEGES ON *.* TO 'root'@'%' WITH GRANT OPTION;
CREATE DATABASE IF NOT EXISTS xigu DEFAULT CHARACTER SET utf8mb4;
FLUSH PRIVILEGES;
SQL
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
