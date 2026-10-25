#include "StatusServiceImpl.h"
#include "ConfigMgr.h"
#include "const.h"
#include "RedisMgr.h"

StatusServiceImpl::StatusServiceImpl()
{
	auto& cfg = ConfigMgr::Inst(); //获取配置管理器实例
	//从配置文件中读取聊天服务器的信息, 初始化服务器列表
	auto server_list = cfg["chatservers"]["Name"];

	std::vector<std::string> words; //用于存储分割后的服务器名称列表
	std::stringstream ss(server_list); //使用字符串流将服务器列表字符串分割成单个服务器名称
	std::string word; //临时变量用于存储分割出的服务器名称

	//将服务器列表字符串按照逗号分割成单个服务器名称, 并将其存储到words向量中
	while (std::getline(ss, word, ',')) {
		words.push_back(word);
	}

	for (auto& word : words) {
		//如果服务器名称为空, 则跳过该服务器
		if (cfg[word]["Name"].empty()) {
			continue;
		}

		ChatServer server; //为每个服务器名称创建一个ChatServer对象
		server.port = cfg[word]["Port"];
		server.host = cfg[word]["Host"];
		server.name = cfg[word]["Name"];
		_servers[server.name] = server; //将服务器信息存储到服务器列表中
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

	//从Redis中获取第一个服务器的连接数
	auto count_str = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, minServer.name);
	if (count_str.empty()) {
		//不存在则默认设置为最大
		minServer.con_count = INT_MAX;
	}
	else {
		minServer.con_count = std::stoi(count_str); //将连接数字符串转换为整数
	}

	for (auto& server : _servers)
	{
		//如果当前服务器的名称与假设的最小连接数服务器的名称相同, 则跳过该服务器
		if (server.second.name == minServer.name) {
			continue;
		}

		//从Redis中获取当前服务器的连接数
		auto count_str = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, server.second.name);
		if (count_str.empty()) {
			//不存在则默认设置为最大
			server.second.con_count = INT_MAX;
		}
		else {
			server.second.con_count = std::stoi(count_str); //将连接数字符串转换为整数
		}

		//如果当前服务器的连接数小于假设的最小连接数服务器的连接数, 则更新最小连接数服务器为当前服务器
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
	//从Redis中获取用户id对应的Token值
	bool success = RedisMgr::GetInstance()->Get(token_key, token_value);

	//如果获取失败, 则说明用户id无效, 返回uid无效的错误码
	if (!success) {
		reply->set_error(ErrorCodes::UidInvalid);
		return Status::OK;
	}
	//如果Token值与请求的Token不匹配, 则说明Token无效, 返回Token无效的错误码
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
	std::string uid_str = std::to_string(uid); //将用户id转换为字符串
	std::string token_key = USERTOKENPREFIX + uid_str;
	RedisMgr::GetInstance()->Set(token_key, token); //将用户id和Token存储到Redis中
}
