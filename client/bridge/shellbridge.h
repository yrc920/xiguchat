#ifndef SHELLBRIDGE_H
#define SHELLBRIDGE_H

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

/**
 * @brief 应用外壳：唯一持有「当前显示哪一页」的地方
 *
 * 页面自身不保存导航状态，QML 里调 `go("register")`，这里改 page，
 * 根窗口的 Loader 随之换页并调整窗口尺寸。
 *
 * 掉线通知也在这里：服务端踢人、长连接断开时给出提示，
 * 用户确认后关闭会话并回到登录页。
 */
class ShellBridge : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString page READ page NOTIFY pageChanged)
    Q_PROPERTY(QString notice READ notice NOTIFY noticeChanged)

public:
    explicit ShellBridge(QObject *parent = nullptr);
    ~ShellBridge() override;

    // 根窗口里只有一个实例；页面桥接对象通过它导航
    static ShellBridge *instance();

    QString page() const { return _page; }
    QString notice() const { return _notice; }

    Q_INVOKABLE void go(const QString &target);
    Q_INVOKABLE void dismissNotice();

signals:
    void pageChanged();
    void noticeChanged();

private:
    void showOfflineNotice(const QString &message);

    QString _page = QStringLiteral("login");
    QString _notice;
};

#endif // SHELLBRIDGE_H
