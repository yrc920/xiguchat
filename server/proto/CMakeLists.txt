# ============================================================
#  xigu_proto — message.proto 构建期生成 + 编译为静态库
#
#  作用:
#    1. 用 protoc（protobuf 编译器）把 proto/message.proto
#       生成 C++ 代码 message.pb.*（消息结构体）
#    2. 用 grpc_cpp_plugin 生成 message.grpc.pb.*（gRPC 服务骨架）
#    3. 打包成静态库 xigu_proto，供 4 个服务链接复用
#
#  protoc 与 grpc_cpp_plugin 均由 vcpkg 安装，
#  位于 installed/<triplet>/tools/ 下，版本由 vcpkg 锁定
# ============================================================

# ---------- 定位工具 ----------
# 从 vcpkg 安装目录查找 protoc 与 grpc_cpp_plugin，
# NO_DEFAULT_PATH 避免误用系统里其他版本，REQUIRED 缺失即报错
find_program(XIGU_PROTOC protoc
    HINTS "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/protobuf"
    NO_DEFAULT_PATH REQUIRED)
find_program(XIGU_GRPC_CPP_PLUGIN grpc_cpp_plugin
    HINTS "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/grpc"
    NO_DEFAULT_PATH REQUIRED)

# 生成代码输出到本目录构建子目录下的 generated/
set(_gen_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${_gen_dir}")

# VERBATIM 由 CMake 处理三平台命令行转义，支持路径含空格。
# protoc/plugin 的可执行文件也加入 DEPENDS，其时间戳变化时重新生成，
# 避免只更新依赖工具却继续使用原先生成的代码。
# ---------- 生成消息代码（protobuf） ----------
# add_custom_command 定义"生成动作"：
#   输入  message.proto（DEPENDS 跟踪变更，proto 改了会自动重新生成）
#   输出  message.pb.cc / message.pb.h
#   命令  protoc --cpp_out=<输出目录> -I <proto 所在目录> message.proto
add_custom_command(
    OUTPUT "${_gen_dir}/message.pb.cc" "${_gen_dir}/message.pb.h"
    COMMAND "${XIGU_PROTOC}" "--cpp_out=${_gen_dir}"
        -I "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/message.proto"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/message.proto" "${XIGU_PROTOC}"
    VERBATIM
    COMMENT "Generate protobuf code (message.pb.*)")

# ---------- 生成服务代码（gRPC） ----------
# 同样的机制生成 gRPC 服务骨架代码：
#   --grpc_out 指定输出目录
#   --plugin=protoc-gen-grpc=<插件路径> 指定 gRPC 代码生成插件
add_custom_command(
    OUTPUT "${_gen_dir}/message.grpc.pb.cc" "${_gen_dir}/message.grpc.pb.h"
    COMMAND "${XIGU_PROTOC}" "--grpc_out=${_gen_dir}"
        "--plugin=protoc-gen-grpc=${XIGU_GRPC_CPP_PLUGIN}"
        -I "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/message.proto"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/message.proto"
        "${XIGU_PROTOC}" "${XIGU_GRPC_CPP_PLUGIN}"
    VERBATIM
    COMMENT "Generate gRPC code (message.grpc.pb.*)")

# ---------- 静态库 ----------
# 编译两个生成的 .cc 文件并打包为 xigu_proto，两个 .h 文件供生成代码和各服务包含
add_library(xigu_proto STATIC
    "${_gen_dir}/message.pb.cc"
    "${_gen_dir}/message.grpc.pb.cc")

# PUBLIC：generated/ 与 proto 目录暴露给链接方，
# 服务代码只需 #include "message.pb.h" 即可引用
target_include_directories(xigu_proto PUBLIC
    "${_gen_dir}"
    "${CMAKE_CURRENT_SOURCE_DIR}")

# 链接 gRPC 与 protobuf 运行时库（PUBLIC 传递给下游服务）
target_link_libraries(xigu_proto PUBLIC
    gRPC::grpc++
    protobuf::libprotobuf)
