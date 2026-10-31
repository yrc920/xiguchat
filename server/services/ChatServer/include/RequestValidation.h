#pragma once

#include "const.h"
#include <json/json.h>

//在业务分发前统一检查请求字段和会话身份，只有校验通过才交给对应的handler处理
//这里校验登录请求的字段格式，token是否有效仍由LoginHandler查询Redis后判断
//uid/fromuid在不同消息中含义不同：搜索uid和touid是目标用户，不能用来判断操作者身份
inline ErrorCodes ValidateChatRequest(short id, const Json::Value& root, int session_uid)
{
    if (!root.isObject()) return Error_Json; //请求体必须是JSON对象
    //先检查是否能表示为int，再读取数值，避免数组、对象或超出int范围的数字触发转换异常
    auto positiveId = [&root](const char* key) {
        return root[key].isInt() && root[key].asInt() > 0;
    };
    auto stringField = [&root](const char* key) { return root[key].isString(); }; //检查字符串字段的类型
    //一条连接只绑定一个用户，已经登录的连接不能再次登录，否则原用户映射可能无法正确清理
    if (id == MSG_CHAT_LOGIN) {
        if (session_uid != 0) return TokenInvalid;
        return positiveId("uid") && stringField("token") && !root["token"].asString().empty()
            ? Success : Error_Json;
    }
    if (session_uid <= 0) return TokenInvalid; //除登录外，其余请求都需要已认证的会话
    //搜索内容既可以是数字uid，也可以是用户名，按字符串接收；被搜索的人不必是当前登录用户
    if (id == ID_SEARCH_USER_REQ) {
        return stringField("uid") && !root["uid"].asString().empty() ? Success : Error_Json;
    }
    //好友申请用uid表示申请人，同意好友、文本消息和心跳用fromuid表示操作者
    //请求中声明的操作者必须与服务端绑定的用户一致，不能仅凭客户端传来的uid执行操作
    const char* actor = id == ID_ADD_FRIEND_REQ ? "uid" : "fromuid";
    if (!positiveId(actor)) return Error_Json;
    if (root[actor].asInt() != session_uid) return UidInvalid;
    if (id == ID_HEART_BEAT_REQ) return Success; //心跳没有目标用户，只需校验发送者
    //剩余请求都指向其他用户，这里只校验uid格式与非自身约束，不负责判断目标是否存在或是否为好友
    if (!positiveId("touid") || root["touid"].asInt() == session_uid) return UidInvalid;
    //申请说明和备注允许为空，但必须是字符串，确保handler可以安全调用asString()
    if (id == ID_ADD_FRIEND_REQ) {
        return stringField("applyname") && stringField("bakname") ? Success : Error_Json;
    }
    if (id == ID_AUTH_FRIEND_REQ) return stringField("back") ? Success : Error_Json;
    //文本消息按数组传入，每一项都要有字符串类型的内容和消息标识，避免处理到一半才发现字段错误
    if (id == ID_TEXT_CHAT_MSG_REQ) {
        const auto& texts = root["text_array"];
        if (!texts.isArray() || texts.empty()) return Error_Json;
        for (const auto& text : texts) {
            if (!text.isObject() || !text["content"].isString() || !text["msgid"].isString())
                return Error_Json;
        }
        return Success;
    }
    return Error_Json; //不接受未定义的请求格式
}
