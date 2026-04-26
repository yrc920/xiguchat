// 启动gRPC服务器, 提供GetVerifyCode服务, 生成验证码并发送邮件
//

//引入必要的模块, 使用commonjs模块规范
const grpc = require("@grpc/grpc-js");
const message_proto = require("./proto");
const const_module = require("./const");
const { v4: uuidv4 } = require("uuid");
const emailModule = require("./email");
const redis_module = require("./redis");

//call: 请求(相当于proto的req), callback: 回复(相当于proto的rsp)
async function GetVerifyCode(call, callback) {
  console.log("email is ", call.request.email);

  //sendmail函数出现错误时, 会抛出异常, 使用try/catch捕获异常并处理
  try {
    //根据邮箱地址查询redis中是否存在验证码
    let query_res = await redis_module.GetRedis(
      const_module.code_prefix + call.request.email,
    );
    console.log("query_res is ", query_res);

    let uniqueId = query_res; //如果查询结果不为空, 则使用查询结果作为验证码
    //如果查询结果为空, 则生成一个新的验证码并存储到redis中
    if (query_res == null) {
      uniqueId = uuidv4(); //生成一个唯一的验证码

      if (uniqueId.length > 4) {
        uniqueId = uniqueId.substring(0, 4); //为了简化验证码, 只取uuid的前4位
      }
      //将验证码存储到redis中, 键为"code_" + email地址, 值为验证码, 过期时间为600秒(10分钟)
      let bres = await redis_module.SetRedisExpire(
        const_module.code_prefix + call.request.email,
        uniqueId,
        600,
      );
      //如果存储失败, 回复客户端错误信息, 错误类型为RedisErr(redis错误)
      if (!bres) {
        callback(null, {
          email: call.request.email,
          error: const_module.Errors.RedisErr,
        });
        return;
      }
    }

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

    //如果发送邮件失败, 回复客户端错误信息, 错误类型为SendMailErr(发送邮件错误)
    if (!send_res) {
      callback(null, {
        email: call.request.email,
        error: const_module.Errors.SendMailErr,
      });
      return;
    }

    //调用callback函数回复客户端
    callback(null, {
      email: call.request.email,
      error: const_module.Errors.Success,
    });
  } catch (error) {
    console.log("catch error is ", error);

    //如果发送邮件出现错误, 回复客户端错误信息, 错误类型为Exception(异常错误)
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
