# ============================================================
#  GateServer — HTTP 网关（boost::beast）
#  对外暴露 HTTP 接口，转发 gRPC 请求到后端服务
#
#  依赖: xigu_proto（共享 proto 库）+ gRPC/protobuf + Boost
#        + hiredis + jsoncpp + mysql-connector-cpp
#  产物: build/windows-msvc/bin/GateServer/Debug/GateServer.exe
# ============================================================

# 可执行文件名（同时用于输出目录与部署）
set(SERVER_NAME GateServer)

# ---------- 源文件清单 ----------
# 每个服务自持源码：GateServer 同时消费 Status 与 Verify
# 两个 gRPC 服务，因此链接对应的客户端封装
set(SERVER_SOURCES
    src/GateServer.cpp        # 主入口：初始化配置、Redis、MySQL、服务
    src/CServer.cpp           # HTTP 服务骨架（boost::beast 监听）
    src/HttpConnection.cpp    # 单个 HTTP 连接的处理
    src/LogicSystem.cpp       # 业务分发：URL 路由 -> 具体处理函数
    src/ConfigMgr.cpp         # 配置文件 config.ini 的读取与单例
    src/IOContextPool.cpp     # boost::asio io_context 线程池
    src/RedisMgr.cpp          # Redis 客户端封装（hiredis）
    src/MysqlMgr.cpp          # MySQL 连接池（X DevAPI）
    src/MysqlDao.cpp          # 数据访问层（用户表查询等）
    src/StatusGrpcClient.cpp  # 调用 StatusServer 的 gRPC 客户端
    src/VerifyGrpcClient.cpp  # 调用 VerifyServer 的 gRPC 客户端
)

# ---------- 头文件清单 ----------
# 与源文件一一对应；列出头文件主要是让 IDE 与 CMake
# 能感知依赖（改动头文件会触发对应源文件重编）
set(SERVER_HEADERS
    include/CServer.h
    include/HttpConnection.h
    include/LogicSystem.h
    include/ConfigMgr.h
    include/IOContextPool.h
    include/RedisMgr.h
    include/MysqlMgr.h
    include/MysqlDao.h
    include/StatusGrpcClient.h
    include/VerifyGrpcClient.h
    include/Singleton.h
    include/const.h
)

# ---------- 可执行目标 ----------
add_executable(${SERVER_NAME} ${SERVER_SOURCES} ${SERVER_HEADERS})

# 本服务 include/ 目录加入头文件搜索路径，
# 源码内 #include "xxx.h" 可直接引用本服务头文件
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

# 输出到统一的 bin/<服务名>/ 目录（Debug/Release 由配置决定）
set_target_properties(${SERVER_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${SERVER_NAME}")

# 构建后部署：拷贝 config.ini 与全部运行时 DLL 到 exe 目录
xigu_deploy(${SERVER_NAME})
