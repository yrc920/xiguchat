#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <QString>
#include "singleton.h"

// 文本消息的分帧、发送与本地记录；不依赖聊天页面的生命周期。
class ChatService : public Singleton<ChatService>
{
public:
    friend class Singleton<ChatService>;
    bool sendText(int uid, const QString &text);

private:
    ChatService() = default;
};

#endif // CHATSERVICE_H
