#include "findsuccessdlg.h"
#include "ui_findsuccessdlg.h"

#include <QDir>
#include "applyfriend.h"

FindSuccessDlg::FindSuccessDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FindSuccessDlg),_parent(parent)
{
    ui->setupUi(this);
    setWindowTitle("添加"); //设置对话框标题
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint); //隐藏对话框标题栏
    this->setObjectName("FindSuccessDlg");

    QString app_path = QCoreApplication::applicationDirPath(); //获取当前应用程序的路径
    QString pix_path = QDir::toNativeSeparators(app_path +
        QDir::separator() + "static" + QDir::separator() + "head_1.jpg"); //构建图片的完整路径
    QPixmap head_pix(pix_path); //加载图片
    //缩放图片到label的大小，保持宽高比，并使用平滑变换以获得更好的图像质量
    head_pix = head_pix.scaled(ui->head_lb->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->head_lb->setPixmap(head_pix); //将缩放后的图片设置到QLabel上
    ui->add_friend_btn->SetState("normal", "hover", "press"); //设置添加好友按钮的状态样式
    this->setModal(true); //设置对话框为模态, 阻止用户与其他窗口交互, 直到对话框被关闭
}

FindSuccessDlg::~FindSuccessDlg()
{
    delete ui;
}

void FindSuccessDlg::SetSearchInfo(std::shared_ptr<SearchInfo> si)
{
    ui->name_lb->setText(si->_name); //设置用户昵称
    _si = si; //保存搜索信息到成员变量
}

void FindSuccessDlg::on_add_friend_btn_clicked()
{
    this->hide(); //隐藏当前的搜索成功对话框
    //弹出加好友界面
    auto applyFriend = new ApplyFriend(_parent);
    applyFriend->SetSearchInfo(_si); //将搜索信息传递给加好友界面, 以便在加好友界面中显示用户信息
    applyFriend->setModal(true); //设置加好友界面为模态, 阻止用户与其他窗口交互, 直到加好友界面被关闭
    applyFriend->show(); //显示加好友界面
}
