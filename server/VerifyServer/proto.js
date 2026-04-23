// 解析 .proto文件, 获取服务定义和消息类型
//

//引入必要的模块, 使用commonjs模块规范
const path = require("path");
const grpc = require("@grpc/grpc-js");
const protoLoader = require("@grpc/proto-loader");

//读取 .proto文件
const PROTO_PATH = path.join(__dirname, "message.proto");
//加载 .proto文件
const packageDefinition = protoLoader.loadSync(PROTO_PATH, {
  keepCase: true, //保持原有字段的大小写
  longs: String, //将long类型转换为字符串(避免整数溢出)
  enums: String, //将enum类型转换为字符串
  defaults: true, //未明确设置的字段使用默认值
  oneofs: true, //支持Protocol Buffers中的oneof特性
});

//grpc加载包定义并解析
const protoDescriptor = grpc.loadPackageDefinition(packageDefinition);
//获取message包中的服务定义, 也就是.proto文件中定义的package message
const message_proto = protoDescriptor.message;

//导出message_proto对象, 供其他模块使用
module.exports = message_proto;
