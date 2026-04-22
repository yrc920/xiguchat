#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include "global.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QFile qss(":style/stylesheet.qss"); //资源文件路径
    if(qss.open(QFile::ReadOnly)){
        qDebug("open success");
        QString style = QLatin1String(qss.readAll()); //读取样式表内容
        a.setStyleSheet(style); //应用样式表到整个应用程序
        qss.close();
    }else{
        qDebug("Open failed");
    }

    // 获取当前应用程序的路径
    QString app_path = QCoreApplication::applicationDirPath();
    QString fileName = "config.ini"; //配置文件名
    //完整的配置文件路径
    QString config_path = QDir::toNativeSeparators(app_path + QDir::separator() + fileName);

    QSettings settings(config_path, QSettings::IniFormat); //使用INI格式读取配置文件
    //从配置文件中读取网关服务器的主机地址和端口号
    QString gate_host = settings.value("GateServer/host").toString();
    QString gate_port = settings.value("GateServer/port").toString();
    //构建网关服务器的URL前缀
    gate_url_prefix = "http://" + gate_host + ":" + gate_port;

    MainWindow w;
    w.show();
    return a.exec();
}
