// 储存常量和全局得变量
//

let code_prefix = "code_"; //验证码的前缀

const Errors = {
  Success: 0,
  RedisErr: 1, //redis错误
  Exception: 2, //异常错误
  SendMailErr: 3, //发送邮件错误
};

//导出模块
module.exports = { code_prefix, Errors };
