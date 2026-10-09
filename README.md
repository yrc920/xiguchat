# XiguChat

一个类似微信的聊天项目：Qt 客户端 + gRPC 微服务后端。

项目采用 CMake 统一构建（前端 Qt 工程与后端 C++ 服务均为 CMake 工程，当前在 Windows 平台下运行验证通过）。

## 项目结构

```
chat
├── .gitignore
├── LICENSE                 # GPL-3.0 开源协议
├── backup\                 # 数据库备份（不入库）
├── client\                 # 前端：Qt 5.15.2 桌面客户端
│   ├── CMakeLists.txt      # CMake 工程
│   ├── CMakePresets.json   # windows-msvc 预设
│   ├── main.cpp
│   ├── core\               # 全局定义、单例与公共数据结构
│   ├── network\            # HTTP / TCP 长连接通信层
│   ├── widgets\            # 自定义控件（气泡、列表、输入框等）
│   ├── dialogs\            # 业务对话框与页面（登录、注册、聊天等）
│   ├── views\              # 主窗口
│   ├── resources\          # 图标与资源文件
│   ├── static\             # 运行时静态资源
│   └── config.ini          # 客户端配置
└── server\                 # 后端：C++ gRPC 微服务 + Node 验证服务
    ├── CMakeLists.txt      # 顶层 CMake
    ├── CMakePresets.json   # windows-msvc 预设（Debug/Release）
    ├── vcpkg.json          # vcpkg manifest 依赖清单
    ├── cmake\XiguCommon.cmake  # 公共部署规则（config.ini + 运行时 DLL）
    ├── proto\              # 共享 proto 单一源（构建期生成 C++ 代码）
    ├── services\
    │   ├── GateServer\     # HTTP 网关
    │   ├── StatusServer\   # 在线状态与路由
    │   ├── ChatServer\     # 聊天服务实例 1
    │   ├── ChatServer2\    # 聊天服务实例 2
    │   └── VerifyServer\   # Node.js 验证服务（gRPC）
    └── build\windows-msvc\ # 后端构建产物
```

## 技术栈

| 部分      | 技术                                                                                                 |
| --------- | ---------------------------------------------------------------------------------------------------- |
| 前端      | Qt 5.15.2（Core / Gui / Network / Widgets）、C++17                                                   |
| 后端 C++  | C++20、gRPC、protobuf、boost::asio / boost::beast、hiredis、jsoncpp、mysql-connector-cpp（X DevAPI） |
| 后端 Node | Node.js + gRPC                                                                                       |
| 数据库    | MySQL、Redis                                                                                         |
| 构建      | CMake（前后端统一）、vcpkg（后端依赖管理）                                                           |

## 系统架构

```
客户端 (Qt)
   │  TCP 长连接 (8090 / 8091)
   ▼
GateServer (HTTP 8080)  ──gRPC──►  VerifyServer (50051)
   │                                StatusServer (50052)
   ▼
ChatServer  (TCP 8090 + RPC 50055)
ChatServer2 (TCP 8091 + RPC 50056)
```

- **GateServer**：对外 HTTP 网关，处理注册、登录等请求，转发 gRPC 到 VerifyServer / StatusServer
- **VerifyServer**（Node）：验证码生成与校验（gRPC）
- **StatusServer**：维护用户在线状态，将消息路由到对应聊天实例（gRPC）
- **ChatServer / ChatServer2**：两个独立实例（多线程），处理客户端 TCP 长连接与消息收发，服务间通过 gRPC 互通

## 构建与运行（Windows）

### 前端（Qt 客户端）

用 **Qt Creator** 打开 `client\CMakeLists.txt`，选择 Qt 5.15.2 kit 构建运行。
产物：`client\build\windows-msvc\bin\xiguchat.exe`

### 后端（4 个 C++ 服务）

1. 用 VSCode 打开 `server` 目录（`.vscode\settings.json` 已配置 CMake）
2. CMake 插件选择 `windows-msvc` 预设构建（Debug/Release）
3. 产物：`server\build\windows-msvc\bin\<服务名>\Debug\<服务名>.exe`
4. 依次运行：`StatusServer.exe` → `ChatServer.exe` / `ChatServer2.exe` → `GateServer.exe`

命令行等价方式：

```
cd server
cmake --preset windows-msvc
cmake --build --preset windows-msvc-debug
```

依赖：VS2026 MSVC（v143+）、cmake >= 3.20、vcpkg（manifest 自动安装依赖）。

### Node 验证服务（单独启动）

VerifyServer 不参与 CMake 构建，单独运行：

```
cd server\services\VerifyServer
node src\server.js
```

（首次需 `pnpm install` / `npm install` 安装依赖）

## 端口一览

| 服务         | 协议/端口            |
| ------------ | -------------------- |
| VerifyServer | gRPC 50051           |
| StatusServer | RPC 50052            |
| ChatServer   | TCP 8090 + RPC 50055 |
| ChatServer2  | TCP 8091 + RPC 50056 |
| GateServer   | HTTP 8080            |

## 外部依赖

运行前需保证以下服务在线：

- MySQL：`127.0.0.1:3306`（root/123456）
- Redis：`127.0.0.1:6380`

## 编码约定

项目全部使用 **UTF-8（无 BOM）**编码；批处理脚本保持纯英文内容。

## 开源协议

本项目采用 **GNU GPL-3.0** 开源协议，见 [LICENSE](LICENSE)。
