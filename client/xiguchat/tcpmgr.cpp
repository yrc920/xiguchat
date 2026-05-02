#include "tcpmgr.h"
#include <QAbstractSocket>
#include <QJsonDocument>
#include "usermgr.h"

TcpMgr::TcpMgr() : _host(""), _port(0), _b_recv_pending(false), _message_id(0), _message_len(0)
{
    //处理连接成功, 相当于异步等待连接成功的事件
    QObject::connect(&_socket, &QTcpSocket::connected, [&]() {
        qDebug() << "Connected to server!";
        emit sig_con_success(true); //连接成功后发出信号通知界面更新状态
    });

    QObject::connect(&_socket, &QTcpSocket::readyRead, [&]() {
        //当有数据可读时，读取所有数据并追加到缓冲区
        _buffer.append(_socket.readAll());

        QDataStream stream(&_buffer, QIODevice::ReadOnly); //创建一个QDataStream对象来读取缓冲区中的数据
        stream.setVersion(QDataStream::Qt_5_0); //设置数据流的版本，以确保数据格式的一致性

        forever {
            //先解析头部
            if(!_b_recv_pending){
                //检查缓冲区中的数据是否足够解析出一个消息头(消息ID + 消息长度)
                if (_buffer.size() < static_cast<int>(sizeof(quint16) * 2))
                    return; //数据不够，等待更多数据

                stream >> _message_id >> _message_len; //预读取消息ID和消息长度，但不从缓冲区中移除
                _buffer = _buffer.mid(sizeof(quint16) * 2); //将buffer中的前四个字节移除
                qDebug() << "Message ID:" << _message_id << ", Length:" << _message_len;
            }

            //buffer剩余长读是否满足消息体长度，不满足则退出继续等待接受
            if(_buffer.size() < _message_len){
                _b_recv_pending = true; //设置标志，表示正在等待接收完整消息
                return;
            }

            _b_recv_pending = false; //重置标志，准备接收下一条消息
            //读取消息体
            QByteArray messageBody = _buffer.mid(0, _message_len);
            qDebug() << "receive body msg is " << messageBody ;

            _buffer = _buffer.mid(_message_len); //将buffer中的前_message_len个字节移除，为下一条消息做准备
            //调用消息处理函数，根据消息ID分发到对应的处理函数
            handleMsg(ReqId(_message_id), _message_len, messageBody);
        }

    });

    //处理连接错误
    QObject::connect(&_socket,
        QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            [&](QAbstractSocket::SocketError socketError) {
                Q_UNUSED(socketError)
                qDebug() << "Error:" << _socket.errorString();
                });

    //处理连接断开
    QObject::connect(&_socket, &QTcpSocket::disconnected, [&]() {
        qDebug() << "Disconnected from server.";
        });

    //连接发送信号用来发送数据
    QObject::connect(this, &TcpMgr::sig_send_data, this, &TcpMgr::slot_send_data);

    initHandlers(); //注册消息
}

TcpMgr::~TcpMgr(){

}

void TcpMgr::initHandlers()
{
    //shared_from_this()只能在对象已经完全构建之后调用
    //auto self = shared_from_this();
    _handlers.insert(ID_CHAT_LOGIN_RSP, [this](ReqId id, int len, QByteArray data){
        Q_UNUSED(len); //表示这个参数在函数体内未使用，避免编译器警告
        qDebug() << "handle id is " << id << " data is " << data;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data); //将QByteArray转换为QJsonDocument

        //检查转换是否成功
        if(jsonDoc.isNull()){
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object(); //从QJsonDocument中提取QJsonObject

        //如果JSON对象中不包含"error"字段，说明服务器返回的数据格式不正确
        if(!jsonObj.contains("error")){
            int err = ErrorCodes::ERR_JSON; //定义一个错误码表示JSON解析错误
            qDebug() << "Login Failed, err is Json Parse Err" << err ;
            emit sig_login_failed(err); //发出登录失败的信号，参数为错误码
            return;
        }

        int err = jsonObj["error"].toInt();
        //如果服务器返回的错误码不为0，说明登录失败
        if(err != ErrorCodes::SUCCESS){
            qDebug() << "Login Failed, err is " << err ;
            emit sig_login_failed(err); //发出登录失败的信号，参数为错误码
            return;
        }

        //登录成功，保存用户信息
        UserMgr::GetInstance()->SetUid(jsonObj["uid"].toInt());
        UserMgr::GetInstance()->SetName(jsonObj["name"].toString());
        UserMgr::GetInstance()->SetToken(jsonObj["token"].toString());
        emit sig_swich_chatdlg(); //发出切换到聊天界面的信号，进入聊天界面
    });
}

void TcpMgr::handleMsg(ReqId id, int len, QByteArray data)
{
    auto find_iter =  _handlers.find(id); //在_handlers中查找对应消息ID的处理函数
    //如果没有找到对应的处理函数，输出调试信息并返回
    if(find_iter == _handlers.end()){
        qDebug() << "not found id [" << id << "] to handle";
        return;
    }

    find_iter.value()(id, len, data); //调用找到的处理函数，传入消息ID、消息长度和消息数据
}

void TcpMgr::slot_tcp_connect(ServerInfo si)
{
    qDebug()<< "receive tcp connect signal";
    //尝试连接到服务器
    qDebug() << "Connecting to server...";
    _host = si.Host; //服务器地址
    _port = static_cast<uint16_t>(si.Port.toUInt()); //服务器端口
    _socket.connectToHost(si.Host, _port); //连接到服务器(异步)
}

void TcpMgr::slot_send_data(ReqId reqId, QString data)
{
    uint16_t id = reqId; //将ReqId转换为uint16_t类型的消息ID
    QByteArray dataBytes = data.toUtf8(); //将字符串转换为UTF-8编码的字节数组
    quint16 len = static_cast<quint16>(data.size()); //计算长度(使用网络字节序转换)

    //创建一个QByteArray用于存储要发送的所有数据
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);

    //设置数据流使用网络字节序
    out.setByteOrder(QDataStream::BigEndian);
    out << id << len; //写入ID和长度
    block.append(dataBytes); //添加字符串数据
    _socket.write(block); //发送数据
}
