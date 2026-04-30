#ifndef TCPMGR_H
#define TCPMGR_H

#include <QTcpSocket>
#include "singleton.h"
#include "global.h"
#include <QObject>
/******************************************************************************
 *
 * @file       tcpmgr.h
 * @brief      tcp管理者
 *
 * @author     西故
 * @date       2026/04/29
 * @history
 *****************************************************************************/

class TcpMgr : public QObject, public Singleton<TcpMgr>,
    public std::enable_shared_from_this<TcpMgr>
{
    Q_OBJECT //使得TcpMgr可以使用Qt的信号和槽机制

public:
    ~TcpMgr(); //使基类析构时可以访问派生类的析构函数

private:
    friend class Singleton<TcpMgr>; //使基类创建实例时可以访问私有构造函数
    TcpMgr();
    void initHandlers(); //初始化消息处理函数
    void handleMsg(ReqId id, int len, QByteArray data); //处理消息的函数

    QTcpSocket _socket; //TCP套接字对象
    QString _host; //服务器地址
    uint16_t _port; //服务器端口
    QByteArray _buffer; //接收数据的缓冲区
    bool _b_recv_pending; //是否正在等待接收完整消息的标志
    quint16 _message_id; //当前消息的ID
    quint16 _message_len; //当前消息的长度
    //消息ID到处理函数的映射
    QMap<ReqId, std::function<void(ReqId id, int len, QByteArray data)>> _handlers;

public slots:
    void slot_tcp_connect(ServerInfo); //连接服务器的槽函数
    void slot_send_data(ReqId reqId, QString data); //发送数据的槽函数

signals:
    void sig_con_success(bool bsuccess); //连接成功的信号
    void sig_send_data(ReqId reqId, QString data); //发送数据的信号
    void sig_swich_chatdlg(); //切换到聊天界面的信号
    void sig_login_failed(int); //登录失败的信号，参数为错误码
};

#endif // TCPMGR_H
