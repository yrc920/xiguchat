#include "logindialog.h"
#include "ui_logindialog.h"
#include <QPainter>
#include <QPainterPath>
#include "httpmgr.h"
#include "tcpmgr.h"

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    ui->pass_edit->setEchoMode(QLineEdit::Password); //设置密码输入框的显示模式为密码模式
    ui->err_tip->clear(); //清空提示信息显示区域
    initHead(); //初始化头像

    //点击注册按钮触发切换信号
    connect(ui->reg_btn, &QPushButton::clicked, this, &LoginDialog::switchRegister);
    //设置忘记密码标签的状态样式
    ui->forget_label->SetState("normal", "hover", "", "selected", "selected_hover", "");
    //连接忘记密码标签的点击信号到对应的槽函数
    connect(ui->forget_label, &ClickedLabel::clicked, this, &LoginDialog::slot_forget_pwd);

    initHttpHandlers(); //初始化http响应处理函数, 将请求id和对应的处理函数的映射表插入到_handlers中

    //连接http管理者的登录模块完成信号到对应的槽函数
    connect(HttpMgr::GetInstance().get(), &HttpMgr::sig_login_mod_finish, this,
        &LoginDialog::slot_login_mod_finish);

    //连接tcp连接请求的信号和槽函数
    connect(this, &LoginDialog::sig_connect_tcp,
        TcpMgr::GetInstance().get(), &TcpMgr::slot_tcp_connect);
    //连接tcp管理者发出的连接成功信号
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_con_success,
        this, &LoginDialog::slot_tcp_con_finish);
    //连接tcp管理者发出的登陆失败信号
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_login_failed,
        this, &LoginDialog::slot_login_failed);

    //设置状态的样式名称
    ui->pass_visible->SetState("unvisible", "unvisible_hover", "", "visible", "visible_hover", "");
    //连接点击事件(发出点击信号之后, 已切换状态)
    connect(ui->pass_visible, &ClickedLabel::clicked, this, [this]() {
        auto state = ui->pass_visible->GetCurState(); //获取当前状态
        //如果当前状态是正常状态
        if(state == ClickLbState::Normal){
            ui->pass_edit->setEchoMode(QLineEdit::Password); //设置密码输入框的显示模式为密码模式
        }else{
            ui->pass_edit->setEchoMode(QLineEdit::Normal); //设置密码输入框的显示模式为正常模式
        }
        qDebug() << "Label was clicked!";
    });
}

LoginDialog::~LoginDialog()
{
    qDebug() << "LoginDialog destruct";
    delete ui;
}

void LoginDialog::initHttpHandlers()
{
    //注册获取登录回包逻辑
    _handlers.insert(ReqId::ID_LOGIN_USER, [this](QJsonObject jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            showTip(tr("参数错误"),false);
            enableBtn(true);
            return;
        }
        auto email = jsonObj["email"].toString();

        //发送信号通知tcpMgr发送长链接
        ServerInfo si;
        si.Uid = jsonObj["uid"].toInt();
        si.Host = jsonObj["host"].toString();
        si.Port = jsonObj["port"].toString();
        si.Token = jsonObj["token"].toString();

        _uid = si.Uid; //保存用户ID
        _token = si.Token; //保存用户令牌
        qDebug() << "email is " << email << " uid is " << si.Uid << " host is "
            << si.Host << " Port is " << si.Port << " Token is " << si.Token;
        emit sig_connect_tcp(si); //发出连接聊天服务器的信号，参数是服务器信息结构体
    });
}

void LoginDialog::on_login_btn_clicked()
{
    //检查输入是否合法, 如果不合法就直接返回，不发送http请求登录
    qDebug() << "login btn clicked";
    if(checkEmailValid() == false)
        return;

    if(checkPwdValid() == false)
        return;

    enableBtn(false); //登录过程中禁用登录和注册按钮，防止重复点击

    //发送http请求登录
    QJsonObject json_obj;
    json_obj["email"] = ui->email_edit->text();
    json_obj["passwd"] = xorString(ui->pass_edit->text()); //对密码进行异或加密

    HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix + "/user_login"),
        json_obj, ReqId::ID_LOGIN_USER, Modules::LOGINMOD);
}

void LoginDialog::slot_login_mod_finish(ReqId id, QString res, ErrorCodes err)
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

    //解析成功,根据请求id找到对应的处理函数进行处理
    _handlers[id](jsonDoc.object());
    return;
}

void LoginDialog::slot_forget_pwd()
{
    qDebug() << "slot forget pwd";
    emit switchReset(); //发出切换到重置密码界面的信号
}

void LoginDialog::initHead()
{
    QPixmap originalPixmap(":/res/head_1.jpg"); //加载图片
    //设置图片自动缩放
    qDebug() << originalPixmap.size() << ui->head_label->size();
    originalPixmap = originalPixmap.scaled(ui->head_label->size(),
        Qt::KeepAspectRatio, Qt::SmoothTransformation);

    //创建一个和原始图片相同大小的QPixmap，用于绘制圆角图片
    QPixmap roundedPixmap(originalPixmap.size());
    roundedPixmap.fill(Qt::transparent); // 用透明色填充

    QPainter painter(&roundedPixmap);
    painter.setRenderHint(QPainter::Antialiasing); //设置抗锯齿，使圆角更平滑
    painter.setRenderHint(QPainter::SmoothPixmapTransform); //设置平滑变换，使缩放后的图片更清晰

    //使用QPainterPath设置圆角
    QPainterPath path;
    //最后两个参数分别是x和y方向的圆角半径
    path.addRoundedRect(0, 0, originalPixmap.width(), originalPixmap.height(), 10, 10);
    painter.setClipPath(path); //设置剪切路径为圆角矩形，这样之后绘制的图片就会被剪切成圆角的形状
    painter.drawPixmap(0, 0, originalPixmap); //将原始图片绘制到roundedPixmap上
    ui->head_label->setPixmap(roundedPixmap); //设置绘制好的圆角图片到QLabel上
}

void LoginDialog::showTip(QString str, bool b_ok)
{
    if(b_ok){
        ui->err_tip->setProperty("state", "normal"); //设置状态为正常
    }else{
        ui->err_tip->setProperty("state", "err"); //设置状态为错误
    }

    ui->err_tip->setText(str); //设置提示信息
    repolish(ui->err_tip); //刷新样式表，使得状态生效
}

void LoginDialog::AddTipErr(TipErr te,QString tips)
{
    _tip_errs[te] = tips; //将错误类型和对应的提示信息插入到_tip_errs中
    showTip(tips, false); //显示错误提示信息
}

void LoginDialog::DelTipErr(TipErr te)
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

bool LoginDialog::checkEmailValid()
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

bool LoginDialog::checkPwdValid()
{
    auto pwd = ui->pass_edit->text(); //获取密码输入框的文本

    if(pwd.length() < 6 || pwd.length() > 15){
        qDebug() << "Pass length invalid";
        //提示长度不准确
        AddTipErr(TipErr::TIP_PWD_ERR, tr("密码长度应为6~15"));
        return false;
    }

    //密码长度至少6，可以是字母、数字和特定的特殊字符
    static QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*]{6,15}$");
    bool match = regExp.match(pwd).hasMatch(); //执行正则表达式匹配，判断密码是否符合要求
    if(!match){
        //提示字符非法
        AddTipErr(TipErr::TIP_PWD_ERR, tr("不能包含非法字符且长度为(6~15)"));
        return false;;
    }

    DelTipErr(TipErr::TIP_PWD_ERR); //如果密码合法，就从_tip_errs中删除密码错误提示信息
    return true;
}

bool LoginDialog::enableBtn(bool enabled)
{
    ui->login_btn->setEnabled(enabled); //设置登录按钮的可用状态
    ui->reg_btn->setEnabled(enabled); //设置注册按钮的可用状态
    return true;
}

void LoginDialog::slot_tcp_con_finish(bool bsuccess)
{
    //如果连接成功，就发送登录请求给聊天服务器
    if(bsuccess){
        showTip(tr("聊天服务连接成功，正在登录..."), true);
        QJsonObject jsonObj; //创建一个JSON对象，用于存储登录请求的数据
        jsonObj["uid"] = _uid; //将用户ID添加到JSON对象中
        jsonObj["token"] = _token; //将用户令牌添加到JSON对象中

        QJsonDocument doc(jsonObj); //将JSON对象转换为JSON文档，以便后续转换为字符串
        QString jsonString = doc.toJson(QJsonDocument::Indented);

        //发送tcp请求给chat server
        emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_CHAT_LOGIN, jsonString);
    }else
    {
        showTip(tr("网络异常"),false);
        enableBtn(true); //连接失败后重新启用登录和注册按钮
    }

}

void LoginDialog::slot_login_failed(int err)
{
    QString result = QString("登录失败, err is %1").arg(err);
    showTip(result, false);
    enableBtn(true); //登录失败后重新启用登录和注册按钮
}
