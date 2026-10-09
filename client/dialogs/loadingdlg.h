#ifndef LOADINGDLG_H
#define LOADINGDLG_H

#include <QDialog>
/******************************************************************************
 *
 * @file       loadingdlg.h
 * @brief      加载对话框
 *
 * @author     西故
 * @date       2026/05/07
 * @history
 *****************************************************************************/

namespace Ui {
class LoadingDlg;
}

class LoadingDlg : public QDialog
{
    Q_OBJECT

public:
    explicit LoadingDlg(QWidget *parent = nullptr);
    ~LoadingDlg();

private:
    Ui::LoadingDlg *ui;
};

#endif // LOADINGDLG_H
