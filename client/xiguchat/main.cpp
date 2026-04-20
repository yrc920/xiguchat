#include "mainwindow.h"

#include <QApplication>
#include <QFile>

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


    MainWindow w;
    w.show();
    return a.exec();
}
