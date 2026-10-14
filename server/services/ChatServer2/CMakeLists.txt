# ============================================================
#  ChatServer2 — TCP 长连接消息服务实例 2（与 ChatServer 独立编译）
#  注意：ChatServer2 的 LogicSystem.cpp 与 ChatServer 存在逻辑差异，
#  必须保持各自独立目录编译，不得合并为同一源码
#
#  依赖: xigu_proto（共享 proto 库）+ gRPC/protobuf + Boost
#        + hiredis + jsoncpp + mysql-connector-cpp
#  产物: 构建目录 bin/ChatServer2/ 下（三平台同一布局，Debug/Release 由预设决定）
# ============================================================

# 可执行文件名（同时用于输出目录与部署）
set(SERVER_NAME ChatServer2)

# ---------- 源文件清单 ----------
# 与 ChatServer 同构：TCP 长连接 + gRPC 服务/客户端，
# 唯一差异在 LogicSystem.cpp 的业务逻辑
set(SERVER_SOURCES
    src/ChatServer.cpp         # 主入口：初始化配置、启动 TCP 与 gRPC
    src/ChatServiceImpl.cpp    # gRPC 服务实现（服务间消息转发）
    src/CServer.cpp            # TCP 监听器（boost::asio acceptor）
    src/CSession.cpp           # 单个 TCP 会话的收发与心跳
    src/LogicSystem.cpp        # 业务分发（与 ChatServer 存在差异）
    src/MsgNode.cpp            # 消息协议编解码节点（包头+包体）
    src/ConfigMgr.cpp          # 配置文件 config.ini 的读取与单例
    src/IOContextPool.cpp      # boost::asio io_context 线程池
    src/RedisMgr.cpp           # Redis 客户端封装（hiredis）
    src/MysqlMgr.cpp           # MySQL 连接池（X DevAPI）
    src/MysqlDao.cpp           # 数据访问层
    src/UserMgr.cpp            # 在线用户管理（uid -> session 映射）
    src/ChatGrpcClient.cpp     # 调用 ChatServer 的 gRPC 客户端
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

# 输出到统一的 bin/<服务名>/ 目录
set_target_properties(${SERVER_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${SERVER_NAME}")

# 构建后部署：拷贝 config.ini 与全部运行时 DLL 到 exe 目录
xiguchat_deploy(${SERVER_NAME})
