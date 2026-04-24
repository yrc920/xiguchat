# XiguChat

XiguChat 是一个基于 C++ 和 Node.js 开发的跨平台即时通讯（IM）软件，致力于实现类似 QQ 的核心聊天与社交体验。项目采用微服务化的后端架构以及基于 Qt 的丰富客户端界面。

## 🚀 当前项目进度

- [x] 微服务架构搭建
- [x] 客户端基础 UI 框架 (登录、注册界面)
- [x] 邮件验证码下发与校验 (VerifyServer 已跑通)
- [ ] 用户注册与登录逻辑
- [ ] 核心网关服务 (GateServer 路由转发)
- [ ] 实时单聊 / 群聊功能

## 🛠️ 技术栈

本项目采取完全的前后端分离与后端微服务设计，主要技术栈如下：

### 客户端 (Client)

- **语言框架**: C++, Qt 5.15+ (MinGW 64-bit)
- **UI 样式**: 原生 UI 组件 + QSS (Qt Style Sheets) 客制化
- **网络通信**: 基于 HTTP/RPC 的业务请求与后续的长连接管理

### 服务端 (Server)

服务端分为多个独立的微服务：

1. **GateServer (网关服务)**
   - **语言**: C++ (Visual Studio 2022)
   - **核心技术**: gRPC、Protobuf、HTTP 处理。负责处理客户端连接、协议解析以及向内部请求的分发。
2. **VerifyServer (验证服务)**
   - **语言**: Node.js
   - **核心技术**: gRPC、邮件发送模块 (`email.js`)。负责处理用户注册时的验证码下发与比对等安全验证。

## 📁 目录结构

```text
xiguchat/
├── client/                 # C++ Qt 客户端代码
│   ├── build/              # 编译输出目录
│   ├── res/                # 资源文件 (图标、图片等)
│   ├── style/              # QSS 样式文件
│   └── xiguchat/           # Qt UI 及核心逻辑 (如 MainWindow, LoginDialog)
├── server/                 # 后端服务代码
│   ├── GateServer/         # C++ 网关与逻辑分发服务器 (VS 解决方案)
│   └── VerifyServer/       # Node.js 邮件与验证码服务
├── README.md               # 项目说明文档
└── .gitignore              # Git 忽略配置
```

## ⚙️ 快速启动

### 1. 启动 VerifyServer (验证服务)

本服务负责基于 Node.js 发送注册邮箱验证码。

```bash
cd server/VerifyServer
# 安装依赖
pnpm install
# 根据 config.example.json 配置您的 config.json (配置邮件SMTP服务器、端口等)
# 启动服务
node server.js
```

### 2. 启动 GateServer (网关服务)

1. 使用 Visual Studio 打开 GateServer.slnx。
2. 确保已经配置好 `Protobuf` 与 `gRPC` 的 C++ 依赖环境。
3. 编译并运行项目主程序。

### 3. 运行 Client (客户端)

1. 使用 Qt Creator 打开 xiguchat.pro。
2. 配置项目构建环境（推荐桌面端 MinGW 64-bit）。
3. 构建并运行，即可看到应用的登录与注册主界面。

## 📄 开源协议

本项目遵循 MIT License 开源协议。
