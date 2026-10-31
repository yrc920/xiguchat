# 03 · 通信协议

系统使用三种协议，各自承担不同职责：

| 协议 | 参与方 | 用途 | 编码 |
| --- | --- | --- | --- |
| HTTP/1.1 | Client ↔ GateServer | 注册、登录、改密、获取验证码 | JSON（`text/json`） |
| TCP 自定义帧 | Client ↔ ChatServer | 长连接实时消息与通知 | 4 字节头 + JSON 体 |
| gRPC | 服务之间 | 验证码、状态路由、跨实例消息转发 | protobuf（`server/proto/message.proto`） |

---

## 3.1 TCP 私有协议（Client ↔ ChatServer）

### 帧格式

```
 0        1        2        3        4 ...
+--------+--------+--------+--------+------------------+
|     msg_id      |   msg_len      |      payload      |
|  (2 bytes, BE)  |  (2 bytes, BE) |  (msg_len bytes)  |
+--------+--------+--------+---------------------------+
|<---------- HEAD_TOTAL_LEN = 4 --->|
```

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `msg_id` | `uint16`，**网络字节序（大端）** | 消息类型，见消息 ID 表 |
| `msg_len` | `uint16`，**网络字节序** | **仅 payload 长度，不含头部** |
| `payload` | UTF-8 JSON | 业务数据 |

约束（服务端 `const.h`）：

| 常量 | 值 | 含义 |
| --- | --- | --- |
| `HEAD_TOTAL_LEN` | 4 | 头部总长度 |
| `HEAD_ID_LEN` | 2 | msg_id 长度 |
| `HEAD_DATA_LEN` | 2 | msg_len 长度 |
| `MAX_LENGTH` | 2048 | 单帧（头 + 体）上限，同时也是接收缓冲区大小 |

服务端解析流程（`CSession::AsyncReadHead`）：

1. `asyncReadFull(4)` 保证读满头部（内部用 `async_read_some` 循环补齐，应对 TCP 拆包）；
2. `network_to_host_short` 转换两个字段；
3. `msg_id > MAX_LENGTH` 或 `msg_len > MAX_LENGTH` 视为非法帧，直接 `ClearSession`；
4. 分配 `RecvNode(msg_len, msg_id)`，继续 `AsyncReadBody(msg_len)`；
5. 读完 payload 后追加 `'\0'`（`RecvNode` 多分配 1 字节），投入逻辑队列，然后重新监听下一帧头部。

客户端（`TcpMgr`）：

- 发送：`QDataStream` 设为 `BigEndian`，写入 `quint16 id`、`quint16 len`，再 append JSON 字节；
- 接收：`readyRead` 累积到 `_buffer`，用 `_b_recv_pending` 处理半包，按同样的 4 字节头循环拆帧。

> 两端对字节序的处理是对称的：客户端 `QDataStream::BigEndian`，服务端
> `boost::asio::detail::socket_ops::network_to_host_short`。在小端机器（x86/ARM 均默认小端）
> 上，如果任一侧漏掉字节序转换，解析出的 msg_id 会是错误值——这是排查"消息不响应"时首先要核对的点。

### 消息 ID 表

服务端定义在 `services/ChatServer/include/const.h` 的 `MSG_IDS`，
客户端定义在 `client/core/global.h` 的 `ReqId`。**两处必须保持一致。**

| ID | 名称 | 方向 | 说明 |
| --- | --- | --- | --- |
| 1005 | `MSG_CHAT_LOGIN` | C → S | 登录长连接 |
| 1006 | `MSG_CHAT_LOGIN_RSP` | S → C | 登录回包（含用户资料与好友数据） |
| 1007 | `ID_SEARCH_USER_REQ` | C → S | 搜索用户 |
| 1008 | `ID_SEARCH_USER_RSP` | S → C | 搜索回包 |
| 1009 | `ID_ADD_FRIEND_REQ` | C → S | 发起好友申请 |
| 1010 | `ID_ADD_FRIEND_RSP` | S → C | 申请回执 |
| 1011 | `ID_NOTIFY_ADD_FRIEND_REQ` | S → C | 通知被申请人"有人申请加你为好友" |
| 1013 | `ID_AUTH_FRIEND_REQ` | C → S | 同意好友申请 |
| 1014 | `ID_AUTH_FRIEND_RSP` | S → C | 同意回包（返回对方资料） |
| 1015 | `ID_NOTIFY_AUTH_FRIEND_REQ` | S → C | 通知申请方"对方已同意" |
| 1017 | `ID_TEXT_CHAT_MSG_REQ` | C → S | 发送文本消息 |
| 1018 | `ID_TEXT_CHAT_MSG_RSP` | S → C | 发送方回执 |
| 1019 | `ID_NOTIFY_TEXT_CHAT_MSG_REQ` | S → C | 向接收方投递消息 |

> 1012 与 1016 为历史预留空位，当前未使用。

### 载荷定义

服务端 JSON 由 jsoncpp 的 `toStyledString()` 生成（带缩进换行），客户端用 `QJsonDocument::fromJson` 解析，
两种格式兼容。以下列出各消息的字段。

**1005 登录请求** `{uid:int, token:string}`

**1006 登录回包**

```jsonc
{
  "error": 0,                       // 非 0 时其余字段可能缺失
  "uid": 1002, "name": "xigu", "email": "a@b.com",
  "nick": "xigu", "desc": "", "sex": 0, "icon": ":/res/head_1.jpg",
  "apply_list": [                   // 待处理的好友申请
    {"name":"", "uid":0, "icon":"", "nick":"", "sex":0, "desc":"", "status":0}
  ],
  "friend_list": [                  // 好友列表
    {"name":"", "uid":0, "icon":"", "nick":"", "sex":0, "desc":"", "back":""}
  ]
}
```

> 登录与搜索只返回用户资料，不返回密码或密码哈希。除登录请求外，TCP 业务必须绑定已认证会话，操作者 uid/fromuid 必须与会话一致。

**1007 搜索请求** `{uid:string}` —— 传**字符串**，`isPureDigit` 判定：纯数字按 uid 查，否则按用户名查。

**1008 搜索回包** `{error, uid, name, email, nick, desc, sex, icon}`
（按用户名查询时不含 `icon`）

**1009 好友申请** `{uid:int, applyname:string, bakname:string, touid:int}`

**1010 好友申请回执** `{error:int}`

**1011 好友申请通知** `{error, applyuid, name, desc, icon, sex, nick}`

**1013 同意好友申请** `{fromuid:int, touid:int, back:string}`（`back` 为备注名）

**1014 同意回包** `{error, name, nick, icon, sex, uid}`

**1015 同意通知** `{error, fromuid, touid, name, nick, icon, sex}`

**1017 文本消息** `{fromuid:int, touid:int, text_array:[{content:string, msgid:string}]}`

**1018 文本消息回执** `{error, text_array, fromuid, touid}`

**1019 文本消息投递** `{error, fromuid, touid, text_array:[{content, msgid}]}`

---

## 3.2 HTTP 接口（Client ↔ GateServer）

Base URL 由客户端 `config.ini` 的 `[GateServer] host/port` 拼成 `http://<host>:<port>`。
默认 `http://127.0.0.1:8080`。所有响应 `Content-Type: text/json`，`Server: GateServer`，
连接为 `keep_alive(false)`（每请求一连接）。

路由注册在 `GateServer/src/LogicSystem.cpp`，未匹配的路径返回 404 `url not found`。

### `GET /get_test`

连通性测试，回显查询参数。用于验证 GET 参数解析（URL 解码由 `HttpConnection::PreParseGetParam` 完成）。

### `POST /get_verifycode`

请求：`{"email": "user@example.com"}`

响应：`{"error": int, "email": string}`

流程：gRPC 调用 `VerifyService.GetVerifyCode`；错误码直接透传（0/1/2/3，见错误码表）。

### `POST /user_register`

请求：

```json
{"email": "...", "user": "...", "passwd": "...", "confirm": "...", "verifycode": "..."}
```

校验顺序：

1. JSON 解析失败或缺字段 → `1001`；
2. `passwd != confirm` → `1006`；
3. Redis `GET code_<email>` 不存在 → `1003`（验证码过期）；
4. 与请求中的 `verifycode` 不等 → `1004`；
5. `MysqlMgr::RegUser()` 返回 `0` 或 `-1` → `1005`（用户名或邮箱已存在）。

成功响应：`{"error":0, "uid":int, "email":string, "user":string, "verifycode":string}`

### `POST /reset_pwd`

请求：`{"email": "...", "user": "...", "passwd": "...", "verifycode": "..."}`

校验顺序：JSON → 验证码存在性（1003）→ 验证码比对（1004）→ `CheckEmail(name, email)`（1007）
→ `UpdatePwd(name, pwd)`（1008，失败时）。

> 新密码在 DAO 层做 Argon2id 哈希后落库，网关只处理明文入参。

成功响应：`{"error":0, "email":string, "user":string, "verifycode":string}`

### `POST /user_login`

请求：`{"email": "...", "passwd": "..."}`

校验顺序：JSON（1001）→ `CheckPwd`（1009 邮箱或密码不匹配）→ `StatusGrpcClient::GetChatServer`（1002 RPC 失败）。

成功响应：

```json
{"error":0, "email":"...", "uid":1002, "token":"<uuid>", "host":"127.0.0.1", "port":"8090"}
```

客户端随后用 `host:port` 建立 TCP 连接，并用 `uid + token` 发送 1005 完成长连接登录。

---

## 3.3 gRPC 接口（`server/proto/message.proto`）

`package message;`，三个 service：

### `VerifyService`（VerifyServer，50051）

```proto
rpc GetVerifyCode (GetVerifyReq) returns (GetVerifyRsp);
// GetVerifyReq { string email = 1; }
// GetVerifyRsp { int32 error = 1; string email = 2; string code = 3; }
```

调用方：GateServer（`VerifyGrpcClient`）。

### `StatusService`（StatusServer，50052）

```proto
rpc GetChatServer (GetChatServerReq) returns (GetChatServerRsp);
rpc Login (LoginReq) returns (LoginRsp);
```

| 消息 | 字段 |
| --- | --- |
| `GetChatServerReq` | `uid` |
| `GetChatServerRsp` | `error`, `host`, `port`, `token` |
| `LoginReq` | `uid`, `token` |
| `LoginRsp` | `error`, `uid`, `token` |

`GetChatServer` 的调用方是 GateServer；**`Login` 当前无人调用**
（ChatServer 直接读 Redis `utoken_<uid>` 校验），见 `11-known-limitations.md`。

### `ChatService`（ChatServer 实例，50055 / 50056 …）

```proto
rpc NotifyAddFriend    (AddFriendReq)   returns (AddFriendRsp);
rpc RplyAddFriend      (RplyFriendReq)  returns (RplyFriendRsp);
rpc SendChatMsg        (SendChatMsgReq) returns (SendChatMsgRsp);
rpc NotifyAuthFriend   (AuthFriendReq)  returns (AuthFriendRsp);
rpc NotifyTextChatMsg  (TextChatMsgReq) returns (TextChatMsgRsp);
```

| 消息 | 字段 |
| --- | --- |
| `AddFriendReq` | `applyuid`, `name`, `desc`, `icon`, `nick`, `sex`, `touid` |
| `AddFriendRsp` | `error`, `applyuid`, `touid` |
| `RplyFriendReq` | `rplyuid`, `agree`, `touid` |
| `RplyFriendRsp` | `error`, `rplyuid`, `touid` |
| `SendChatMsgReq` | `fromuid`, `touid`, `message` |
| `SendChatMsgRsp` | `error`, `fromuid`, `touid` |
| `AuthFriendReq` | `fromuid`, `touid` |
| `AuthFriendRsp` | `error`, `fromuid`, `touid` |
| `TextChatMsgReq` | `fromuid`, `touid`, `repeated TextChatData textmsgs` |
| `TextChatData` | `msgid`, `msgcontent` |
| `TextChatMsgRsp` | `error`, `fromuid`, `touid`, `repeated TextChatData textmsgs` |

实际使用：`NotifyAddFriend`、`NotifyAuthFriend`、`NotifyTextChatMsg` 三个用于跨实例转发；
`RplyAddFriend` 与 `SendChatMsg` **已定义但未实现调用**（服务实现中直接返回默认响应）。

所有 RPC 使用 `grpc::InsecureServerCredentials()` / `InsecureChannelCredentials()`，
即**服务间通信未加密**，适用于受信任的内网部署。

---

## 3.4 错误码总表

### 服务端错误码（`GateServer/include/const.h`、`ChatServer/include/const.h`）

| 值 | 名称 | 含义 | 出现位置 |
| --- | --- | --- | --- |
| 0 | `Success` | 成功 | 全部 |
| 1001 | `Error_Json` | JSON 解析失败或缺字段 | HTTP 接口 |
| 1002 | `RPCFailed` | gRPC 调用失败 | HTTP 接口、gRPC 响应 |
| 1003 | `VerifyExpired` | 验证码不存在或已过期 | 注册、改密 |
| 1004 | `VerifyCodeErr` | 验证码不匹配 | 注册、改密 |
| 1005 | `UserExist` | 用户名或邮箱已存在 | 注册 |
| 1006 | `PasswdErr` | 两次密码不一致 | 注册 |
| 1007 | `EmailNotMatch` | 用户名与邮箱不匹配 | 改密 |
| 1008 | `PasswdUpFailed` | 更新密码失败 | 改密 |
| 1009 | `PasswdInvalid` | 邮箱或密码错误 | 登录 |
| 1010 | `TokenInvalid` | Token 不匹配 | 长连接登录、StatusServer::Login |
| 1011 | `UidInvalid` | uid 无效（Redis 无 token / 用户不存在） | 长连接登录、通知类消息 |

> `ChatServer/include/const.h` 在 1001–1009 的基础上追加了 1010/1011；
> StatusServer 复用同一份定义。三处 `ErrorCodes` 目前是**各自独立复制**的枚举，
> 修改时需同步。

### 验证码服务错误码（`VerifyServer/include/const.h`，`VerifyCodes`）

| 值 | 名称 | 含义 |
| --- | --- | --- |
| 0 | `Success` | 验证码已发送 |
| 1 | `RedisErr` | 写入 Redis 失败 |
| 2 | `Exception` | 处理过程抛出异常 |
| 3 | `SendMailErr` | 发送邮件失败 |

该错误码由 GateServer 原样写入 HTTP 响应的 `error` 字段，客户端可据此提示"邮件发送失败"。

### 客户端错误码（`client/core/global.h`，`ErrorCodes`）

| 值 | 名称 | 含义 |
| --- | --- | --- |
| 0 | `SUCCESS` | 成功 |
| 1 | `ERR_JSON` | JSON 解析失败 |
| 2 | `ERR_NETWORK` | 网络错误（HTTP 层） |

这是**客户端本地**的错误码空间，与服务端 1001+ 的错误码不是同一套，
客户端在展示时直接用服务端透传的数值。

### 服务端错误码到界面文案的映射（`client/core/service/authservice.cpp`）

界面不直接显示数字，由 `AuthService` 按流程翻译。同一码在不同流程含义不同的只有 1006：

| 码 | 登录 | 注册 | 重置密码 |
| --- | --- | --- | --- |
| 1001 | 请求数据不完整，请重试 | 同左 | 同左 |
| 1002 | 服务器繁忙，请稍后再试 | 同左 | 同左 |
| 1003 | — | 验证码已过期，请重新获取 | 同左 |
| 1004 | — | 验证码不正确 | 同左 |
| 1005 | — | 该邮箱或用户名已被注册 | — |
| 1006 | 密码不正确 | 两次输入的密码不一致 | — |
| 1007 | — | — | 用户名与邮箱不匹配 |
| 1008 | — | — | 密码更新失败，请稍后再试 |
| 1009 | 邮箱或密码不正确 | — | — |
| 其他 | 操作失败（错误码 N） | 同左 | 同左 |

> 登录失败统一提示「邮箱或密码不正确」，不区分账号是否存在，避免枚举账号。


---

## 3.5 协议扩展指南

新增一个客户端 ↔ ChatServer 的消息需要改动**四处**：

1. `server/services/ChatServer/include/const.h` 的 `MSG_IDS` 增加 ID；
2. `client/core/global.h` 的 `ReqId` 增加同名同值 ID；
3. 服务端 `LogicSystem::RegisterCallBacks()` 注册处理函数（处理函数在单一逻辑线程执行）；
4. 客户端 `TcpMgr::initHandlers()` 注册处理函数，并在 UI 层连接对应信号。

新增一个服务间 RPC 需要改动：

1. `server/proto/message.proto` 定义消息与 rpc（构建期自动生成代码，无需手工执行 protoc）；
2. 服务提供方实现 `ServiceImpl` 中的方法；
3. 调用方在 `*GrpcClient` 中增加方法，复用已有 Stub 连接池。
