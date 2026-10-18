# 11 · 已知限制与技术债

本文档如实记录当前实现（仓库 HEAD）的功能缺口、设计妥协与重复代码，供后续迭代参考。
每条都标注了**影响面**与**建议的改进方向**。

---

## 11.1 安全类

| # | 问题 | 影响 | 改进方向 |
| --- | --- | --- | --- |
| S1 | 密码在 HTTP 链路上仅经客户端 `xorString()` 做**可逆 XOR 变形**（按长度取模，只处理前 `len % 255` 个字符），网关未启用 TLS | 任何中间人可还原明文密码 | 网关启用 HTTPS（Beast 支持 `ssl_stream`）；客户端改为在 TLS 之上传输，或至少改用不可逆的服务端挑战-响应 |
| S2 | 服务间 gRPC 全部使用 `InsecureChannelCredentials` | 内网可信前提下的设计，跨机房部署不适用 | 启用 TLS 或 mTLS |
| S3 | Token（UUID）**无有效期、无刷新、无吊销**；`utoken_<uid>` 永不过期 | token 泄露即长期有效 | `SETEX` + 刷新机制；登出时主动删除 |
| S4 | 登录回包与用户缓存中包含 `pwd`（Argon2id 哈希串） | 扩大敏感数据暴露面 | 从 `GetBaseInfo` 与 1006 回包中移除 `pwd` |
| S5 | 验证码无发送频率限制 | 同一邮箱可被滥用于邮件轰炸 | 按邮箱 + IP 做限流（Redis 计数器 + TTL） |
| S6 | 注册/改密成功后未删除 `code_<email>` | 10 分钟内验证码可重复使用 | 成功后 `DEL` |
| S7 | 好友申请与同意之间无事务 | 中途失败产生"申请已同意但未成为好友"的中间态 | 用事务包裹 `AuthFriendApply` + `AddFriend` |

## 11.2 可靠性类

| # | 问题 | 影响 | 改进方向 |
| --- | --- | --- | --- |
| R1 | 无心跳机制 | TCP 半开连接无法及时发现，`logincount` 虚高，选路失真；服务端资源泄漏 | 增加应用层心跳（如每 30s 一次 ping/pong）+ 读超时 |
| R2 | 客户端无断线重连 | 网络抖动后需重启应用 | `TcpMgr` 增加重连退避与自动重新登录（复用 token） |
| R3 | 无离线消息 | 目标不在线时消息直接丢弃 | 增加消息表 + 离线队列，登录时拉取 |
| R4 | 无消息送达确认 | 客户端收到 1018 回执后未做任何状态标记（源码留有 TODO） | 增加"发送中 / 已送达 / 已读"状态与 UI 展示 |
| R5 | 跨实例 RPC 失败即丢弃 | 对端实例不可达时消息静默丢失 | 失败重试 + 降级为离线消息 |
| R6 | 无服务探活 | StatusServer 无法感知实例已崩溃，仍可能分配 | 心跳上报 + 过期剔除（`logincount` 加时间戳） |
| R7 | `uip_<uid>` 断连时不删除 | 离线用户残留过期位置值，造成一次无效的跨实例 RPC | 断连时 `DEL`（需先确认无多端登录） |
| R8 | 用户资料缓存无失效 | 修改昵称/头像/密码后缓存不更新 | 更新时同步删除 `ubaseinfo_<uid>` / `nameinfo_<name>`，或加 TTL |

## 11.3 性能类

| # | 问题 | 影响 | 改进方向 |
| --- | --- | --- | --- |
| P1 | GateServer 的 HTTP handler **同步阻塞**：业务处理（gRPC + MySQL）直接在 IO 线程执行 | 并发吞吐受限于 `IOContextPool` 线程数 | 把阻塞操作投递到独立线程池，或改为异步链路 |
| P2 | ChatServer 逻辑层**单线程串行** | 单实例消息处理吞吐有上限；慢查询会阻塞整个队列 | 按 uid 分片多队列 + 多消费者（需同步给 `UserMgr` 加锁） |
| P3 | 每次选路对**每个实例**发起一次 `HGET` | O(n) 次 Redis 往返 | 一次 `HGETALL logincount` 后在内存比较 |
| P4 | MySQL 访问未体现批量/预编译复用 | 单次请求开销偏大 | 视需要引入批量查询与语句缓存 |
| P5 | 用户搜索返回完整资料（含 `pwd`） | 无用字段传输与缓存 | 拆分"搜索结果"与"完整资料"两套结构 |

## 11.4 代码结构类

| # | 问题 | 位置 | 改进方向 |
| --- | --- | --- | --- |
| C1 | `GetBaseInfo` 逻辑在 **三处重复** | `LogicSystem`、`ChatServiceImpl`、`ChatGrpcClient`（ChatServer） | 收敛到 `UserMgr` 或独立的 `UserCache` 组件 |
| C2 | `ErrorCodes` 枚举在 GateServer / StatusServer / ChatServer **各自复制一份**（1001–1009，ChatServer 多 1010/1011） | 三处 `include/const.h` | 抽到公共头，避免漏改导致语义漂移 |
| C3 | `IOContextPool` / `ConfigMgr` / `RedisMgr` / `MysqlMgr` / `MysqlDao` / `Singleton` 在四个服务中**逐份复制** | `services/*/` | 抽取为公共静态库，各服务链接；代价是服务间的独立性下降，需权衡 |
| C4 | `StatusServer::ChatGrpcClient` 为**死代码**，且构造用的是 TCP 端口而非 RPC 端口 | `services/StatusServer/` | 删除，或修正端口语义后在需要时启用 |
| C5 | `ChatServer::StatusGrpcClient` 已链接但**未被调用**；`StatusService::Login` 与 ChatServer 自行读 Redis 校验 token 逻辑重复 | `services/ChatServer/`、`services/StatusServer/` | 统一为"ChatServer 调 StatusServer::Login"，收敛校验入口 |
| C6 | proto 中 `RplyAddFriend`、`SendChatMsg` 已定义但无实现/无调用 | `proto/message.proto` | 实现或删除 |
| C7 | `IPCOUNTPREFIX`（`ipcount_`）等常量已无引用 | `ChatServer/include/const.h` | 清理 |
| C8 | `LogicSystem` handler 返回值 `bool` 未被使用 | GateServer | 接口简化或真正使用返回值表达"是否成功" |

## 11.5 功能缺口

| # | 缺失 | 说明 |
| --- | --- | --- |
| F1 | 群聊 | 只有单聊；`grouptipitem` 仅作为列表分组提示存在 |
| F2 | 图片 / 文件消息 | 客户端 `MessageTextEdit` 支持插入，服务端只处理文本；`MsgInfo.msgFlag` 的 `image` / `file` 分支未投递 |
| F3 | 聊天记录持久化 | 无任何消息表，聊天记录只在客户端内存，重启即丢失 |
| F4 | 多端登录 | `uip_<uid>` 只保留最后一个登录位置；多端会互相踢掉（后登录覆盖） |
| F5 | 好友删除 / 黑名单 | 只有添加与同意 |
| F6 | 头像上传 | `icon` 只能是资源路径，无上传与存储 |
| F7 | 消息历史分页拉取 | 登录只下发好友列表，无历史消息 |
| F8 | 国际化 | 客户端文案混用中文与英文（如 `strs` 常量），`tr()` 未全面覆盖 |

## 11.6 改进优先级建议

按"风险 × 成本"排序，建议的推进顺序：

1. **S1 / S3 / S6**（安全，成本低）：HTTP 上 TLS、token 加 TTL 与登出删除、验证码用后即删。
2. **R1 / R2**（可靠性，成本中）：心跳 + 断线重连，直接决定用户体验。
3. **C1 / C2 / C4 / C5**（结构，成本低）：消除重复实现与死代码，降低后续改动出错概率。
4. **R3 / R4 / F3**（功能，成本中高）：离线消息与聊天记录，需要新增表与投递链路。
5. **P1 / P2**（性能，成本高）：异步化与多队列，建议在单机吞吐成为实际瓶颈后再做。

> 本项目的定位是学习与工程实践，上述缺口多数是"有意留白"而非缺陷；
> 若要用于生产环境，至少需完成第 1 组（S1/S3/S6）与第 2 组（R1/R2）。
