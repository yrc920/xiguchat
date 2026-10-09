// 读取配置文件
//

const fs = require("fs"); //引入文件系统模块, 用于读取配置文件

//读取config.json文件, 使用json.parse解析
let config = JSON.parse(fs.readFileSync("config.json", "utf8"));
let email_user = config.email.user;
let email_pass = config.email.pass;
let mysql_host = config.mysql.host;
let mysql_port = config.mysql.port;
let mysql_passwd = config.mysql.passwd;
let redis_host = config.redis.host;
let redis_port = config.redis.port;
let redis_passwd = config.redis.passwd;
let code_prefix = "code_"; //验证码的前缀, 用于在redis中存储验证码时区分不同类型的数据

//导出模块, 供其他模块使用
module.exports = {
  email_pass,
  email_user,
  mysql_host,
  mysql_port,
  mysql_passwd,
  redis_host,
  redis_port,
  redis_passwd,
  code_prefix,
};
