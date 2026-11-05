#include "userdata.h"
#include <algorithm>

SearchInfo::SearchInfo(int uid, QString name,QString nick, QString desc, int sex, QString icon) :
    _uid(uid), _name(name), _nick(nick), _desc(desc), _sex(sex), _icon(icon){
}

AddFriendApply::AddFriendApply(int from_uid, QString name, QString desc, QString icon, QString nick, int sex)
    : _from_uid(from_uid), _name(name), _desc(desc), _icon(icon), _nick(nick), _sex(sex)
{

}

//按 msgid 去重后追加：同一条消息可能先以「发送中」进入列表，
//回执或历史同步再带服务端 message_id 回来，这里只做原地更新
void FriendInfo::AppendChatMsgs(const std::vector<std::shared_ptr<TextChatData> > text_vec)
{
    for (const auto &text : text_vec) {
        auto existing = std::find_if(_chat_thread->_chat_msgs.begin(), _chat_thread->_chat_msgs.end(), [&](const auto &item) {
            return item->_msg_id == text->_msg_id;
        });
        if (existing == _chat_thread->_chat_msgs.end()) {
            _chat_thread->_chat_msgs.push_back(text);
        } else if ((*existing)->_message_id == 0 || text->_message_id > 0) {
            (*existing)->_message_id = text->_message_id;
            (*existing)->_thread_id = text->_thread_id;
            (*existing)->_status = text->_status;
            (*existing)->_msg_content = text->_msg_content;
        }
    }
    //有服务端 ID 的消息按 ID 升序；尚未确认的排在最后，拿到回执后自然归位
    std::stable_sort(_chat_thread->_chat_msgs.begin(), _chat_thread->_chat_msgs.end(), [](const auto &left, const auto &right) {
        if (left->_message_id == 0 || right->_message_id == 0)
            return left->_message_id != 0 && right->_message_id == 0;
        return left->_message_id < right->_message_id;
    });
}
