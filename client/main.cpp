#include <QDir>
#include <QFont>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QSettings>
#include <QUrl>

#include "global.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("xiguchat"));
    QGuiApplication::setOrganizationName(QStringLiteral("XiguChat"));
    QFont font = app.font();
#if defined(Q_OS_WIN)
    font.setFamilies({QStringLiteral("Microsoft YaHei UI"), QStringLiteral("Segoe UI")});
#elif defined(Q_OS_MACOS)
    font.setFamilies({QStringLiteral("PingFang SC"), QStringLiteral("Helvetica Neue")});
#else
    font.setFamilies({QStringLiteral("Noto Sans CJK SC"), QStringLiteral("Noto Sans")});
#endif
    app.setFont(font);

    // 统一设置应用窗口图标（三端通用，从 qrc 资源加载）：
    // Windows/Linux 显示在窗口标题栏与任务栏；
    // macOS 窗口标题栏不显示图标，Dock 图标由 .app bundle 内的 .icns 提供（见 CMakeLists APPLE 分支）
    QGuiApplication::setWindowIcon(QIcon(QStringLiteral(":/res/xiguchat.png")));

    // config.ini 与可执行文件同目录（构建后由 CMake 拷到产物目录）
    const QString config_path = QDir(QCoreApplication::applicationDirPath())
                                    .filePath(QStringLiteral("config.ini"));
    QSettings settings(config_path, QSettings::IniFormat);
    gate_url_prefix = QStringLiteral("http://")
                      + settings.value(QStringLiteral("GateServer/host")).toString()
                      + QLatin1Char(':')
                      + settings.value(QStringLiteral("GateServer/port")).toString();

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/XiguChat/qml/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return 1;

    return app.exec();
}
