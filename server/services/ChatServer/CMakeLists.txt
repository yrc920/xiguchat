# ============================================================
#  ChatServer — TCP 长连接消息服务（boost::asio + gRPC）
#
#  一套代码、多实例部署：
#    config/<实例名>.ini  ->  build/bin/<实例名>/（可执行文件副本 + config.ini）
#  新增实例只需往 config/ 下再放一份 ini，无需改动本文件。
#
#  依赖: xigu_proto（共享 proto 库）+ gRPC/protobuf + Boost
#        + hiredis + jsoncpp + mysql-connector-cpp
#  产物: 构建目录 bin/<实例名>/ 下（三平台同一布局，Debug/Release 由预设决定）
# ============================================================

set(SERVER_NAME ChatServer)

# ---------- 源文件清单 ----------
# ChatServer 同时承担两类角色：
#   1. 对客户端：TCP 长连接服务（CServer/CSession/LogicSystem）
#   2. 对服务间：gRPC 服务端（ChatServiceImpl）与客户端
#      （ChatGrpcClient 调对端聊天实例、StatusGrpcClient 调 StatusServer）
set(SERVER_SOURCES
    src/ChatServer.cpp         # 主入口：初始化配置、启动 TCP 与 gRPC
    src/ChatServiceImpl.cpp    # gRPC 服务实现（服务间消息转发）
    src/CServer.cpp            # TCP 监听器（boost::asio acceptor）
    src/CSession.cpp           # 单个 TCP 会话的收发
    src/LogicSystem.cpp        # 业务分发：消息路由到处理函数
    src/MsgNode.cpp            # 消息协议编解码节点（包头+包体）
    src/ConfigMgr.cpp          # 配置文件 config.ini 的读取与单例
    src/IOContextPool.cpp      # boost::asio io_context 线程池
    src/RedisMgr.cpp           # Redis 客户端封装（hiredis）
    src/MysqlMgr.cpp           # MySQL 连接池（X DevAPI）
    src/MysqlDao.cpp           # 数据访问层
    src/UserMgr.cpp            # 在线用户管理（uid -> session 映射）
    src/ChatGrpcClient.cpp     # 调用对端聊天实例的 gRPC 客户端
    src/StatusGrpcClient.cpp   # 调用 StatusServer 的 gRPC 客户端
)

# ---------- 头文件清单 ----------
# 与源文件一一对应；列出头文件让 CMake 感知依赖
set(SERVER_HEADERS
    include/ChatServiceImpl.h
    include/CServer.h
    include/CSession.h
    include/LogicSystem.h
    include/MsgNode.h
    include/ConfigMgr.h
    include/IOContextPool.h
    include/RedisMgr.h
    include/MysqlMgr.h
    include/MysqlDao.h
    include/UserMgr.h
    include/ChatGrpcClient.h
    include/StatusGrpcClient.h
    include/Singleton.h
    include/const.h
    include/data.h
)

# ---------- 实例清单 ----------
# 扫描 config/ 下的 ini，每个 ini 对应一个可独立启动的实例。
# CONFIGURE_DEPENDS 让 CMake 在新增/删除 ini 时自动重新配置。
file(GLOB CHAT_INSTANCE_CONFIGS CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/config/*.ini")
if(NOT CHAT_INSTANCE_CONFIGS)
    message(FATAL_ERROR
        "services/ChatServer/config/ 下未找到任何 *.ini，"
        "至少需要一份实例配置（如 config/ChatServer.ini）")
endif()
list(SORT CHAT_INSTANCE_CONFIGS)

# 列表按名字排序后取第一项作为主实例：
# 它的直接产物目录即 bin/<主实例名>/，其余实例目录再复制一份可执行文件
list(GET CHAT_INSTANCE_CONFIGS 0 CHAT_PRIMARY_CFG)
get_filename_component(CHAT_PRIMARY_NAME "${CHAT_PRIMARY_CFG}" NAME_WE)

# ---------- 可执行目标 ----------
add_executable(${SERVER_NAME} ${SERVER_SOURCES} ${SERVER_HEADERS})

# 本服务 include/ 目录加入头文件搜索路径
target_include_directories(${SERVER_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

# ---------- 链接库 ----------
# xigu_proto:            共享 proto 生成代码（静态库）
# gRPC::grpc++:          gRPC C++ 运行时
# protobuf::libprotobuf: protobuf 运行时
# Boost::filesystem:     路径处理
# Boost::headers:        仅头文件的 Boost 库（asio/beast 等）
# hiredis::hiredis:      Redis 客户端
# JsonCpp::JsonCpp:      JSON 库
# unofficial::mysql-connector-cpp::connector: MySQL 连接器
# Threads::Threads:      线程支持
target_link_libraries(${SERVER_NAME} PRIVATE
    xigu_proto
    gRPC::grpc++
    protobuf::libprotobuf
    Boost::filesystem
    Boost::headers
    hiredis::hiredis
    JsonCpp::JsonCpp
    unofficial::mysql-connector-cpp::connector
    Threads::Threads
)
if(WIN32)
    # Windows 下网络编程需要 ws2_32（Winsock）库
    target_link_libraries(${SERVER_NAME} PRIVATE ws2_32)
endif()

# 输出到主实例目录（其余实例在构建后复制可执行文件过去）
set_target_properties(${SERVER_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${CHAT_PRIMARY_NAME}")

# ---------- 实例目录布局 ----------
# 可执行文件与 config.ini 必须落在同一目录，因此实例目录要与
# 可执行文件的实际落点严格对齐。两种生成器的落点规则不同：
#   · 单配置生成器（Ninja / Makefile，含本项目的 macOS/Linux 预设）：
#     CMAKE_BUILD_TYPE 决定配置，输出目录不再分层 -> bin/<实例名>/
#   · 多配置生成器（Visual Studio / Ninja Multi-Config，Windows 预设）：
#     CMake 会在输出目录下再追加一层配置名 -> bin/<实例名>/<配置名>/
# 用 CMAKE_CONFIGURATION_TYPES 区分两者，并让配置层用 $<CONFIG> 表达，
# 构建时才求值，同一份规则在三平台、Debug/Release 下都成立。
if(CMAKE_CONFIGURATION_TYPES)
    set(CHAT_INSTANCE_DIR_SUFFIX "/$<CONFIG>")
else()
    set(CHAT_INSTANCE_DIR_SUFFIX "")
endif()

# ---------- 多实例部署 ----------
# 服务按当前工作目录读取 config.ini，因此每个实例必须有独立目录：
#   build/bin/ChatServer/   <- config/ChatServer.ini
#   build/bin/ChatServer2/  <- config/ChatServer2.ini
# 启动方式与单实例完全相同：进入各自目录运行 ChatServer。
foreach(cfg_path IN LISTS CHAT_INSTANCE_CONFIGS)
    get_filename_component(inst_name "${cfg_path}" NAME_WE)
    set(inst_dir "${CMAKE_BINARY_DIR}/bin/${inst_name}${CHAT_INSTANCE_DIR_SUFFIX}")

    # 非主实例：先建目录，再把可执行文件复制过去
    # （copy_if_different 会保留可执行权限，三平台一致）
    if(NOT inst_name STREQUAL CHAT_PRIMARY_NAME)
        add_custom_command(TARGET ${SERVER_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${inst_dir}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:${SERVER_NAME}>" "${inst_dir}/"
            COMMENT "Deploy ChatServer executable -> bin/${inst_name}")
    endif()

    # 每个实例部署自己的 config.ini（统一命名为 config.ini）
    add_custom_command(TARGET ${SERVER_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${cfg_path}" "${inst_dir}/config.ini"
        COMMENT "Deploy config.ini -> bin/${inst_name}")

    # Windows：依赖的 DLL 必须与可执行文件同目录，逐实例复制
    if(WIN32)
        add_custom_command(TARGET ${SERVER_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:${SERVER_NAME}> "${inst_dir}/"
            COMMAND_EXPAND_LISTS
            COMMENT "Copy runtime DLLs -> bin/${inst_name}")
    endif()
endforeach()
