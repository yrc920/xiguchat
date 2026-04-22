#ifndef HTTPMGR_H
#define HTTPMGR_H

#include "singleton.h"
#include <QString>
#include <QUrl>
#include <QObject>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <QJsonDocument>
/******************************************************************************
 *
 * @file       httpmgr.h
 * @brief      http管理者
 *
 * @author     西故
 * @date       2026/04/20
 * @history
 *****************************************************************************/

//CRTP技术
class HttpMgr : public QObject, public Singleton<HttpMgr>,
                public std::enable_shared_from_this<HttpMgr>
{
    Q_OBJECT //使得HttpMgr可以使用Qt的信号和槽机制

public:
    ~HttpMgr(); //使基类析构时可以访问派生类的析构函数
    //发送http请求
    void PostHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod);

private:
    friend class Singleton<HttpMgr>; //使基类创建实例时可以访问私有构造函数
    HttpMgr();
    QNetworkAccessManager _manager; //网络访问管理器，用于发送http请求

private slots:
    //http请求完成的槽函数,根据模块分发到对应的模块处理(解耦合)
    void slot_http_finished(ReqId id, QString res, ErrorCodes err, Modules mod);

signals:
    //http请求完成的信号
    void sig_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod);
    //注册模块http请求完成的信号
    void sig_reg_mod_finish(ReqId id, QString res, ErrorCodes err);
};

#endif // HTTPMGR_H
