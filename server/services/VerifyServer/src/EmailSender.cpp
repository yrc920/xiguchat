#include "EmailSender.h"
#include "ConfigMgr.h"

#include <Poco/Net/AcceptCertificateHandler.h>
#include <Poco/Net/Context.h>
#include <Poco/Net/MailMessage.h>
#include <Poco/Net/MailRecipient.h>
#include <Poco/Net/SSLManager.h>
#include <Poco/Net/SecureStreamSocket.h>
#include <Poco/Net/SMTPClientSession.h>
#include <Poco/Net/StringPartSource.h>
#include <Poco/Exception.h>

void EmailSender::InitSSL()
{
	// POCO 使用 OpenSSL 前必须先初始化 NetSSL 库
	Poco::Net::initializeSSL();

	// 服务端证书校验失败时走 AcceptCertificateHandler（配合 VERIFY_RELAXED：
	// 会校验证书链与主机名，但允许出现校验问题时不中断，由 handler 决定是否接受）
	Poco::Net::SSLManager::instance().initializeClient(
		nullptr,
		new Poco::Net::AcceptCertificateHandler(false),
		nullptr);
}

void EmailSender::UninitSSL()
{
	Poco::Net::uninitializeSSL(); //释放 OpenSSL 资源
}

bool EmailSender::Send(const std::string& to, const std::string& subject, const std::string& text)
{
	auto& cfg = ConfigMgr::Inst();
	auto host = cfg["Email"]["Host"]; //SMTP 服务器地址，如 smtp.qq.com
	auto port_str = cfg["Email"]["Port"]; //SMTP 端口，SSL 直连通常为 465
	auto user = cfg["Email"]["User"]; //发件邮箱账号
	auto pass = cfg["Email"]["Pass"]; //邮箱授权码
	auto from = cfg["Email"]["From"]; //发件人地址（通常与账号相同）
	auto ca_location = cfg["Email"]["CA"]; //CA 证书路径，留空则用 OpenSSL 默认证书目录

	if (from.empty()) {
		from = user; //未单独配置发件人时取账号
	}

	try {
		//构建客户端 SSL 上下文：
		// TLS_CLIENT_USE 表示只做 TLS 客户端；loadDefaultCAs 打开内置 CA，
		// CA 配置了具体路径时以该路径为准（Linux/macOS 常见 /etc/ssl/cert.pem）
		Poco::Net::Context::Ptr pContext = new Poco::Net::Context(
			Poco::Net::Context::TLS_CLIENT_USE,
			ca_location,
			Poco::Net::Context::VERIFY_RELAXED,
			9,
			ca_location.empty(),
			"ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");

		Poco::Net::SocketAddress addr(host, static_cast<Poco::UInt16>(atoi(port_str.c_str())));
		//465 端口是"连接即 TLS"的隐式 SSL：构造连接时完成握手，hostName 用于证书主机名校验
		Poco::Net::SecureStreamSocket socket(addr, host, pContext);

		Poco::Net::SMTPClientSession session(socket);
		session.login(host); //发送 EHLO，读取服务器初始响应
		session.login(Poco::Net::SMTPClientSession::AUTH_LOGIN, user, pass); //用授权码认证

		Poco::Net::MailMessage message;
		message.setSender(from);
		message.addRecipient(Poco::Net::MailRecipient(
			Poco::Net::MailRecipient::PRIMARY_RECIPIENT, to));
		//中文主题与正文必须按 RFC2047/UTF-8 编码，否则客户端显示为乱码
		message.setSubject(Poco::Net::MailMessage::encodeWord(subject, "UTF-8"));
		message.setContentType("text/plain; charset=UTF-8");
		message.addContent(new Poco::Net::StringPartSource(text, "text/plain; charset=UTF-8"));

		session.sendMessage(message);
		session.close(); //发送 QUIT 并关闭连接
		std::cout << "Succeed to send mail to " << to << std::endl;
		return true;
	}
	catch (Poco::Exception& e) {
		std::cerr << "Send mail failed: " << e.displayText() << std::endl;
		return false;
	}
	catch (std::exception& e) {
		std::cerr << "Send mail failed: " << e.what() << std::endl;
		return false;
	}
}
