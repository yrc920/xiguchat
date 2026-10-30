#ifndef PAGEBRIDGE_H
#define PAGEBRIDGE_H

#include <QObject>
#include <QString>

#include "shellbridge.h"

/**
 * @brief 页面桥接对象的基类
 *
 * 只定义一个导航契约：QML 里调 `bridge.go("register")`，交给应用外壳换页。
 * 页面不认识其他页面，也不持有导航状态。
 */
class PageBridge : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

    Q_INVOKABLE void go(const QString &target)
    {
        if (navigationBlocked())
            return;
        if (auto *shell = ShellBridge::instance())
            shell->go(target);
    }
protected:
    virtual bool navigationBlocked() const { return false; }
};

#endif // PAGEBRIDGE_H
