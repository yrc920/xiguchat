// 发送邮件的模块
//

//引入必要的模块, 使用commonjs模块规范
const nodemailer = require("nodemailer");
const config_module = require("./config");

/**
 * 创建发送邮件的代理
 */
let transport = nodemailer.createTransport({
  host: "smtp.qq.com", //邮件服务器地址, 这里使用QQ邮箱的SMTP服务器
  port: 465, //邮件服务器端口, 465是SSL加密的端口
  secure: true, //使用SSL加密连接邮件服务器, 有安全验证
  auth: {
    user: config_module.email_user, //发送方邮箱地址
    pass: config_module.email_pass, //邮箱授权码或者密码
  },
});

/**
 * 发送邮件的函数
 * @param {*} mailOptions_ 发送邮件的参数
 * @returns
 */
function SendMail(mailOptions_) {
  //返回一个Promise对象, 可以使用async/await语法处理异步操作(实现异步改为同步的效果)
  return new Promise(function (resolve, reject) {
    //使用transport对象发送邮件, mailOptions_包含邮件的内容和收件人等信息
    transport.sendMail(mailOptions_, function (error, info) {
      if (error) {
        console.log(error);
        reject(error); //使用reject函数将错误信息传递给调用者
      } else {
        console.log("邮件已成功发送：" + info.response);
        resolve(info.response); //使用resolve函数将成功信息传递给调用者
      }
    });
  });
}

//导出SendMail函数, 供其他模块使用
module.exports.SendMail = SendMail;
