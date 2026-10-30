# ============================================================
#  XiguChat Client — 聊天客户端（Qt6 桌面程序）
#
#  技术栈: Qt 6.5 (Core / Gui / Network / Quick) + C++17
#
#  目标划分（库边界由链接器强制，不靠自觉）:
#    xiguchat_core  静态库：core/ 业务与数据 + network/ HTTP/TCP 传输
#                   只链接 Qt Core / Network，
#                   不依赖界面头与 Gui/Quick，保持无界面测试能力
#    xiguchat       可执行：main.cpp + bridge/ + qml/ 界面层
#
#  界面用 QML 描述：qml/ 放页面与组件，bridge/ 是 QML 与 core 的唯一通道
#  （Q_PROPERTY / Q_INVOKABLE / QAbstractListModel）。
#
#  构建入口（两条路径共用本文件）:
#    1) Qt Creator：选择 Qt 6.5 Kit 打开工程，Kit 自动注入编译器与 Qt 路径
#    2) 命令行：cmake --preset <平台预设> && cmake --build --preset <平台预设>
#       三平台均为单配置生成器（Ninja），Debug/Release 由预设区分
# ============================================================
cmake_minimum_required(VERSION 3.16)

# 编译器（跨平台统一由 Kit 或系统提供）：macOS/Linux 使用系统 clang/gcc，
# 无需任何设置；Windows 使用 Qt 安装包自带的 MinGW 11.2（mingw1120_64），
# 两种来源任选其一，互不覆盖：
#  1) Qt Creator：选择 Qt 6.5 MinGW Kit 打开工程，Kit 直接指定编译器
#  2) 命令行/VSCode：QT_MINGW_ROOT 环境变量（在 project() 之前设置）
# 注意：Windows 端不使用 MSVC——Qt 官方安装包的 Windows 桌面套件是
# MinGW 构建，与 MSVC 编译出的库 ABI 不兼容，两者不能混用。
if(WIN32 AND NOT CMAKE_CXX_COMPILER)
    if(NOT DEFINED ENV{QT_MINGW_ROOT})
        message(FATAL_ERROR
            "未指定 Windows 编译器，两种方式任选其一：\n"
            "  1) Qt Creator：选择 Qt 6.5 MinGW Kit 打开工程\n"
            "  2) 命令行/VSCode：setx QT_MINGW_ROOT \"D:\\Qt\\Tools\\mingw1120_64\"\n"
            "     （必须是与 Qt 6.5 配套的 MinGW 11.2，即 Qt 自带 mingw1120_64，\n"
            "      其他版本 ABI 不兼容），设置后重新打开 VSCode 再 configure。")
    endif()
    set(CMAKE_C_COMPILER "$ENV{QT_MINGW_ROOT}/bin/gcc.exe")
    set(CMAKE_CXX_COMPILER "$ENV{QT_MINGW_ROOT}/bin/g++.exe")
endif()

# Windows 的 EXE 图标由同一套 MinGW 工具链的 windres 嵌入。
if(WIN32 AND NOT CMAKE_RC_COMPILER AND NOT DEFINED ENV{RC})
    get_filename_component(XIGUCHAT_MINGW_BIN "${CMAKE_CXX_COMPILER}" DIRECTORY)
    find_program(CMAKE_RC_COMPILER NAMES windres.exe windres
        HINTS "${XIGUCHAT_MINGW_BIN}" "$ENV{QT_MINGW_ROOT}/bin")
endif()

project(xiguchat VERSION 1.0.0 LANGUAGES CXX)

# ---------- C++ 标准 ----------
# 项目统一使用 C++17，编译期强制（不满足则报错）
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------- Qt 自动工具 ----------
# moc（元对象编译）与 rcc（资源编译）由 CMake 自动处理：
# 源码中出现的 Q_OBJECT 类、.qrc 文件都不用手写额外步骤。
# 静态库与可执行目标共用这些开关，xiguchat_core 中的 Q_OBJECT 类同样会被 moc。
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
# 让编译器默认在当前目录找头文件（配合下方 include 目录使用）
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# ---------- 查找 Qt（Windows/macOS/Linux 通用）----------
# Qt 安装路径两种来源（任选其一，互不覆盖）：
#  1) Qt Creator：选择本机 Qt 6.5 Kit（三平台通用），Kit 自动注入 CMAKE_PREFIX_PATH
#  2) 命令行/VSCode：QT_ROOT 环境变量
find_package(Qt6 6.5 QUIET COMPONENTS Core Gui Network Quick)
if(NOT Qt6_FOUND AND DEFINED ENV{QT_ROOT})
    list(APPEND CMAKE_PREFIX_PATH "$ENV{QT_ROOT}")
    find_package(Qt6 6.5 QUIET COMPONENTS Core Gui Network Quick)
endif()
if(NOT Qt6_FOUND)
    message(FATAL_ERROR
        "找不到 Qt 6.5（Core/Gui/Network/Quick），两种方式任选其一：\n"
        "  1) Qt Creator：选择本机 Qt 6.5 Kit 打开工程\n"
        "  2) 命令行/VSCode：设置 QT_ROOT 环境变量\n"
        "       macOS/Linux: export QT_ROOT=~/Qt/6.5.3/macos\n"
        "       Windows:      setx QT_ROOT D:\\Qt\\6.5.3\\mingw_64")
endif()

# ------------------------------------------------------------
#  xiguchat_core — 业务与传输层静态库
#
#  只链接 Qt Core / Network：这里没有一行界面代码，
#  界面层的一切（QML / 窗口 / 绘图）都在这个库之外。
# ------------------------------------------------------------
set(CORE_SOURCES
    core/global.cpp
    core/userdata.cpp
    core/usermgr.cpp
    core/service/authservice.cpp
    core/service/sessionservice.cpp
    core/service/chatservice.cpp
    core/service/friendservice.cpp
)

set(CORE_HEADERS
    core/global.h
    core/singleton.h
    core/userdata.h
    core/usermgr.h
    core/service/authservice.h
    core/service/sessionservice.h
    core/service/chatservice.h
    core/service/friendservice.h
)

set(NETWORK_SOURCES
    network/httpmgr.cpp
    network/tcpmgr.cpp
)
set(NETWORK_HEADERS
    network/httpmgr.h
    network/tcpmgr.h
)

add_library(xiguchat_core STATIC
    ${CORE_SOURCES} ${CORE_HEADERS}
    ${NETWORK_SOURCES} ${NETWORK_HEADERS}
)

# 让 IDE（Qt Creator / VS / Xcode）按真实目录树展示源文件，
# 而不是默认的 "Header Files / Source Files" 两堆
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES ${CORE_SOURCES} ${CORE_HEADERS} ${NETWORK_SOURCES} ${NETWORK_HEADERS})

# PUBLIC：链接 xiguchat_core 的目标自动获得这三个头文件搜索路径，
# 源码里 #include "usermgr.h" / "httpmgr.h" 保持原样
target_include_directories(xiguchat_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/core/service
    ${CMAKE_CURRENT_SOURCE_DIR}/network
)

target_link_libraries(xiguchat_core PUBLIC
    Qt6::Core
    Qt6::Network
)

# ------------------------------------------------------------
#  bridge/ — QML 与 core 的边界
#
#  页面桥接对象与列表模型：Q_PROPERTY 暴露状态、Q_INVOKABLE 暴露动作、
#  QAbstractListModel 暴露列表。带 QML_ELEMENT 的类必须编译进可执行目标，
#  放进静态库会与 qt_add_qml_module 各生成一份 moc，链接期符号重复。
# ------------------------------------------------------------
set(BRIDGE_SOURCES
    bridge/shellbridge.cpp
    bridge/loginbridge.cpp
    bridge/registerbridge.cpp
    bridge/resetbridge.cpp
    bridge/friendlistmodel.cpp
    bridge/applylistmodel.cpp
    bridge/messagemodel.cpp
    bridge/composerbridge.cpp
    bridge/chatbridge.cpp
)
set(BRIDGE_HEADERS
    bridge/shellbridge.h
    bridge/pagebridge.h
    bridge/loginbridge.h
    bridge/registerbridge.h
    bridge/resetbridge.h
    bridge/friendlistmodel.h
    bridge/applylistmodel.h
    bridge/messagemodel.h
    bridge/composerbridge.h
    bridge/chatbridge.h
)

# ------------------------------------------------------------
#  可执行目标
#  WIN32 标志：仅 Windows 生效——生成 GUI 子系统程序（无控制台黑窗），
#  入口仍为 main()；macOS/Linux 下该标志被忽略，Qt 自动处理 GUI 程序
# ------------------------------------------------------------
add_executable(xiguchat WIN32
    main.cpp
    ${BRIDGE_SOURCES} ${BRIDGE_HEADERS}
    resources/rc.qrc
)

target_include_directories(xiguchat PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/bridge
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES main.cpp ${BRIDGE_SOURCES} ${BRIDGE_HEADERS} resources/rc.qrc)

# ------------------------------------------------------------
#  qml/ — 界面描述层
#
#  根窗口 Main.qml 只负责选页面、定尺寸、提示掉线；
#  颜色、字号、间距集中在 Theme.qml 单例。
#  单例声明必须写在 qt_add_qml_module 之前，否则属性全读成 undefined。
# ------------------------------------------------------------
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

set(QML_FILES
    qml/Main.qml
    qml/Theme.qml
    qml/BrandMark.qml
    qml/AppIcon.qml
    qml/PaperCompanion.qml
    qml/AuthPage.qml
    qml/Login.qml
    qml/Register.qml
    qml/Reset.qml
    qml/ChatWindow.qml
    qml/FriendDialogs.qml
    qml/ChatPage.qml
    qml/FriendInfoPage.qml
    qml/ApplyFriendPage.qml
    qml/ChatBubble.qml
    qml/Avatar.qml
    qml/IconButton.qml
    qml/ToolIcon.qml
    qml/SideButton.qml
    qml/AppInput.qml
    qml/AppButton.qml
    qml/AppLink.qml
    qml/CodeButton.qml
    qml/Spacer.qml
)

qt_add_qml_module(xiguchat
    URI XiguChat
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    QML_FILES ${QML_FILES}
    SOURCES ${BRIDGE_HEADERS}
)

# 链接关系即分层关系：可执行依赖业务库与 Quick，业务库不认识它们
target_link_libraries(xiguchat PRIVATE
    xiguchat_core
    Qt6::Gui
    Qt6::Quick
)

# 输出目录：构建目录下的 bin/（三平台产物统一在 build/<preset>/bin/ 下）
set_target_properties(xiguchat PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# SVG 是图标源文件；PNG 用于页面与窗口，ICO/ICNS 用于系统应用图标。
# Windows 需将图标嵌入 EXE，macOS 需随 bundle 放入 Resources。
if(WIN32)
    target_sources(xiguchat PRIVATE resources/xiguchat.rc)
    target_include_directories(xiguchat PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/resources")
endif()
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
#  构建后部署（三平台统一）：把运行必需的 config.ini 拷到产物同目录
#  （Windows/Linux 为 bin/，macOS 为 .app/Contents/MacOS/），
#  保证程序可直接运行
# ------------------------------------------------------------
# config.ini 含服务器地址与密钥，仓库只有 config.ini.example，缺失时用它代替
set(XIGUCHAT_CLIENT_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/config.ini")
if(NOT EXISTS "${XIGUCHAT_CLIENT_CONFIG}")
    set(XIGUCHAT_CLIENT_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/config.ini.example")
endif()

add_custom_command(TARGET xiguchat POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${XIGUCHAT_CLIENT_CONFIG}"
        "$<TARGET_FILE_DIR:xiguchat>/config.ini"
    COMMENT "Copy config.ini to output"
)

# ------------------------------------------------------------
#  Qt 运行库部署（仅 Windows 需要）：macOS/Linux 的 Qt 库位于系统
#  标准路径，构建产物直接可运行；Windows 的 Qt 为绿色安装，
#  DLL 不在系统路径，需通过 windeployqt 把 Qt6*.dll、平台插件
#  （platforms/qwindows.dll）与 MinGW 运行时（libgcc/libstdc++/
#  libwinpthread）拷到产物同目录，保证双击即可运行，无需配置
#  Qt 的 PATH
# ------------------------------------------------------------
if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt
        HINTS "$ENV{QT_ROOT}/bin" "${CMAKE_PREFIX_PATH}/bin")
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET xiguchat POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                    --compiler-runtime --no-translations
                    --qmldir "${CMAKE_CURRENT_SOURCE_DIR}/qml"
                    "$<TARGET_FILE:xiguchat>"
            COMMENT "Deploy Qt runtime libraries via windeployqt")
    endif()
endif()
