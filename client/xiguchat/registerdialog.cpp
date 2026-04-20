#include "registerdialog.h"
#include "ui_registerdialog.h"
#include "httpmgr.h"

RegisterDialog::RegisterDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::RegisterDialog)
{
    ui->setupUi(this);
    //设置密码输入框的显示模式为密码模式
    ui->pass_edit->setEchoMode(QLineEdit::Password);
    ui->confirm_edit->setEchoMode(QLineEdit::Password);
    ui->err_tip->setProperty("state","normal"); //设置初始状态为正常(告诉qss有这个状态)
    repolish(ui->err_tip); //刷新样式表，使得状态生效

    //连接http管理者的注册模块完成信号到对应的槽函数
    connect(HttpMgr::GetInstance().get(), &HttpMgr::sig_reg_mod_finish,
            this, &RegisterDialog::slot_reg_mod_finish);

    initHttpHandlers(); //初始化http响应处理函数,将请求id和对应的处理函数的映射表插入到_handlers中
}

RegisterDialog::~RegisterDialog()
{
    delete ui;
}

void RegisterDialog::on_get_code_clicked()
{
    auto email = ui->email_edit->text(); //获取邮箱输入框的文本
    //不要创建临时的QRegularExpression对象(邮箱正则表达式)
    static QRegularExpression regex(R"((\w+)(\.|_)?(\w*)@(\W+)(\.(\w+))+)");
    bool match = regex.match(email).hasMatch(); //使用正则表达式验证邮箱格式是否正确
    if(match){
        //发送http验证码
    }else{
        showTip(tr("邮箱地址不正确"), false);
    }
}

void RegisterDialog::slot_reg_mod_finish(ReqId id, QString res, ErrorCodes err)
{
    if(err != ErrorCodes::SUCCESS){
        showTip(tr("网络请求错误"), false);
        return;
    }

    //解析JSON字符串,res转化为QByteArray
    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());
    //转换过程中如果失败
    if(jsonDoc.isNull()){
        showTip(tr("json解析失败"), false);
        return;
    }
    //json解析错误
    if(!jsonDoc.isObject()){
        showTip(tr("json解析失败"), false);
        return;
    }

    //解析成功,根据请求id找到对应的处理函数进行处理
    _handlers[id](jsonDoc.object());
    return;
}

void RegisterDialog::initHttpHandlers()
{
    //注册获取验证码回包的逻辑
    _handlers.insert(ReqId::ID_GET_VARIFY_CODE, [this](const QJsonObject& jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            showTip(tr("参数错误"), false);
            return;
        }

        auto email = jsonObj["email"].toString();
        showTip(tr("验证码已经发送到邮箱,注意查收"), true);
        qDebug() << "email is " << email;
    });
}

void RegisterDialog::showTip(QString str, bool b_ok)
{
    if(b_ok){
        ui->err_tip->setProperty("state","normal"); //设置状态为正常
    }else{
        ui->err_tip->setProperty("state","err"); //设置状态为错误
    }
    ui->err_tip->setText(str); //设置提示信息
    repolish(ui->err_tip); //刷新样式表，使得状态生效
}
