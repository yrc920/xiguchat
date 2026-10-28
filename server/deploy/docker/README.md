# XiguChat 后端 Docker 部署

把「编译环境 + MySQL + Redis」打包成一个镜像。跑镜像的机器不需要装编译器、
不需要访问 GitHub：镜像里的第三方依赖（grpc / boost / mysql-connector-cpp 等）
已经用 vcpkg 预编译好，容器启动后只需从 git 仓库取代码、编译自己的代码。

---

## 1. 准备

- 安装 Docker（服务器装 `docker.io` 即可，无需其他任何东西）
- 本机直接跑源码（不用容器）时，需要改地址或密钥就把模板复制一份再改：

```bash
cp client/config.ini.example client/config.ini
cp server/services/GateServer/config.ini.example server/services/GateServer/config.ini
cp server/services/StatusServer/config.ini.example server/services/StatusServer/config.ini
cp server/services/VerifyServer/config.ini.example server/services/VerifyServer/config.ini
cp server/services/ChatServer/config/ChatServer.ini.example server/services/ChatServer/config/ChatServer.ini
cp server/services/ChatServer/config/ChatServer2.ini.example server/services/ChatServer/config/ChatServer2.ini
```

> 不改也能直接构建运行，CMake 会在缺 `config.ini` 时自动用模板补一份。

---

## 2. 构建镜像

在 `server` 目录下执行（构建上下文就是 `server` 目录）：

```bash
docker build -f deploy/docker/Dockerfile -t xiguchat-server:amd64 .
```

首次约 1 小时 —— 时间主要花在 vcpkg 编译第三方依赖上；Apple 芯片的 Mac 是在
x86 模拟环境里编译，比同配置的 Linux 机器慢不少。以后再构建会命中缓存。

网络受限导致容器内下载失败时，可把宿主机代理传进去：

```bash
docker build --build-arg PROXY=http://host.docker.internal:7890 \
  -f deploy/docker/Dockerfile -t xiguchat-server:amd64 .
```

---

## 3. 配置环境变量

```bash
cd server/deploy/docker
cp .env.example .env
```

编辑 `.env`：

| 变量 | 说明 |
|---|---|
| `PUBLIC_HOST` | **必须填对**。客户端实际用来访问服务器的地址：本机测试用 `127.0.0.1`，公网部署填服务器公网 IP。`StatusServer` 会把这个地址原样下发给客户端去连聊天服务 |
| `EMAIL_HOST` / `EMAIL_PORT` | SMTP 服务器，默认 `smtp.qq.com` / `465` |
| `EMAIL_USER` / `EMAIL_PASS` | 发信账号与授权码（QQ 邮箱在「设置 → 账号 → POP3/SMTP 服务」生成） |
| `EMAIL_FROM` | 发件人显示地址，通常与 `EMAIL_USER` 相同 |

---

## 4. 启动

```bash
cd server/deploy/docker
docker compose up -d          # 启动
docker compose logs -f        # 看日志，Ctrl+C 退出
docker compose down           # 停止并移除容器（数据卷保留）
```

首次启动容器内依次完成：拉代码 → 初始化 MySQL 并导入 `backup/xigu.sql` → 编译 → 启动五个服务。
看到日志里列出 8080 / 8090 / 8091 等端口在监听，即启动成功。

验证服务活着：

```bash
curl http://<PUBLIC_HOST>:8080/get_test
```

---

## 5. 部署到服务器

服务器不需要代码也不需要编译环境，把镜像搬过去即可：

```bash
# 本地导出
docker save xiguchat-server:amd64 -o xiguchat-server.tar

# 传到服务器
scp xiguchat-server.tar ubuntu@<服务器IP>:/home/ubuntu/

# 服务器上导入并启动
ssh ubuntu@<服务器IP>
cd ~ && docker load -i xiguchat-server.tar
mkdir -p xiguchat && cd xiguchat
# 把 deploy/docker 下的 docker-compose.yml 与填好的 .env 放到这里
docker compose up -d
```

云服务器的安全组 / 防火墙要放行 **8080、8090、8091**，否则外面连不上。

---

## 6. 更新代码

代码推到仓库后，在跑容器的机器上执行一条命令即可（拉代码 → 重新编译 → 重启服务）：

```bash
docker exec xiguchat /opt/xiguchat/scripts/update.sh
```

先编译成功才停旧服务，所以编译失败时正在运行的服务不会被打断。
容器重启时是否自动拉最新代码由环境变量 `AUTO_PULL` 控制（默认 `1`）。

---

## 7. 端口

| 端口 | 用途 | 是否对外开放 |
|---|---|---|
| 8080 | GateServer HTTP 接口 | 是 |
| 8090 / 8091 | 两个聊天实例的 TCP 长连接 | 是 |
| 50051 / 50052 / 50055 / 50056 | 服务间 gRPC | 否，仅容器内部 |
| 33060 | MySQL X 协议 | 否，仅容器内部 |
| 6380 | Redis | 否，仅容器内部 |

---

## 8. 其他环境变量

| 变量 | 默认值 | 说明 |
|---|---|---|
| `XIGU_REPO` | `https://gitee.com/shishangyiwugu/xiguchat.git` | 代码仓库 |
| `XIGU_BRANCH` | `main` | 部署哪个分支（仓库的主分支）；要发布别的分支就改这里 |
| `AUTO_PULL` | `1` | 容器启动时是否拉最新代码 |
| `SKIP_BUILD` | `0` | `1` 表示跳过编译，直接用上次的产物启动 |

---

## 9. 常见问题

- **客户端能登录但连不上聊天服务**：`PUBLIC_HOST` 没填外网能访问的地址。
- **注册时收不到验证码邮件**：`.env` 里的邮件账号或授权码不对；容器日志里能看到 SMTP 报错。
- **改了配置想生效**：改 `server/deploy/docker/config-templates/` 下的模板后重建镜像。
  直接改容器里 `/workspace/.../config.ini` 没用，每次启动都会被模板覆盖。
- **MySQL 数据在哪**：数据卷 `mysql-data`，删容器不影响数据。
