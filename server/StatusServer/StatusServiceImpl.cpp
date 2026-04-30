#include "StatusServiceImpl.h"
#include "ConfigMgr.h"
#include "const.h"

StatusServiceImpl::StatusServiceImpl()
{
	auto& cfg = ConfigMgr::Inst(); //获取配置管理器实例
	//从配置文件中读取聊天服务器的信息, 初始化服务器列表
	ChatServer server;
	server.port = cfg["ChatServer1"]["Port"];
	server.host = cfg["ChatServer1"]["Host"];
	server.con_count = 0;
	server.name = cfg["ChatServer1"]["Name"];
	_servers[server.name] = server;

	server.port = cfg["ChatServer2"]["Port"];
	server.host = cfg["ChatServer2"]["Host"];
	server.name = cfg["ChatServer2"]["Name"];
	server.con_count = 0;
	_servers[server.name] = server;
}

//生成一个唯一的字符串, 用于Token的生成
std::string generate_unique_string()
{
	boost::uuids::uuid uuid = boost::uuids::random_generator()(); //创建UUID对象
	std::string unique_string = to_string(uuid); //将UUID转换为字符串
	return unique_string;
}

Status StatusServiceImpl::GetChatServer(ServerContext* context,
	const GetChatServerReq* request, GetChatServerRsp* reply)
{
	std::string prefix("llfc status server has received :  ");
	const auto& server = getChatServer(); //从服务器列表中获取连接数最少的服务器信息
	reply->set_host(server.host); //设置返回的服务器IP地址
	reply->set_port(server.port); //设置返回的服务器端口号
	reply->set_error(ErrorCodes::Success); //设置返回的错误码, 表示成功
	reply->set_token(generate_unique_string()); //生成并设置返回的Token
	insertToken(request->uid(), reply->token()); //将用户id和生成的Token插入到用户token列表中
	return Status::OK;
}

ChatServer StatusServiceImpl::getChatServer()
{
	std::lock_guard<std::mutex> guard(_server_mtx); //加锁保护服务器列表的访问
	auto minServer = _servers.begin()->second; //假设第一个服务器的连接数最少
	for (const auto& server : _servers) {
		//找到连接数最少的服务器
		if (server.second.con_count < minServer.con_count) {
			minServer = server.second;
		}
	}
	return minServer; //返回连接数最少的服务器信息
}

Status StatusServiceImpl::Login(ServerContext* context, const LoginReq* request, LoginRsp* reply)
{
	auto uid = request->uid(); //获取请求的用户id
	auto token = request->token(); //获取请求的Token

	std::lock_guard<std::mutex> guard(_token_mtx); //加锁保护用户token列表的访问
	auto iter = _tokens.find(uid); //在用户token列表中查找用户id对应的Token
	//如果用户id不存在
	if (iter == _tokens.end()) {
		reply->set_error(ErrorCodes::UidInvalid); //设置返回的错误码, 表示用户id无效
		return Status::OK;
	}
	//如果用户id对应的Token与请求的Token不匹配
	if (iter->second != token) {
		reply->set_error(ErrorCodes::TokenInvalid); //设置返回的错误码, 表示Token无效
		return Status::OK;
	}

	reply->set_error(ErrorCodes::Success); //设置返回的错误码, 表示成功
	reply->set_uid(uid); //设置返回的用户id
	reply->set_token(token); //设置返回的Token
	return Status::OK;
}

void StatusServiceImpl::insertToken(int uid, std::string token)
{
	std::lock_guard<std::mutex> guard(_token_mtx); //加锁保护用户token列表的访问
	_tokens[uid] = token; //将用户id和生成的token插入到用户token列表中
}
