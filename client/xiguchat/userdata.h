#ifndef USERDATA_H
#define USERDATA_H

#include <QString>
/******************************************************************************
 *
 * @file       userdata.h
 * @brief      用户数据结构
 *
 * @author     西故
 * @date       2026/05/09
 * @history
 *****************************************************************************/

class SearchInfo {
public:
    SearchInfo(int uid, QString name, QString nick, QString desc, int sex);

    int _uid; //用户ID
    QString _name; //用户名
    QString _nick; //用户昵称
    QString _desc; //用户描述
    int _sex; //用户性别
};

#endif // USERDATA_H
