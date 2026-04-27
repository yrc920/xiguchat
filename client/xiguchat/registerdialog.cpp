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

    //day11 设定输入框输入后清空字符串
    ui->err_tip->clear();

    connect(ui->user_edit,&QLineEdit::editingFinished,this,[this](){ checkUserValid(); });
    connect(ui->email_edit, &QLineEdit::editingFinished, this, [this](){ checkEmailValid(); });
    connect(ui->pass_edit, &QLineEdit::editingFinished, this, [this](){ checkPassValid(); });
    connect(ui->confirm_edit, &QLineEdit::editingFinished, this, [this](){ checkConfirmValid(); });
    connect(ui->verify_edit, &QLineEdit::editingFinished, this, [this](){ checkVerifyValid(); });
}

RegisterDialog::~RegisterDialog()
{
    delete ui;
}

void RegisterDialog::on_get_code_clicked()
{
    auto email = ui->email_edit->text(); //获取邮箱输入框的文本
    //不要创建临时的QRegularExpression对象(邮箱正则表达式)
    static QRegularExpression regex(R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)");
    bool match = regex.match(email).hasMatch(); //使用正则表达式验证邮箱格式是否正确
    if(match){
        //发送http请求获取验证码
        QJsonObject json_obj;
        json_obj["email"] = email; //将邮箱地址放入json对象中
        HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix + "/get_verifycode"),
            json_obj, ReqId::ID_GET_VERIFY_CODE, Modules::REGISTERMOD);
    }else{
        showTip(tr("邮箱地址不正确"), false);
    }
}

void RegisterDialog::on_sure_btn_clicked()
{
    bool valid = checkUserValid();
    if(!valid)
        return;

    valid = checkEmailValid();
    if(!valid)
        return;

    valid = checkPassValid();
    if(!valid)
        return;

    valid = checkConfirmValid();
    if(!valid)
        return;

    valid = checkVerifyValid();
    if(!valid)
        return;

    //发送http请求注册用户
    QJsonObject json_obj;
    json_obj["user"] = ui->user_edit->text();
    json_obj["email"] = ui->email_edit->text();
    json_obj["passwd"] = xorString(ui->pass_edit->text()); //对密码进行异或加密
    json_obj["confirm"] = xorString(ui->confirm_edit->text()); //对确认密码进行异或加密
    json_obj["verifycode"] = ui->verify_edit->text();

    HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix + "/user_register"),
        json_obj, ReqId::ID_REG_USER, Modules::REGISTERMOD);
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
    _handlers.insert(ReqId::ID_GET_VERIFY_CODE, [this](const QJsonObject& jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            showTip(tr("参数错误"), false);
            return;
        }

        auto email = jsonObj["email"].toString();
        showTip(tr("验证码已经发送到邮箱,注意查收"), true);
        qDebug() << "email is " << email;
    });

    //注册注册用户回包逻辑
    _handlers.insert(ReqId::ID_REG_USER, [this](QJsonObject jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            showTip(tr("参数错误"),false);
            return;
        }
        auto email = jsonObj["email"].toString();
        showTip(tr("用户注册成功"), true);
        qDebug() << "user uid is " << jsonObj["uid"].toInt();
        qDebug() << "email is " << email ;
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

void RegisterDialog::AddTipErr(TipErr te, QString tips)
{
    _tip_errs[te] = tips; //将错误类型和对应的提示信息插入到_tip_errs中
    showTip(tips, false); //显示错误提示信息
}

void RegisterDialog::DelTipErr(TipErr te)
{
    _tip_errs.remove(te);
    if(_tip_errs.empty()){
        ui->err_tip->clear();
        return;
    }

    showTip(_tip_errs.first(), false);
}

bool RegisterDialog::checkUserValid()
{
    if(ui->user_edit->text() == ""){
        AddTipErr(TipErr::TIP_USER_ERR, tr("用户名不能为空"));
        return false;
    }

    DelTipErr(TipErr::TIP_USER_ERR);
    return true;
}


bool RegisterDialog::checkPassValid()
{
    auto pass = ui->pass_edit->text();

    if(pass.length() < 6 || pass.length()>15){
        //提示长度不准确
        AddTipErr(TipErr::TIP_PWD_ERR, tr("密码长度应为6~15"));
        return false;
    }

    // 创建一个正则表达式对象，按照上述密码要求
    // 这个正则表达式解释：
    // ^[a-zA-Z0-9!@#$%^&*]{6,15}$ 密码长度至少6，可以是字母、数字和特定的特殊字符
    static QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*]{6,15}$");
    bool match = regExp.match(pass).hasMatch();
    if(!match){
        //提示字符非法
        AddTipErr(TipErr::TIP_PWD_ERR, tr("不能包含非法字符"));
        return false;;
    }

    DelTipErr(TipErr::TIP_PWD_ERR);

    return true;
}

bool RegisterDialog::checkConfirmValid()
{
    auto confirm = ui->confirm_edit->text();
    if(confirm != ui->pass_edit->text()){
        //提示密码和确认密码不匹配
        AddTipErr(TipErr::TIP_PWD_CONFIRM, tr("密码和确认密码不匹配"));
        return false;
    }

    DelTipErr(TipErr::TIP_PWD_CONFIRM);
    return true;
}

bool RegisterDialog::checkEmailValid()
{
    //验证邮箱的地址正则表达式
    auto email = ui->email_edit->text();
    // 邮箱地址的正则表达式
    static QRegularExpression regex(R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)");
    bool match = regex.match(email).hasMatch(); // 执行正则表达式匹配
    if(!match){
        //提示邮箱不正确
        AddTipErr(TipErr::TIP_EMAIL_ERR, tr("邮箱地址不正确"));
        return false;
    }

    DelTipErr(TipErr::TIP_EMAIL_ERR);
    return true;
}

bool RegisterDialog::checkVerifyValid()
{
    auto pass = ui->verify_edit->text();
    if(pass.isEmpty()){
        AddTipErr(TipErr::TIP_VARIFY_ERR, tr("验证码不能为空"));
        return false;
    }

    DelTipErr(TipErr::TIP_VARIFY_ERR);
    return true;
}
