# 05 · 配置参考

所有服务通过 `ConfigMgr`（Boost.PropertyTree 的 `read_ini`）读取**当前工作目录下**的 `config.ini`，
因此启动时必须 `cd` 到可执行文件所在目录。配置文件在构建后由 CMake 自动拷贝到产物目录，
修改**源码目录下的 ini** 后需要重新构建，或直接修改产物目录下的 ini。

客户端通过 Qt `QSettings(IniFormat)` 读取可执行文件同目录的 `config.ini`，机制类似。

配置访问方式：`ConfigMgr::Inst()[section][key]`（未命中返回空串）或
`ConfigMgr::Inst().GetValue(section, key)`。

---

## 5.1 客户端 `client/config.ini`

```ini
[GateServer]
host=127.0.0.1
port=8080
```

| 字段 | 说明 |
| --- | --- |
| `host` / `port` | 网关地址，启动时拼成 `http://host:port` 存入全局 `gate_url_prefix` |

> 键名大小写：`QSettings` 读取时用 `"GateServer/host"`、`"GateServer/port"`，
> 与文件中的小写键名保持一致即可。

---

## 5.2 GateServer `server/services/GateServer/config.ini`

```ini
[GateServer]
Port = 8080              # HTTP 监听端口

[VerifyServer]
Host = 127.0.0.1         # VerifyServer 地址（gRPC 客户端用）
Port = 50051

[StatusServer]
Host = 127.0.0.1         # StatusServer 地址（gRPC 客户端用）
Port = 50052

[Mysql]
Host = 127.0.0.1
XPort = 33060            # MySQL X 协议端口，不是 3306
User = root
Passwd = 123456
Schema = xigu

[Redis]
Host = 127.0.0.1
Port = 6380
Passwd = 123456
```

| 段 | 字段 | 说明 |
| --- | --- | --- |
| `GateServer` | `Port` | HTTP 监听端口 |
| `VerifyServer` / `StatusServer` | `Host` / `Port` | 下游 gRPC 服务地址，用于建立 Stub 连接池（各 5 条） |
| `Mysql` | `XPort` | **X Protocol 端口（默认 33060）**，经典协议 3306 不适用 |
| `Redis` | `Port` | 默认 6380（非 Redis 默认的 6379） |

---

## 5.3 StatusServer `server/services/StatusServer/config.ini`

```ini
[StatusServer]
Host = 127.0.0.1
Port = 50052             # gRPC 监听地址（Host 填 0.0.0.0 可对外）

[Mysql]  ...同 GateServer...
[Redis]  ...同 GateServer...

[chatservers]
Name = chatserver1,chatserver2     # 逗号分隔的实例名清单，顺序即遍历顺序

[chatserver1]
Name = chatserver1
Host = 127.0.0.1
Port = 8090                        # 客户端 TCP 端口（不是 RPC 端口）

[chatserver2]
Name = chatserver2
Host = 127.0.0.1
Port = 8091
```

| 段/字段 | 说明 |
| --- | --- |
| `[chatservers] Name` | 参与调度的实例名清单。新增实例必须在此登记，否则不会被分配 |
| `[<实例名>] Name/Host/Port` | 返回给客户端的连接地址。`Port` 是**客户端 TCP 端口** |

调度算法（`StatusServiceImpl::getChatServer`）：遍历清单，逐个 `HGET logincount <name>`；
取不到的实例按 `INT_MAX` 处理（即视为"无连接数记录"），最终返回连接数最小的实例。

---

## 5.4 ChatServer `server/services/ChatServer/config/*.ini`

每份 ini 对应一个实例。以 `ChatServer.ini`（实例 `chatserver1`）为例：

```ini
[GateServer]  Port = 8080            # 保留段，当前未被 ChatServer 读取
[VerifyServer] Host/Port = 50051     # 保留段，当前未被 ChatServer 读取
[StatusServer] Host/Port = 50052     # 保留段，当前未被 ChatServer 读取

[SelfServer]
Name = chatserver1       # 本实例名，必须与 StatusServer 登记的名字一致
Host = 0.0.0.0           # gRPC 监听地址
Port = 8090              # 面向客户端的 TCP 监听端口
RPCPort = 50055          # 面向其他 ChatServer 实例的 gRPC 端口

[Mysql]  ...同 GateServer...
[Redis]  ...同 GateServer...

[PeerServer]
Servers = chatserver2    # 除自己以外的全部实例名，逗号分隔

[chatserver2]            # 每个对端一段
Name = chatserver2
Host = 127.0.0.1
Port = 50056             # 对端的 RPCPort（gRPC），注意不是对端的 TCP 端口
```

| 段 | 字段 | 说明 |
| --- | --- | --- |
| `[SelfServer] Name` | 实例名，写入 `uip_<uid>` 与 `logincount` 的 field |
| `[SelfServer] Host` | gRPC 监听地址（`0.0.0.0` 表示监听全部网卡） |
| `[SelfServer] Port` | TCP 监听端口，客户端连这里 |
| `[SelfServer] RPCPort` | gRPC 监听端口，其他实例连这里 |
| `[PeerServer] Servers` | 对端实例名清单，用于建立 `ChatConPool`（每个 5 条 Stub） |
| `[<对端名>]` | 对端的 `Host` 与其 **`RPCPort`** |

**两个 Port 的语义差异是这里最容易出错的地方**：`[SelfServer]` 同时有 `Port`（TCP）和 `RPCPort`（gRPC），
而对端段里只有一个 `Port`，指的是对端的 **RPC 端口**。

`ChatServer2.ini` 与 `ChatServer.ini` 完全对称：`SelfServer` 改为 `chatserver2` / 8091 / 50056，
`PeerServer` 改为 `chatserver1`，对端段指向 `50055`。

---

## 5.5 VerifyServer `server/services/VerifyServer/config.ini`

```ini
[VerifyServer]
Host = 0.0.0.0
Port = 50051             # gRPC 监听端口

[Redis]
Host = 127.0.0.1
Port = 6380
Passwd = 123456

[Email]
Host = smtp.qq.com       # SMTP 服务器
Port = 465               # SSL 直连（隐式 TLS）；587 是 STARTTLS，需改代码
User = xxx@qq.com        # 发件邮箱账号
Pass = <授权码>           # 邮箱授权码，不是登录密码
From = xxx@qq.com        # 发件人，留空则与 User 相同
CA =                     # CA 证书包路径；留空按平台自动探测
```

| 字段 | 说明 |
| --- | --- |
| `Port` | `465` = 连接即 TLS 的隐式 SSL，代码用 `SecureStreamSocket(addr, host, ctx)` 建连后交给 `SMTPClientSession`。`SecureSMTPClientSession`（STARTTLS）只适用于 587，换端口需同步改代码 |
| `Pass` | 各大邮箱的"SMTP 授权码 / 应用专用密码"，不是网页登录密码 |
| `CA` | 三级回退，见下 |

### CA 路径解析（`EmailSender::ResolveCAPath`）

1. 配置显式给出**且文件存在** → 使用该路径（用于私有 CA）；
2. 配置给出但文件不存在 → 打印告警，继续自动探测（保证同一份 ini 在三平台都能启动）；
3. 未配置 → 依次探测 `/etc/ssl/cert.pem`（macOS / Alpine）、
   `/etc/ssl/certs/ca-certificates.crt`（Debian / Ubuntu）、
   `/etc/pki/tls/certs/ca-bundle.crt`（RHEL / CentOS / Fedora）；
4. 仍为空 → 构造 SSL Context 时启用 `loadDefaultCAs`，使用 OpenSSL 默认证书目录
   （Windows 通常走到这一步）。

证书校验失败会**直接导致发信失败**（`Send` 返回 false → 响应 `SendMailErr(3)`）。

---

## 5.6 常见变更场景

| 场景 | 需要改动的位置 |
| --- | --- |
| 改 MySQL / Redis 地址或密码 | GateServer、StatusServer、ChatServer（每份实例 ini）、VerifyServer 各自的 `config.ini` |
| 改网关端口 | GateServer `[GateServer] Port` + 客户端 `config.ini` |
| 新增 ChatServer 实例 | 新建 `ChatServer/config/<Name>.ini`；已存在实例的 `[PeerServer]` 与对端段互相登记；`StatusServer` 的 `[chatservers] Name` 与 `[<Name>]` 段 |
| 改实例端口 | 该实例 ini 的 `[SelfServer]`；对端 ini 的对端段；`StatusServer` 的 `[<实例名>].Port` |
| 换发件邮箱 | VerifyServer `[Email]` 的 `Host` / `User` / `Pass` / `From`（注意 465 vs 587） |
| 部署到非 localhost | 各服务 ini 中的 `Host` 改为实际地址；ChatServer `[SelfServer].Host` 用 `0.0.0.0` 监听 |
