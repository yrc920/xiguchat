# ============================================================
#  XiguChat Client — 聊天客户端（Qt6 桌面程序）
#
#  技术栈: Qt 6.5 (Core / Gui / Network / Widgets) + C++17
# ============================================================
cmake_minimum_required(VERSION 3.16)

project(xiguchat VERSION 1.0.0 LANGUAGES CXX)

# ---------- C++ 标准 ----------
# 项目统一使用 C++17，编译期强制（不满足则报错）
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------- Qt 自动工具 ----------
# Qt 的 moc（元对象编译）/ uic（.ui 编译）/ rcc（资源编译）
# 由 CMake 自动处理：源码中出现的 Q_OBJECT 类、.ui 文件、.qrc 文件
# 都不用手写额外步骤，AUTOMOC/AUTOUIC/AUTORCC 会自动生成并编译
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
# 让编译器默认在当前目录找头文件（配合下方 include 目录使用）
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# ---------- 查找 Qt ----------
# Qt 安装路径来源（按优先级）：
#  1. Qt Creator 打开工程时，Kit 会自动注入 CMAKE_PREFIX_PATH（Qt 路径）
#  2. 命令行构建：export QT_ROOT=<Qt 安装目录>（如 ~/Qt/6.5.3/macos）
#     或 -DCMAKE_PREFIX_PATH=<Qt 安装目录>，二选一
# 只使用下面列出的 4 个模块；缺少任何模块都会在配置期直接报错
if(DEFINED ENV{QT_ROOT})
    list(APPEND CMAKE_PREFIX_PATH "$ENV{QT_ROOT}")
endif()
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Network Widgets)

# ------------------------------------------------------------
#  core/ — 全局定义、单例与公共数据结构
# ------------------------------------------------------------
set(CORE_SOURCES
    core/global.cpp
    core/userdata.cpp
    core/usermgr.cpp
)
set(CORE_HEADERS
    core/global.h
    core/singleton.h
    core/userdata.h
    core/usermgr.h
)

# ------------------------------------------------------------
#  network/ — 网络通信层 (HTTP / TCP 长连接)
# ------------------------------------------------------------
set(NETWORK_SOURCES
    network/httpmgr.cpp
    network/tcpmgr.cpp
)
set(NETWORK_HEADERS
    network/httpmgr.h
    network/tcpmgr.h
)

# ------------------------------------------------------------
#  widgets/ — 自定义控件（聊天气泡、列表项、输入框等）
# ------------------------------------------------------------
set(WIDGET_SOURCES
    widgets/adduseritem.cpp
    widgets/applyfrienditem.cpp
    widgets/bubbleframe.cpp
    widgets/chatitembase.cpp
    widgets/chatuserlist.cpp
    widgets/chatuserwid.cpp
    widgets/chatview.cpp
    widgets/clickedbtn.cpp
    widgets/clickedlabel.cpp
    widgets/clickedoncelabel.cpp
    widgets/conuseritem.cpp
    widgets/customizeedit.cpp
    widgets/friendlabel.cpp
    widgets/grouptipitem.cpp
    widgets/listitembase.cpp
    widgets/messagetextedit.cpp
    widgets/picturebubble.cpp
    widgets/searchlist.cpp
    widgets/statewidget.cpp
    widgets/textbubble.cpp
    widgets/timerbtn.cpp
)
set(WIDGET_HEADERS
    widgets/adduseritem.h
    widgets/applyfrienditem.h
    widgets/bubbleframe.h
    widgets/chatitembase.h
    widgets/chatuserlist.h
    widgets/chatuserwid.h
    widgets/chatview.h
    widgets/clickedbtn.h
    widgets/clickedlabel.h
    widgets/clickedoncelabel.h
    widgets/conuseritem.h
    widgets/customizeedit.h
    widgets/friendlabel.h
    widgets/grouptipitem.h
    widgets/listitembase.h
    widgets/messagetextedit.h
    widgets/picturebubble.h
    widgets/searchlist.h
    widgets/statewidget.h
    widgets/textbubble.h
    widgets/timerbtn.h
)
# .ui 文件：AUTOUIC 会自动把它们转成 ui_xxx.h 并参与编译
set(WIDGET_FORMS
    widgets/adduseritem.ui
    widgets/applyfrienditem.ui
    widgets/chatuserwid.ui
    widgets/conuseritem.ui
    widgets/friendlabel.ui
    widgets/grouptipitem.ui
)

# ------------------------------------------------------------
#  dialogs/ — 业务对话框与页面（登录、注册、聊天主界面等）
# ------------------------------------------------------------
set(DIALOG_SOURCES
    dialogs/applyfriend.cpp
    dialogs/applyfriendlist.cpp
    dialogs/applyfriendpage.cpp
    dialogs/authenfriend.cpp
    dialogs/chatdialog.cpp
    dialogs/chatpage.cpp
    dialogs/contactuserlist.cpp
    dialogs/findfaildlg.cpp
    dialogs/findsuccessdlg.cpp
    dialogs/friendinfopage.cpp
    dialogs/loadingdlg.cpp
    dialogs/logindialog.cpp
    dialogs/registerdialog.cpp
    dialogs/resetdialog.cpp
)
set(DIALOG_HEADERS
    dialogs/applyfriend.h
    dialogs/applyfriendlist.h
    dialogs/applyfriendpage.h
    dialogs/authenfriend.h
    dialogs/chatdialog.h
    dialogs/chatpage.h
    dialogs/contactuserlist.h
    dialogs/findfaildlg.h
    dialogs/findsuccessdlg.h
    dialogs/friendinfopage.h
    dialogs/loadingdlg.h
    dialogs/logindialog.h
    dialogs/registerdialog.h
    dialogs/resetdialog.h
)
set(DIALOG_FORMS
    dialogs/applyfriend.ui
    dialogs/applyfriendpage.ui
    dialogs/authenfriend.ui
    dialogs/chatdialog.ui
    dialogs/chatpage.ui
    dialogs/findfaildlg.ui
    dialogs/findsuccessdlg.ui
    dialogs/friendinfopage.ui
    dialogs/loadingdlg.ui
    dialogs/logindialog.ui
    dialogs/registerdialog.ui
    dialogs/resetdialog.ui
)

# ------------------------------------------------------------
#  views/ — 主窗口
# ------------------------------------------------------------
set(VIEW_SOURCES
    views/mainwindow.cpp
)
set(VIEW_HEADERS
    views/mainwindow.h
)
set(VIEW_FORMS
    views/mainwindow.ui
)

# ------------------------------------------------------------
#  可执行目标
#  WIN32 标志：生成 GUI 子系统程序（无控制台窗口），
#  入口仍为 main()，Qt 会按 Windows 方式运行
# ------------------------------------------------------------
add_executable(xiguchat WIN32
    main.cpp
    ${CORE_SOURCES} ${CORE_HEADERS}
    ${NETWORK_SOURCES} ${NETWORK_HEADERS}
    ${WIDGET_SOURCES} ${WIDGET_HEADERS} ${WIDGET_FORMS}
    ${DIALOG_SOURCES} ${DIALOG_HEADERS} ${DIALOG_FORMS}
    ${VIEW_SOURCES} ${VIEW_HEADERS} ${VIEW_FORMS}
    resources/rc.qrc
)

# 各模块目录加入头文件搜索路径，源码内 #include "xxx.h" 可直接引用
target_include_directories(xiguchat PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/network
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/views
)

# 链接 Qt 模块库（必须是 find_package 列出的那 4 个）
target_link_libraries(xiguchat PRIVATE Qt6::Core Qt6::Gui Qt6::Network Qt6::Widgets)

# 源码为 UTF-8 编码：MSVC 需要显式告知（/utf-8），
# 否则默认按系统代码页解析，含中文的源码会产生 C4819 警告或乱码
if(MSVC)
    target_compile_options(xiguchat PRIVATE /utf-8 /wd4819)
endif()

# 输出目录：构建目录下的 bin/（三平台产物统一在 build/<preset>/bin/ 下）
set_target_properties(xiguchat PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# macOS：打成 .app bundle，Dock/程序坞才能显示图标。
# macOS 窗口标题栏不显示图标，setWindowIcon 在 mac 上无效，
# Dock 图标来自 bundle Contents/Resources 里的 .icns。
# xiguchat.icns 由 resources/Facebook.ico（128x128）转换生成。
if(APPLE)
    set_target_properties(xiguchat PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "xiguchat"
        MACOSX_BUNDLE_ICON_FILE "xiguchat.icns"
    )
    target_sources(xiguchat PRIVATE resources/xiguchat.icns)
    set_source_files_properties(resources/xiguchat.icns PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
endif()

# ------------------------------------------------------------
#  构建后部署：把运行必需的 config.ini 与 static/ 资源
#  拷贝到 exe 同目录，保证程序可直接运行
# ------------------------------------------------------------
add_custom_command(TARGET xiguchat POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/config.ini"
        "$<TARGET_FILE_DIR:xiguchat>/config.ini"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/static"
        "$<TARGET_FILE_DIR:xiguchat>/static"
    COMMENT "Copy config.ini and static/ to output"
)
