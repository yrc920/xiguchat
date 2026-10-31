#include "StatusServiceImpl.h"
#include "ConfigMgr.h"
#include "const.h"
#include "RedisMgr.h"
#include <charconv>

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
    const auto server = getChatServer(); //根据Redis中的连接数选择候选实例
    //没有候选实例时不能向客户端下发空地址；非法uid也不能用于生成用户token键
    if (request->uid() <= 0 || server.name.empty()) {
        reply->set_error(ErrorCodes::RPCFailed);
        return Status::OK;
    }
    const auto token = generate_unique_string(); //本次登录签发的token
    //ChatServer会从Redis读取token进行比对，必须先写入成功，才能向网关返回登录凭据
    if (!insertToken(request->uid(), token)) {
        reply->set_error(ErrorCodes::RPCFailed);
        return Status::OK;
    }
    //Status::OK只表示RPC已正常处理，具体业务结果通过reply中的error字段表达
    reply->set_host(server.host);
    reply->set_port(server.port);
    reply->set_token(token);
    reply->set_error(ErrorCodes::Success);
    return Status::OK;
}

ChatServer StatusServiceImpl::getChatServer()
{
    std::lock_guard<std::mutex> guard(_server_mtx);
    ChatServer selected; //name为空表示尚未找到符合条件的实例，也能处理空配置列表
    for (const auto& [name, server] : _servers) {
        auto count = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, name);
        int value = 0;
        //from_chars不会抛异常；除了检查错误码，还要确认整个字符串都已消费，拒绝数字后夹杂其他内容
        auto [end, error] = std::from_chars(count.data(), count.data() + count.size(), value);
        //无计数、负计数、损坏计数或地址不完整的实例都不能参与本次选路
        if (error != std::errc{} || end != count.data() + count.size() || value < 0 ||
            server.host.empty() || server.port.empty()) continue;
        //第一个有效实例作为初始候选，之后只在连接数更小时替换；全部跳过时仍保留空候选
        if (selected.name.empty() || value < selected.con_count) {
            selected = server;
            selected.con_count = value;
        }
    }
    //这里只比较已上报的计数，不做实时探活；实例异常退出留下的计数仍可能被选中
    return selected;
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

bool StatusServiceImpl::insertToken(int uid, std::string token)
{
	std::string uid_str = std::to_string(uid); //将用户id转换为字符串
	std::string token_key = USERTOKENPREFIX + uid_str;
	return RedisMgr::GetInstance()->Set(token_key, token); //返回写入结果，由调用方决定是否下发token
}
