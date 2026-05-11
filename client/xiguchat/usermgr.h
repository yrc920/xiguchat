#ifndef USERMGR_H
#define USERMGR_H

#include <QObject>
#include <memory>
#include <singleton.h>
/******************************************************************************
 *
 * @file       usermgr.h
 * @brief      用户管理类，负责保存用户信息
 *
 * @author     西故
 * @date       2026/05/03
 * @history
 *****************************************************************************/

class UserMgr : public QObject,public Singleton<UserMgr>,
    public std::enable_shared_from_this<UserMgr>
{
    Q_OBJECT //使得UserMgr可以使用Qt的信号和槽机制

public:
    friend class Singleton<UserMgr>; //使基类创建实例时可以访问私有构造函数
    ~UserMgr(); //使基类析构时可以访问派生类的析构函数
    void SetName(QString name); //设置用户名
    void SetUid(int uid); //设置用户ID
    void SetToken(QString token); //设置用户令牌
    int GetUid(); //获取用户ID
    QString GetName(); //获取用户名

private:
    UserMgr();
    int _uid; //用户ID
    QString _name; //用户名
    QString _token; //用户令牌
};

#endif // USERMGR_H
