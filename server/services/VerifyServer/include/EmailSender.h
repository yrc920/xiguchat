#pragma once
// 通过 SMTP 发送验证码邮件（POCO Net / NetSSL）
//
// 与 smtp.qq.com 这类"465 端口 SSL 直连"的服务器通信：
// 先建立已握手的 SecureStreamSocket，再在其上跑 SMTP 会话。

#include <string>

class EmailSender {
public:
	//进程启动时初始化 SSL 环境（只调用一次），关闭前用 UninitSSL 释放
	static void InitSSL();
	static void UninitSSL();

	//发送一封纯文本邮件，成功返回 true，失败（网络/认证/服务器拒绝）返回 false
	static bool Send(const std::string& to, const std::string& subject, const std::string& text);
};
