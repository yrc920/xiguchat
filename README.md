# XiguChat

一个跨平台的微信风格聊天项目：Qt 6 客户端 + gRPC 微服务后端。

支持 **Windows（MinGW/MSVC）· macOS（AppleClang）· Linux（GCC）** 三平台本地构建运行，
前端用 Qt Creator 或 CMake 预设打开，后端由 CMakePresets + vcpkg manifest 统一构建。

## 项目结构

```
xiguchat
├── .gitignore
├── LICENSE                 # GPL-3.0 开源协议
├── backup/                 # 数据库备份（不入库）
├── client/                 # 前端：Qt 6.5 桌面客户端
│   ├── CMakeLists.txt      # CMake 工程
│   ├── CMakePresets.json   # 三平台预设（windows-mingw / macos-clang / linux-gcc）
│   ├── main.cpp
│   ├── core/               # 全局定义、单例与公共数据结构
│   ├── network/            # HTTP / TCP 长连接通信层
│   ├── widgets/            # 自定义控件（气泡、列表、输入框等）
│   ├── dialogs/            # 业务对话框与页面（登录、注册、聊天等）
│   ├── views/              # 主窗口
│   ├── resources/          # 图标与资源文件（qrc + .ico/.icns）
│   ├── static/             # 运行时静态资源
│   └── config.ini          # 客户端配置（网关地址）
└── server/                 # 后端：C++ gRPC 微服务 + Node 验证服务
    ├── CMakeLists.txt      # 顶层 CMake
    ├── CMakePresets.json   # 三平台预设（windows-msvc / macos-clang / linux-gcc，Debug/Release）
    ├── vcpkg.json          # vcpkg manifest 依赖清单
    ├── cmake/XiguChatCommon.cmake  # 公共部署规则（config.ini + 运行时 DLL）
    ├── proto/              # 共享 proto 单一源（构建期生成 C++ 代码）
    └── services/
        ├── GateServer/     # HTTP 网关（8080）
        ├── StatusServer/   # 在线状态与路由（50052）
        ├── ChatServer/     # 聊天服务（一套代码多实例）
        │   └── config/     # 每个 ini 对应一个实例：ChatServer.ini / ChatServer2.ini
        └── VerifyServer/   # Node.js 验证码服务（gRPC 50051）
```

> **ChatServer 是一套代码、多实例部署**：`config/` 目录下每份 `*.ini` 会在构建后生成
> 一个独立的运行目录 `bin/<实例名>/`（内含可执行文件与本实例的 config.ini）。
> 新增第三个实例只需再放一份 ini，无需改动 CMake。

## 技术栈

| 部分      | 技术                                                                                                 |
| --------- | ---------------------------------------------------------------------------------------------------- |
| 前端      | Qt 6.5（Core / Gui / Network / Widgets）、C++17                                                      |
| 后端 C++  | C++20、gRPC、protobuf、boost::asio / boost::beast、hiredis、jsoncpp、mysql-connector-cpp（X DevAPI） |
| 后端 Node | Node.js + gRPC（VerifyServer）                                                                       |
| 数据库    | MySQL（3306）、Redis（6380）                                                                          |
| 构建      | CMake ≥ 3.21 + Ninja、vcpkg（后端依赖管理）                                                            |

## 系统架构

```
客户端 (Qt)
   │  TCP 长连接 (8090 / 8091)
   ▼
GateServer (HTTP 8080)  ──gRPC──►  VerifyServer (50051)
   │                                StatusServer (50052)
   ▼
ChatServer 实例 chatserver1 (TCP 8090 + RPC 50055)
ChatServer 实例 chatserver2 (TCP 8091 + RPC 50056)
        └────────── gRPC 互通 ──────────┘
```

- **GateServer**：对外 HTTP 网关，处理注册、登录等请求，转发 gRPC 到 VerifyServer / StatusServer
- **VerifyServer**（Node）：验证码生成与校验（gRPC）
- **StatusServer**：维护用户在线状态，按最小连接数把客户端分配到某个聊天实例（gRPC）
- **ChatServer**：处理客户端 TCP 长连接与消息收发（多线程）。代码只有一份，可同时起多
  个实例；不同实例通过 Redis 的 `uip_<uid>` 判断收件人在哪个实例，本实例内直投，
  跨实例则经 gRPC 转发

## 环境准备（三平台通用）

| 工具 | 要求 |
| ---- | ---- |
| CMake | ≥ 3.21（CMakePresets / 运行时 DLL 部署所需） |
| Ninja | 后端 macos-clang / linux-gcc 预设需要 |
| vcpkg | 后端依赖管理，需设置环境变量 `VCPKG_ROOT` |
| Qt | 6.5.x（前端） |

> 后端依赖由 `server/vcpkg.json` 的 manifest 模式自动安装，无需手动装包。
> 前端不依赖 vcpkg，Qt 路径通过 `QT_ROOT` 环境变量或 Qt Creator 的 Kit 提供。

## 构建与运行

### 数据库（Docker，可选本机安装）

```bash
# MySQL：端口 3306，root/123456，库 xigu（utf8mb4）
docker run -d --name chat-mysql -p 3306:3306 -p 33060:33060 \
  -e MYSQL_ROOT_PASSWORD=123456 mysql:latest

# Redis：项目配置使用 6380 端口，密码 123456
docker run -d --name chat-redis -p 6380:6379 -v redis-data:/data \
  redis:latest redis-server --requirepass 123456
```

首次运行需创建库并导入 `backup/xigu.sql`（备份不入库，可从仓库备份恢复）。

### 前端（客户端）

**Qt Creator（推荐）**：打开 `client/CMakeLists.txt`，选择本机 Qt 6.5 Kit 构建运行。
Windows 使用 **MinGW** Kit；macOS / Linux 使用系统编译器 Kit。

**命令行**（三平台同一套预设）：

```bash
cd client
# Windows:  需设置 QT_ROOT（Qt 6.5 目录）与 QT_MINGW_ROOT（Qt 配套 MinGW）
#           如 setx QT_ROOT "D:\Qt\6.5.3\mingw_64"
#             setx QT_MINGW_ROOT "D:\Qt\Tools\mingw1120_64"
#           QT_MINGW_ROOT 必须是与 Qt 版本对应的 MinGW（Qt 6.5 对应 11.2，
#           即 Qt 安装包自带的 mingw1120_64），其他版本 ABI 不兼容
cmake --preset windows-mingw     # 或 windows-mingw-release
cmake --build --preset windows-mingw
# macOS:
cmake --preset macos-clang       # 或 macos-clang-release
cmake --build --preset macos-clang
# Linux:
cmake --preset linux-gcc         # 或 linux-gcc-release
cmake --build --preset linux-gcc
```

产物：`build/<预设>/bin/xiguchat`（macOS 为 `xiguchat.app`）。

### 后端（3 个 C++ 服务）

```bash
cd server
export VCPKG_ROOT=/path/to/vcpkg      # Windows: set VCPKG_ROOT=...
cmake --preset windows-msvc           # macOS: macos-clang | Linux: linux-gcc
cmake --build --preset windows-msvc-debug
```

产物：`build/<预设>/bin/<服务名>/`（已自动拷贝 config.ini 与运行时 DLL）。
其中 ChatServer 会按 `config/*.ini` 生成多个实例目录：

```
bin/ChatServer/      # 实例 chatserver1：可执行文件 + config.ini
bin/ChatServer2/     # 实例 chatserver2：可执行文件副本 + config.ini
```

依次运行（每个服务须在其自身目录下启动，config.ini 按当前工作目录读取）：

```
bin/StatusServer/StatusServer      # 先启动
bin/ChatServer/ChatServer          # 再启动两个聊天实例（同一份可执行文件）
bin/ChatServer2/ChatServer
bin/GateServer/GateServer          # 最后启动网关
```

> 两个聊天实例用的是**同一个可执行文件**，区别只在各自目录下的 config.ini
> （`SelfServer` 的 Name / Port / RPCPort 与 `PeerServer` 指向）。
> 想加第三个实例：复制一份 `services/ChatServer/config/ChatServer.ini` 改名为
> `ChatServer3.ini`、改掉端口与 Name，重新 configure 即会自动生成 `bin/ChatServer3/`，
> 同时记得把新实例名加进 `StatusServer/config.ini` 的 `[chatservers]` 列表。

### 验证码服务（Node，单独启动）

注册 / 重置密码依赖 VerifyServer，首次运行需安装依赖：

```bash
cd server/services/VerifyServer
pnpm install        # 或 npm install
node src/server.js
```

## 端口一览

| 服务                      | 协议/端口            |
| ------------------------- | -------------------- |
| VerifyServer              | gRPC 50051           |
| StatusServer              | RPC 50052            |
| ChatServer 实例 chatserver1 | TCP 8090 + RPC 50055 |
| ChatServer 实例 chatserver2 | TCP 8091 + RPC 50056 |
| GateServer                | HTTP 8080            |

## 外部依赖

运行前需保证以下服务在线：

- MySQL：`127.0.0.1:3306`（root/123456，Schema `xigu`）
- Redis：`127.0.0.1:6380`（密码 123456）

## 编码约定

- 全部源码使用 **UTF-8（无 BOM）**编码，中文注释与文案均按 UTF-8 保存
- 跨平台分支（`WIN32` / `APPLE` / `UNIX`）注释说明均与三平台行为对应

## 开源协议

本项目采用 **GNU GPL-3.0** 开源协议，见 [LICENSE](LICENSE)。
