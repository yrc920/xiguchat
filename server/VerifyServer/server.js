// 启动gRPC服务器, 提供GetVerifyCode服务, 生成验证码并发送邮件
//

//引入必要的模块, 使用commonjs模块规范
const grpc = require("@grpc/grpc-js");
const message_proto = require("./proto");
const const_module = require("./const");
const { v4: uuidv4 } = require("uuid");
const emailModule = require("./email");

//call: 请求(相当于proto的req), callback: 回复(相当于proto的rsp)
async function GetVerifyCode(call, callback) {
  console.log("email is ", call.request.email);

  //sendmail函数出现错误时, 会抛出异常, 使用try/catch捕获异常并处理
  try {
    uniqueId = uuidv4(); //生成一个唯一的验证码
    console.log("uniqueId is ", uniqueId);
    let text_str = "您的验证码为" + uniqueId + "请三分钟内完成注册";

    //构造邮件内容, 包含发送方邮箱地址, 收件人邮箱地址, 邮件主题和邮件正文
    let mailOptions = {
      from: "3099154616@qq.com",
      to: call.request.email,
      subject: "验证码",
      text: text_str,
    };

    //发送邮件, 等待发送结果(使用await关键字等待Promise对象的结果)
    let send_res = await emailModule.SendMail(mailOptions);
    console.log("send res is ", send_res);

    //调用callback函数回复客户端
    callback(null, {
      email: call.request.email,
      error: const_module.Errors.Success,
    });
  } catch (error) {
    console.log("catch error is ", error);

    //如果发送邮件出现错误, 回复客户端错误信息, 错误类型为Exception
    callback(null, {
      email: call.request.email,
      error: const_module.Errors.Exception,
    });
  }
}

function main() {
  var server = new grpc.Server(); //创建一个新的grpc服务器实例

  //将VerifyService服务添加到服务器中, GetVerifyCode函数作为服务的实现,
  //message_proto.VerifyService.service是从.proto文件中解析得到的服务定义
  server.addService(message_proto.VerifyService.service, {
    GetVerifyCode: GetVerifyCode, //调用服务的名字: 具体的接口实现函数
  });

  //绑定服务器到指定的地址和端口, 使用不安全的服务器凭证(不使用SSL加密)
  server.bindAsync(
    "0.0.0.0:50051",
    grpc.ServerCredentials.createInsecure(),
    () => {
      server.start(); //启动服务器, 开始监听客户端的请求
      console.log("grpc server started");
    },
  );
}

main(); //调用main函数, 启动服务器
