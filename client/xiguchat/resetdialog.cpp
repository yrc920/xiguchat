#include "resetdialog.h"
#include "ui_resetdialog.h"
#include "httpmgr.h"

ResetDialog::ResetDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ResetDialog)
{
    ui->setupUi(this);
    ui->pwd_edit->setEchoMode(QLineEdit::Password); //设置密码输入框的显示模式为密码模式
    ui->err_tip->clear(); //清空提示信息显示区域

    //连接http管理者的注册模块完成信号到对应的槽函数
    connect(HttpMgr::GetInstance().get(), &HttpMgr::sig_reset_mod_finish, this,
        &ResetDialog::slot_reset_mod_finish);

    initHandlers(); //初始化http响应处理函数, 将请求id和对应的处理函数的映射表插入到_handlers中

    //连接输入框的editingFinished信号到对应的槽函数,
    //当用户完成输入后, 检查输入是否合法, 如果不合法就显示对应的错误提示信息
    connect(ui->user_edit, &QLineEdit::editingFinished, this ,[this](){ checkUserValid(); });
    connect(ui->email_edit, &QLineEdit::editingFinished, this, [this](){ checkEmailValid(); });
    connect(ui->pwd_edit, &QLineEdit::editingFinished, this, [this](){ checkPassValid(); });
    connect(ui->verify_edit, &QLineEdit::editingFinished, this, [this](){ checkVerifyValid(); });

    //设置状态的样式名称
    ui->pass_visible->SetState("unvisible", "unvisible_hover", "", "visible", "visible_hover", "");
    //连接点击事件(发出点击信号之后, 已切换状态)
    connect(ui->pass_visible, &ClickedLabel::clicked, this, [this]() {
        auto state = ui->pass_visible->GetCurState(); //获取当前状态
        //如果当前状态是正常状态
        if(state == ClickLbState::Normal){
            ui->pwd_edit->setEchoMode(QLineEdit::Password); //设置密码输入框的显示模式为密码模式
        }else{
            ui->pwd_edit->setEchoMode(QLineEdit::Normal); //设置密码输入框的显示模式为正常模式
        }
        qDebug() << "Label was clicked!";
    });
}

ResetDialog::~ResetDialog()
{
    delete ui;
}

void ResetDialog::on_return_btn_clicked()
{
    qDebug() << "sure btn clicked ";
    emit switchLogin(); //发出切换到登录界面的信号，返回登录界面
}

void ResetDialog::slot_reset_mod_finish(ReqId id, QString res, ErrorCodes err)
{
    if(err != ErrorCodes::SUCCESS){
        showTip(tr("网络请求错误"),false);
        return;
    }

    //解析JSON字符串, res需转化为QByteArray
    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());
    //转换过程中如果失败
    if(jsonDoc.isNull()){
        showTip(tr("json解析错误"),false);
        return;
    }

    //json解析错误
    if(!jsonDoc.isObject()){
        showTip(tr("json解析错误"),false);
        return;
    }

    //调用对应的逻辑,根据id回调
    _handlers[id](jsonDoc.object());
    return;
}

void ResetDialog::initHandlers()
{
    //注册获取验证码回包的逻辑(和注册模块使用一样的接口)
    _handlers.insert(ReqId::ID_GET_VERIFY_CODE, [this](QJsonObject jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            showTip(tr("参数错误"),false);
            return;
        }
        auto email = jsonObj["email"].toString();
        showTip(tr("验证码已发送到邮箱，注意查收"), true);
        qDebug()<< "email is " << email ;
    });

    //注册注册用户回包的逻辑
    _handlers.insert(ReqId::ID_RESET_PWD, [this](QJsonObject jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            showTip(tr("参数错误"),false);
            return;
        }
        auto email = jsonObj["email"].toString();
        showTip(tr("重置成功, 点击返回登录"), true);
        qDebug() << "email is " << email ;
        qDebug() << "user uid is " <<  jsonObj["uid"].toString();
    });
}

void ResetDialog::showTip(QString str, bool b_ok)
{
    if(b_ok){
        ui->err_tip->setProperty("state", "normal"); //设置状态为正常
    }else{
        ui->err_tip->setProperty("state", "err"); //设置状态为错误
    }
    ui->err_tip->setText(str); //设置提示信息
    repolish(ui->err_tip); //刷新样式表，使得状态生效
}

void ResetDialog::on_verify_btn_clicked()
{
    qDebug() << "receive verify btn clicked ";
    auto email = ui->email_edit->text(); //获取邮箱输入框的文本
    auto bcheck = checkEmailValid(); //检查邮箱是否合法
    //如果邮箱不合法，就直接返回，不发送http请求获取验证码
    if(!bcheck)
        return;

    //发送http请求获取验证码
    QJsonObject json_obj;
    json_obj["email"] = email; //将邮箱地址放入json对象中
    HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix + "/get_verifycode"),
        json_obj, ReqId::ID_GET_VERIFY_CODE, Modules::RESETMOD);
}

void ResetDialog::on_sure_btn_clicked()
{
    //检查输入是否合法，如果不合法就直接返回，不发送http请求重置密码
    bool valid = checkUserValid();
    if(!valid)
        return;

    valid = checkEmailValid();
    if(!valid)
        return;

    valid = checkPassValid();
    if(!valid)
        return;

    valid = checkVerifyValid();
    if(!valid)
        return;

    //发送http重置用户请求
    QJsonObject json_obj;
    json_obj["user"] = ui->user_edit->text();
    json_obj["email"] = ui->email_edit->text();
    json_obj["passwd"] = xorString(ui->pwd_edit->text()); //对密码进行异或加密
    json_obj["verifycode"] = ui->verify_edit->text();

    HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix + "/reset_pwd"),
        json_obj, ReqId::ID_RESET_PWD, Modules::RESETMOD);
}

void ResetDialog::AddTipErr(TipErr te, QString tips)
{
    _tip_errs[te] = tips; //将错误类型和对应的提示信息插入到_tip_errs中
    showTip(tips, false); //显示错误提示信息
}

void ResetDialog::DelTipErr(TipErr te)
{
    _tip_errs.remove(te); //从_tip_errs中删除指定错误类型的提示信息
    //如果_tip_errs中没有错误了，就清空提示信息显示区域
    if(_tip_errs.empty()){
        ui->err_tip->clear();
        return;
    }

    //如果还有错误提示，就显示第一个错误提示信息
    showTip(_tip_errs.first(), false);
}

bool ResetDialog::checkUserValid()
{
    if(ui->user_edit->text() == ""){
        AddTipErr(TipErr::TIP_USER_ERR, tr("用户名不能为空"));
        return false;
    }

    DelTipErr(TipErr::TIP_USER_ERR); //如果用户名合法，就从_tip_errs中删除用户名错误提示信息
    return true;
}

bool ResetDialog::checkPassValid()
{
    auto pass = ui->pwd_edit->text(); //获取密码输入框的文本

    if(pass.length() < 6 || pass.length() > 15){
        //提示长度不准确
        AddTipErr(TipErr::TIP_PWD_ERR, tr("密码长度应为6~15"));
        return false;
    }

    //密码长度至少6，可以是字母、数字和特定的特殊字符
    static QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*]{6,15}$");
    bool match = regExp.match(pass).hasMatch(); //执行正则表达式匹配，判断密码是否符合要求
    if(!match){
        //提示字符非法
        AddTipErr(TipErr::TIP_PWD_ERR, tr("不能包含非法字符"));
        return false;;
    }

    DelTipErr(TipErr::TIP_PWD_ERR); //如果密码合法，就从_tip_errs中删除密码错误提示信息
    return true;
}

bool ResetDialog::checkEmailValid()
{
    auto email = ui->email_edit->text(); //获取邮箱输入框的文本
    //不要创建临时的QRegularExpression对象(邮箱正则表达式)
    static QRegularExpression regex(R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)");
    bool match = regex.match(email).hasMatch(); //执行正则表达式匹配
    if(!match){
        //提示邮箱不正确
        AddTipErr(TipErr::TIP_EMAIL_ERR, tr("邮箱地址不正确"));
        return false;
    }

    DelTipErr(TipErr::TIP_EMAIL_ERR); //如果邮箱合法，就从_tip_errs中删除邮箱错误提示信息
    return true;
}

bool ResetDialog::checkVerifyValid()
{
    auto pass = ui->verify_edit->text(); //获取验证码输入框的文本
    if(pass.isEmpty()){
        AddTipErr(TipErr::TIP_VERIFY_ERR, tr("验证码不能为空"));
        return false;
    }

    DelTipErr(TipErr::TIP_VERIFY_ERR); //如果验证码合法，就从_tip_errs中删除验证码错误提示信息
    return true;
}

