# ============================================================
#  VerifyServer — 验证码服务（gRPC + Redis + SMTP）
#  生成验证码、写入 Redis 并通过邮件发送给用户
#
#  依赖: xigu_proto（共享 proto 库）+ gRPC/protobuf + Boost
#        + hiredis + Poco Net/NetSSL
#  产物: 构建目录 bin/VerifyServer/ 下（三平台同一布局，Debug/Release 由预设决定）
# ============================================================

# 可执行文件名（同时用于输出目录与部署）
set(SERVER_NAME VerifyServer)

# ---------- 源文件清单 ----------
set(SERVER_SOURCES
    src/VerifyServer.cpp      # 主入口：启动 gRPC 服务，注册信号处理
    src/VerifyServiceImpl.cpp # gRPC 服务实现：验证码生成、Redis 读写、发信调度
    src/EmailSender.cpp       # SMTP 客户端封装（POCO）
    src/RedisMgr.cpp          # Redis 客户端封装（hiredis）
    src/ConfigMgr.cpp         # 配置文件 config.ini 的读取与单例
)

# ---------- 头文件清单 ----------
# 与源文件一一对应；列出头文件让 CMake 感知依赖
set(SERVER_HEADERS
    include/VerifyServiceImpl.h
    include/EmailSender.h
    include/RedisMgr.h
    include/ConfigMgr.h
    include/Singleton.h
    include/const.h
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
# Boost::headers:        仅头文件的 Boost 库（含 uuid）
# hiredis::hiredis:      Redis 客户端
# Poco::NetSSL:          SMTP over SSL（验证码邮件）
# Poco::Net:             SMTP/MailMessage 与 Socket 基础（Poco::Foundation 由其传递引入）
# Threads::Threads:      线程支持
target_link_libraries(${SERVER_NAME} PRIVATE
    xigu_proto
    gRPC::grpc++
    protobuf::libprotobuf
    Boost::filesystem
    Boost::headers
    hiredis::hiredis
    Poco::NetSSL
    Poco::Net
    Threads::Threads
)
if(WIN32)
    # Windows 下网络编程需要 ws2_32（Winsock）库
    target_link_libraries(${SERVER_NAME} PRIVATE ws2_32)
endif()

# 输出到统一的 bin/VerifyServer/ 目录
set_target_properties(${SERVER_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${SERVER_NAME}")

# 构建后部署：拷贝 config.ini 与全部运行时 DLL 到 exe 目录
xiguchat_deploy(${SERVER_NAME})
