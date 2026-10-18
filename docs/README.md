# XiguChat 技术文档

这里是 XiguChat 的深度技术文档。根目录 `README.md` 讲怎么把项目跑起来，本目录讲内部怎么工作、为什么这样设计。

## 文档索引

| # | 文档 | 内容 |
| --- | --- | --- |
| 01 | [架构设计](01-architecture.md) | 系统分层、进程与线程模型、核心时序、消息路由策略、容错边界 |
| 02 | [构建与部署](02-build.md) | CMake 预设、vcpkg manifest、proto 代码生成、ChatServer 多实例机制、产物布局、三平台差异 |
| 03 | [通信协议](03-protocol.md) | TCP 私有协议帧格式、HTTP 接口清单、gRPC 接口清单、错误码总表 |
| 04 | [数据模型](04-data-model.md) | MySQL 表结构与 `reg_user` 存储过程、Redis 键空间、缓存读写策略 |
| 05 | [配置参考](05-configuration.md) | 各服务 `config.ini` 段落字段说明与常见变更场景 |
| 06 | [客户端](06-client.md) | Qt 客户端分层、网络层、UI 层、自定义控件、资源与 HiDPI 处理 |
| 07 | [GateServer](07-gate-server.md) | HTTP 网关：Beast 连接管理、URL 路由、四个接口的完整逻辑 |
| 08 | [StatusServer](08-status-server.md) | 状态路由：token 签发、最小连接数选路、Login 校验 |
| 09 | [ChatServer](09-chat-server.md) | 长连接服务：会话生命周期、逻辑队列、好友与消息处理、跨实例 gRPC 转发 |
| 10 | [VerifyServer](10-verify-server.md) | 验证码服务：编码生成、Redis 复用策略、SMTP SSL 发信与 CA 解析 |
| 11 | [已知限制](11-known-limitations.md) | 技术债清单、风险点、改进优先级建议 |

## 建议阅读路径

刚接手项目：根目录 `README.md` → `01-架构设计` → `02-构建与部署`，先把服务跑起来。

要改协议或加功能：`03-通信协议` → 对应服务文档（07~10）→ `04-数据模型`。

排查问题：`05-配置参考` 核对配置 → `11-已知限制` 确认是不是已知问题 → 进对应服务文档看实现。

图都用 Mermaid 画，GitHub、GitLab、VS Code（装 Markdown Preview Mermaid 支持）可以直接渲染。

## 文档约定

- 文档中的端口、地址、密码均为仓库默认配置（`server/services/*/config.ini`），生产部署需自行修改。
- 涉及平台差异时统一按 Windows / macOS / Linux 三平台描述。
- 文档描述的是仓库 HEAD 状态的实现；与代码不一致时以代码为准，并请顺手修正文档。
