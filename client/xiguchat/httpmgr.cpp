#include "httpmgr.h"

HttpMgr::HttpMgr()
{
    //将http请求完成的信号连接到对应的槽函数,槽函数再根据模块分发到对应的模块处理(解耦合)
    connect(this, &HttpMgr::sig_http_finish, this, &HttpMgr::slot_http_finished);
}

HttpMgr::~HttpMgr()
{

}

void HttpMgr::PostHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod)
{
    QByteArray data = QJsonDocument(json).toJson(); //将json对象转换为字节数组
    //设置请求头，指定内容类型为json，并设置内容长度
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(data.length()));

    auto self = shared_from_this(); //使引用计数增加，确保lambda表达式中可以安全地访问this指针
    QNetworkReply* reply = _manager.post(request, data); //发送post请求
    //接受http回包,把信息通过信号发送出去
    QObject::connect(reply, &QNetworkReply::finished, [self, reply, req_id, mod](){
        //处理错误情况(防御式编程)
        if(reply->error() != QNetworkReply::NoError){
            qDebug() << reply->errorString();
            //发送信号通知完成
            emit self->sig_http_finish(req_id, "", ErrorCodes::ERR_NETWORK, mod);
            reply->deleteLater(); //释放reply对象的内存
            return;
        }

        //无错误
        QString res = reply->readAll();
        //发送信号通知完成
        emit self->sig_http_finish(req_id, res, ErrorCodes::SUCCESS, mod);
        reply->deleteLater(); //释放reply对象的内存
        return;
    });
}

void HttpMgr::slot_http_finished(ReqId id, QString res, ErrorCodes err, Modules mod)
{
    //如果是注册模块
    if(mod == Modules::REGISTERMOD){
        //发送信号通知指定模块http的响应结束了
        emit sig_reg_mod_finish(id, res, err);
    }

     //如果是重置密码模块
    else if(mod == Modules::RESETMOD){
        //发送信号通知指定模块http的响应结束了
        emit sig_reset_mod_finish(id, res, err);
    }
}
