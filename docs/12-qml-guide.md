# 12 · QML 界面约定

客户端的布局、交互动画与外观偏好由 QML 管理，业务规则与会话数据由 C++ 管理，两者通过 `bridge/` 通信。
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
core 不依赖 Gui/Quick；这一边界通过头文件依赖和代码审查维护。

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

界面共用 `Theme.qml` 的日夜语义色、`AppIcon.qml` 的 Canvas 操作图标、
`BrandMark.qml` 的应用品牌图形，以及
`PaperCompanion.qml` 的折纸信封角色。它们的外观与交互状态留在 QML，
不为纯视觉变化新增业务 bridge。

页面品牌标识只显示图形，窗口标题为 `xiguchat`。`resources/xiguchat.svg` 为原创主图，
导出的 PNG 供 `BrandMark` 与窗口图标共用，ICO 和 ICNS 分别供 Windows 与 macOS 系统使用。

## 12.3 bridge 契约

QML 负责结构、动画、焦点与临时表单状态，业务规则和会话数据在 C++。业务页面通过 bridge，
通过 `QML_ELEMENT` 注册，QML 里 `import XiguChat` 后直接实例化。

| 项 | 约定 |
| --- | --- |
| 命名 | 页面级 `XxxBridge`，服务类在 `core/service/`，命名 `XxxService` |
| 上行 | QML 调 `Q_INVOKABLE`，使用接口明确声明的类型；业务协议在 C++ 中组装 |
| 下行 | `Q_PROPERTY`（带 `NOTIFY`）暴露状态，QML 用绑定消费，不写命令式赋值 |
| 列表 | `QAbstractListModel` + `roleNames()`，delegate 用 `model.xxx` 绑定 |
| 错误 | 统一 `errorText` + `busy`，页面只做展示 |
| 所有权 | bridge 由 QML 创建并持有；service 为单例，不自持 UI |

认证页面 bridge 继承 `PageBridge`，QML 调 `bridge.go("register")`，
由 `ShellBridge` 改 `page`，根窗口的 `Loader` 换页并调整窗口尺寸。页面之间互不认识。

`ShellBridge` 在构造函数里登记自己的静态指针，`PageBridge::go()` 通过它导航——
根窗口只有一个实例，不需要额外的注册机制。

## 12.4 QML 编码规范

布局、数据绑定与交互遵守以下约定：

1. **delegate 不要声明与组件同名的 `required property`**。
   `required property string text` 会遮蔽组件自身的 `text`，`text: text` 变成自我赋值，
   气泡内容全空。用 `model.text` 绑定，或给组件属性换名。
2. **尺寸只能单向推导**。`Column.width` 绑 `bubble.width`、`bubble.width` 又依赖
   `Column.implicitWidth` 会形成绑定循环，宽度塌成 0。先由内容算出目标宽度，再让容器跟随。
3. **中文正文不加 `lineHeight` 倍率**。中文字体（苹方 14px）自然行盒已是 19.58px，
   再乘 1.38 会把行盒撑到 27.6px，多出的空白全堆在基线下方，视觉上「上紧下松」。
4. **图片尺寸随消息下发**，不要在界面里读 `Image.implicitWidth`——异步加载完成前它是 0，
   会导致气泡宽度先小后跳。
5. **圆角头像用 `MultiEffect` + mask**。`Rectangle { radius; clip: true }` 的裁剪不认圆角；
   软件渲染时停用遮罩特效，保证头像仍能显示。
6. **`QT_QML_SINGLETON_TYPE` 必须在 `qt_add_qml_module` 之前设置**，否则单例属性全读成
   `undefined`，且报错信息不指向根因。
7. **留白要写成有宽度的元素**。`Item { height: 34 }` 默认宽度为 0，而 `Column` / `Row`
   这类 positioner 会跳过零宽元素——留白被静默吃掉，不报错。统一用 `Spacer.qml`。
8. **`qt_add_qml_module` 要显式给 `RESOURCE_PREFIX /qt/qml`**。不写时前缀为空，
   QML 落在 `qrc:/XiguChat/...` 而不是惯例的 `qrc:/qt/qml/XiguChat/...`。
9. **本项目的 `QML_ELEMENT` 类统一放在可执行目标的 `bridge/` 中**，
   由对应的 `qt_add_qml_module` 注册，不在 core 目标重复编译或注册。
10. **`QString` 到 QML 就是 JS 字符串**，没有 `.left()` / `.mid()`，要用
    `.substring()`；`QVariantMap` 里不存在的键取出来是 `undefined`，
    `.length` 会抛 `TypeError`，赋给 string 属性会报 "Unable to assign [undefined] to QString"。
    统一写 `(x || "")` 兜底。
11. **`Window` 的尺寸要在运行时按页设**：先放开 `maximumWidth/Height`，再设
    `minimumWidth/Height`，最后赋值 `width/height`。反过来（先 min 后 max）会被夹住。
12. **输入法组合态用 `inputMethodComposing` 判断**，不依赖预编辑文本内容。
    组合期间不把回车当成发送操作，输入框占位提示也应隐藏。
13. **页面生命周期与会话分开**：Loader 切换 URL 会销毁旧页面，网络回包与会话数据归服务层。
    `setSource()` 与 `source` 绑定都可切页；相同 URL 不保证重建。请求中禁止切换认证表单。
14. **模型通知必须包住变更**：重置前发 `AboutToChange`，重置后发 `Changed`；
    增量插入期间 `rowCount()` 必须从旧行数切换到新行数。
15. **遮罩必须阻止交互**：弹层显示时禁用底层内容，同时拦截遮罩上的鼠标事件。
16. **图片预览与网络发送分开**：输入文档保存图片内容与尺寸，只有文本进入聊天协议，
    图片保留在当前会话中。
17. **页面使用语义颜色**：背景、正文、边线、选中态与操作色从 `Theme` 读取，
    不在页面内另建日夜配色。Canvas 的颜色或尺寸变化后要触发 `requestPaint()`。
18. **外观选择只保存在本机**：`Theme.dark` 通过 `QtCore.Settings` 的 `Appearance`
    分类记忆，认证页和聊天页共用同一状态，不调用服务端，不写入 `config.ini`。
19. **角色动画只接收交互状态**：`PaperCompanion` 根据焦点、密码显示状态和请求状态反应，
    不接收密码正文。隐藏或紧凑显示时停用不需要的循环动画。
20. **可操作控件支持键盘与辅助功能**：提供可见焦点、可访问名称与键盘触发；
    尚未实现的操作显示为禁用状态，列表不添加虚构的在线状态、时间或用户资料。

## 12.5 构建注意

- **Windows 使用 Qt 安装包自带的 MinGW 11.2**（`mingw1120_64`），不使用 MSVC：
  Qt 官方 Windows 桌面套件是 MinGW 构建，与 MSVC 编译的库 ABI 不兼容。
  两条入口：Qt Creator 选 Qt 6.5 MinGW Kit；命令行设 `QT_MINGW_ROOT` 后用预设构建。
- 三平台预设均为单配置生成器（Ninja），Debug/Release 由预设区分。
- 产物仍在 `build/<预设>/bin/`（macOS 为 `xiguchat.app`），`config.ini` 部署规则不变。
- QML 运行时使用 `QtQuick`、`QtQuick.Effects` 和 `QtCore`；Windows 部署使用
  `windeployqt --qmldir <qml目录>` 扫描所需模块。
- Windows 的 `resources/xiguchat.rc` 将 ICO 嵌入 EXE；CMake 在 `project()` 前尊重已有的
  `CMAKE_RC_COMPILER` 或环境变量 `RC`，未指定时从 MinGW 目录定位 `windres`。
  macOS 使用 bundle 内的 `xiguchat.icns`。
- 认证表单宽 320 像素，窗口尺寸为登录 `420×560`、注册 `420×740`、重置 `420×680`；
  聊天窗口初始至少 `1060×740`，最小 `960×680`。布局检查同时覆盖日间与夜间外观。
- 调试时产生的脚本、截图与临时构建目录在验证结束后清理，不作为项目源码保留。

## 12.6 剩余缺口

- 消息不持久化：双方都在线才能互发，重新登录看不到历史消息。
- 无断线重连：断开后显示「连接中断了」，通过「返回登录」重新登录。
- 图片支持拖入、粘贴和本地预览，图片/文件的网络传输未实现。
