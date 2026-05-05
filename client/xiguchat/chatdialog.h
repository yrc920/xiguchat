#ifndef CHATDIALOG_H
#define CHATDIALOG_H

#include <QDialog>
/******************************************************************************
 *
 * @file       chatdialog.h
 * @brief      聊天界面
 *
 * @author     西故
 * @date       2026/05/05
 * @history
 *****************************************************************************/

namespace Ui {
class ChatDialog;
}

class ChatDialog : public QDialog
{
    Q_OBJECT //使得ChatDialog可以使用Qt的信号和槽机制

public:
    explicit ChatDialog(QWidget *parent = nullptr);
    ~ChatDialog();

private:
    Ui::ChatDialog *ui;
};

#endif // CHATDIALOG_H
