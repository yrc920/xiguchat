# ============================================================
#  XiguChat Server — 后端微服务（CMake 构建）
#
#  技术栈: C++20 + gRPC + protobuf + boost::asio/beast + hiredis
#          + jsoncpp + MySQL X DevAPI (mysql-connector-cpp)
#  依赖:   vcpkg manifest（server/vcpkg.json），由 vcpkg
#          toolchain 自动安装，版本由 vcpkg baseline 锁定
#  目标:   GateServer / StatusServer / ChatServer
#          三个相互独立的可执行程序（微服务）
#          ChatServer 为一套代码多实例部署，实例由 config/*.ini 决定
#  构建:   三平台预设（windows-msvc / macos-clang / linux-gcc），
#          命令行：cmake --preset <平台预设> [-release]
#                  cmake --build --preset <平台预设>[-release]
# ============================================================
cmake_minimum_required(VERSION 3.21)

# ---------- 环境检查 ----------
# vcpkg 工具链依赖 $ENV{VCPKG_ROOT} 定位，未设置时在此给出清晰指引
if(NOT DEFINED ENV{VCPKG_ROOT})
    message(FATAL_ERROR
        "未设置 VCPKG_ROOT 环境变量（vcpkg 安装目录），请先设置：\n"
        "  Windows:   setx VCPKG_ROOT \"D:\\path\\to\\vcpkg\"\n"
        "  macOS/Linux: export VCPKG_ROOT=~/vcpkg\n"
        "设置后重新打开 VSCode（或终端）再 configure。")
endif()

# ---------- vcpkg 工具链 ----------
# 在 project() 前显式设置（toolchain 在第一个 project() 调用时加载），
# 由本文件统一指定，使命令行、IDE 等各构建入口获得一致的构建环境
set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
    CACHE FILEPATH "vcpkg toolchain file")

project(xiguchat-server VERSION 1.0.0 LANGUAGES CXX)

# ---------- C++ 标准 ----------
# 统一使用 C++20；REQUIRED 表示编译器不支持时直接配置失败，
# EXTENSIONS OFF 禁用 GNU 扩展，保证可移植性
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 把项目自带的 cmake/ 目录加入模块搜索路径，
# 这样下面 include(XiguChatCommon) 才能找到 cmake/XiguChatCommon.cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# ---------- 查找依赖（vcpkg 提供） ----------
# 全部使用 CONFIG 模式：vcpkg 安装的包自带 *Config.cmake，
# REQUIRED 表示缺一不可，缺少会在配置期直接报错。
# gRPC/protobuf：微服务 RPC 通信基础
# Boost.filesystem：配置文件路径处理
# hiredis：Redis 客户端
# jsoncpp：JSON 序列化
# libsodium：口令哈希（Argon2id）
# unofficial-mysql-connector-cpp：MySQL X DevAPI 连接器
# Threads：跨平台线程库（std::thread 依赖）
find_package(gRPC CONFIG REQUIRED)
find_package(protobuf CONFIG REQUIRED)
find_package(Boost CONFIG REQUIRED COMPONENTS filesystem)
find_package(hiredis CONFIG REQUIRED)
find_package(jsoncpp CONFIG REQUIRED)
# unofficial-sodium：libsodium 的口令哈希（Argon2id），用于密码加盐哈希
find_package(unofficial-sodium CONFIG REQUIRED)
find_package(unofficial-mysql-connector-cpp CONFIG REQUIRED)
find_package(Threads REQUIRED)
# mysql-connector-cpp 的 X DevAPI 压缩功能依赖 zstd 与 lz4，
# 但 vcpkg 的 unofficial 配置不会自动传递该依赖，需显式查找
find_package(zstd CONFIG REQUIRED)
find_package(lz4 CONFIG REQUIRED)

# ---------- 公共编译选项 ----------
if(MSVC)
    # 源码为 UTF-8（含中文注释）：
    #   /utf-8   让 MSVC 按 UTF-8 解析源码，避免中文乱码与 C4819
    #   /wd4819  关闭"文件含非 UTF-8 字符"编码警告
    #   /wd4996  关闭"使用不安全 CRT 函数"的弃用告警（如 strcpy 等）
    #   _CRT_SECURE_NO_WARNINGS 等效宏定义
    add_compile_options(/utf-8 /wd4819 /wd4996)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    # 非 MSVC 编译器（GCC/Clang）开启常用警告
    add_compile_options(-Wall -Wextra)
endif()

# ---------- 公共部署函数 ----------
# XiguChatCommon.cmake 提供 xiguchat_deploy()：
# 每个服务构建后自动拷贝 config.ini（三平台）与运行时 DLL（仅 Windows）到输出目录
include(XiguChatCommon)

# ---------- proto 共享库 ----------
# proto/CMakeLists.txt 定义 xigu_proto 静态库：
# 构建期用 protoc 从 proto/message.proto 生成 C++ 代码，
# 4 个服务共用同一份生成代码，避免各服务重复维护 proto
add_subdirectory(proto)

# ---------- 微服务 ----------
# 每个服务目录有独立的 CMakeLists.txt，声明各自的源码、
# 头文件与链接库；四个服务相互独立、分别产出可执行文件
add_subdirectory(services/GateServer)
add_subdirectory(services/StatusServer)
add_subdirectory(services/ChatServer)

# ---------- 统一补链接 ----------
# 三个微服务都链接 mysql-connector-cpp（X DevAPI），
# 统一补上其静态库引用的 zstd 压缩符号（顺序在 mysql 之后）
foreach(server IN ITEMS GateServer StatusServer ChatServer)
    target_link_libraries(${server} PRIVATE zstd::libzstd LZ4::lz4)
endforeach()
