# 12 · QML 界面约定

客户端界面用 QML 写、逻辑用 C++ 写，两者之间只通过 `bridge/` 通信。
本文说明目录结构、页面与代码的对应关系、bridge 的接口约定，以及写 QML 时必须遵守的规范。

## 12.1 目录结构

```
client/
├─ core/      业务与数据：global / userdata / usermgr / service/
├─ network/   httpmgr（短连接） + tcpmgr（长连接）
├─ bridge/    QML 与 C++ 的唯一通道
├─ qml/       Main + 各页面 + 组件 + Theme
├─ resources/ 应用图标与 qrc
└─ main.cpp
```

业务调用方向：`qml → bridge → core/service → network`；`core/userdata` 是共享数据类型。
`xiguchat_core` 静态库只链接 `Qt6::Core` / `Qt6::Network`，可执行链接 `Qt6::Quick`。
core 不依赖 Gui/Quick，保持无界面的单元测试能力；这一边界还需要通过头文件依赖和代码审查维护。

## 12.2 页面与代码的对应关系

| 页面 | 文件 | 承载状态的 C++ |
| --- | --- | --- |
| 登录 | `qml/Login.qml` | `LoginBridge` + `AuthService` + `SessionService` |
| 注册 | `qml/Register.qml` | `RegisterBridge` + `AuthService` |
| 重置密码 | `qml/Reset.qml` | `ResetBridge` + `AuthService` |
| 聊天主界面（侧栏 + 左列 + 右侧页面栈） | `qml/ChatWindow.qml` | `ChatBridge` + 三个列表模型 |
| 会话页 | `qml/ChatPage.qml` | `ChatBridge` + `MessageModel` + `ComposerBridge` |
| 好友资料页 | `qml/FriendInfoPage.qml` | `ChatBridge::openProfile` + `profile` |
| 好友申请页 | `qml/ApplyFriendPage.qml` | `ApplyListModel` |
| 申请添加好友（弹层） | `qml/FriendDialogs.qml` | `ChatBridge::applyTo` |
| 验证申请（弹层） | `qml/FriendDialogs.qml` | `ChatBridge::authApply` |
| 搜索结果（弹层） | `qml/FriendDialogs.qml` | `ChatBridge::searchState` |
| 根窗口 + 掉线提示 | `qml/Main.qml` | `ShellBridge` |

## 12.3 bridge 契约

QML 负责结构、动画、焦点与临时表单状态，业务规则和会话数据在 C++。业务页面通过 bridge，
通过 `QML_ELEMENT` 注册，QML 里 `import XiguChat` 后直接实例化。

| 项 | 约定 |
| --- | --- |
| 命名 | 页面级 `XxxBridge`，服务类在 `core/service/`，命名 `XxxService` |
| 上行 | QML 调 `Q_INVOKABLE`，参数只接受基础类型与 `QUrl` |
| 下行 | `Q_PROPERTY`（带 `NOTIFY`）暴露状态，QML 用绑定消费，不写命令式赋值 |
| 列表 | `QAbstractListModel` + `roleNames()`，delegate 用 `model.xxx` 绑定 |
| 错误 | 统一 `errorText` + `busy`，页面只做展示 |
| 所有权 | bridge 由 QML 创建并持有；service 为单例，不自持 UI |

认证页面 bridge 继承 `PageBridge`，QML 调 `bridge.go("register")`，
由 `ShellBridge` 改 `page`，根窗口的 `Loader` 换页并调整窗口尺寸。页面之间互不认识。

`ShellBridge` 在构造函数里登记自己的静态指针，`PageBridge::go()` 通过它导航——
根窗口只有一个实例，不需要额外的注册机制。

## 12.4 QML 编码规范

迁移中实际踩到并修掉的坑，逐条固化：

1. **delegate 不要声明与组件同名的 `required property`**。
   `required property string text` 会遮蔽组件自身的 `text`，`text: text` 变成自我赋值，
   气泡内容全空。用 `model.text` 绑定，或给组件属性换名。
2. **尺寸只能单向推导**。`Column.width` 绑 `bubble.width`、`bubble.width` 又依赖
   `Column.implicitWidth` 会形成绑定循环，宽度塌成 0。先由内容算出目标宽度，再让容器跟随。
3. **中文正文不加 `lineHeight` 倍率**。中文字体（苹方 14px）自然行盒已是 19.58px，
   再乘 1.38 会把行盒撑到 27.6px，多出的空白全堆在基线下方，视觉上「上紧下松」。
4. **图片尺寸随消息下发**，不要在界面里读 `Image.implicitWidth`——异步加载完成前它是 0，
   会导致气泡宽度先小后跳。
5. **圆角图片用 `MultiEffect` + mask**。`Rectangle { radius; clip: true }` 的裁剪不认圆角。
6. **`QT_QML_SINGLETON_TYPE` 必须在 `qt_add_qml_module` 之前设置**，否则单例属性全读成
   `undefined`，且报错信息不指向根因。
7. **留白要写成有宽度的元素**。`Item { height: 34 }` 默认宽度为 0，而 `Column` / `Row`
   这类 positioner 会跳过零宽元素——留白被静默吃掉，不报错。统一用 `Spacer.qml`。
8. **`qt_add_qml_module` 要显式给 `RESOURCE_PREFIX /qt/qml`**。不写时前缀为空，
   QML 落在 `qrc:/XiguChat/...` 而不是惯例的 `qrc:/qt/qml/XiguChat/...`。
9. **带 `QML_ELEMENT` 的类必须编译进可执行目标，不能放进静态库**。静态库的 AUTOMOC 与
   `qt_add_qml_module` 会各生成一份 moc，链接期报 duplicate symbol。
10. **`QString` 到 QML 就是 JS 字符串**，没有 `.left()` / `.mid()`，要用
    `.substring()`；`QVariantMap` 里不存在的键取出来是 `undefined`，
    `.length` 会抛 `TypeError`，赋给 string 属性会报 "Unable to assign [undefined] to QString"。
    统一写 `(x || "")` 兜底。
11. **`Window` 的尺寸要在运行时按页设**：先放开 `maximumWidth/Height`，再设
    `minimumWidth/Height`，最后赋值 `width/height`。反过来（先 min 后 max）会被夹住。
12. **不要用带 `REVISION` 的私有属性**（如 `TextInput.preeditText`、`TextEdit.preeditText`）。
    它们在 Qt 6.5 里是 `Q_REVISION(2, 7)`，`qmlcachegen` 生成的 AOT 代码会拿着空元对象去查属性，
    运行期直接 `SIGSEGV` 而不是给警告——排查时只有崩溃栈，没有 QML 报错行号。
    输入法组合态用 `inputMethodComposing`（无版本门控）代替。
13. **页面生命周期与会话分开**：Loader 切换 URL 会销毁旧页面，网络回包与会话数据归服务层。
    `setSource()` 与 `source` 绑定都可切页；相同 URL 不保证重建。请求中禁止切换认证表单。
14. **模型通知必须包住变更**：重置前发 `AboutToChange`，重置后发 `Changed`；
    增量插入期间 `rowCount()` 必须从旧行数切换到新行数。
15. **遮罩必须阻止交互**：弹层显示时禁用底层内容，同时拦截遮罩上的鼠标事件。
16. **图片预览与网络发送分开**：输入文档保存图片内容与尺寸，只有文本进入聊天协议，
    图片保留在当前会话中。

## 12.5 构建注意

- **Windows 使用 Qt 安装包自带的 MinGW 11.2**（`mingw1120_64`），不使用 MSVC：
  Qt 官方 Windows 桌面套件是 MinGW 构建，与 MSVC 编译的库 ABI 不兼容。
  两条入口：Qt Creator 选 Qt 6.5 MinGW Kit；命令行设 `QT_MINGW_ROOT` 后用预设构建。
- 三平台预设均为单配置生成器（Ninja），Debug/Release 由预设区分。
- 产物仍在 `build/<预设>/bin/`（macOS 为 `xiguchat.app`），`config.ini` 部署规则不变。
- QML 抓帧：`QQuickWindow::grabWindow()` 可在无屏幕录制权限的环境下取证，
  调试期临时挂环境变量入口，验收后删除。

## 12.6 剩余缺口

- 消息不持久化：双方都在线才能互发，重新登录看不到历史消息。
- 无断线重连：断开后提示「已离线」，需重新登录。
- 图片支持拖入、粘贴和本地预览，图片/文件的网络传输未实现。
