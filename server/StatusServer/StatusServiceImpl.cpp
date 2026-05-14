#include "StatusServiceImpl.h"
#include "ConfigMgr.h"
#include "const.h"
#include "RedisMgr.h"

StatusServiceImpl::StatusServiceImpl()
{
	auto& cfg = ConfigMgr::Inst(); //获取配置管理器实例
	//从配置文件中读取聊天服务器的信息, 初始化服务器列表
	auto server_list = cfg["chatservers"]["Name"];

	std::vector<std::string> words;

	std::stringstream ss(server_list);
	std::string word;

	while (std::getline(ss, word, ',')) {
		words.push_back(word);
	}

	for (auto& word : words) {
		if (cfg[word]["Name"].empty()) {
			continue;
		}

		ChatServer server;
		server.port = cfg[word]["Port"];
		server.host = cfg[word]["Host"];
		server.name = cfg[word]["Name"];
		_servers[server.name] = server;
	}
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
	auto count_str = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, minServer.name);
	if (count_str.empty()) {
		//不存在则默认设置为最大
		minServer.con_count = INT_MAX;
	}
	else {
		minServer.con_count = std::stoi(count_str);
	}


	// 使用范围基于for循环
	for (auto& server : _servers) {

		if (server.second.name == minServer.name) {
			continue;
		}

		auto count_str = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, server.second.name);
		if (count_str.empty()) {
			server.second.con_count = INT_MAX;
		}
		else {
			server.second.con_count = std::stoi(count_str);
		}

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

	std::string uid_str = std::to_string(uid);
	std::string token_key = USERTOKENPREFIX + uid_str;
	std::string token_value = "";
	bool success = RedisMgr::GetInstance()->Get(token_key, token_value);
	if (!success) {
		reply->set_error(ErrorCodes::UidInvalid);
		return Status::OK;
	}

	if (token_value != token) {
		reply->set_error(ErrorCodes::TokenInvalid);
		return Status::OK;
	}

	reply->set_error(ErrorCodes::Success); //设置返回的错误码, 表示成功
	reply->set_uid(uid); //设置返回的用户id
	reply->set_token(token); //设置返回的Token
	return Status::OK;
}

void StatusServiceImpl::insertToken(int uid, std::string token)
{
	std::string uid_str = std::to_string(uid);
	std::string token_key = USERTOKENPREFIX + uid_str;
	RedisMgr::GetInstance()->Set(token_key, token);
}
