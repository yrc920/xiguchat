#ifndef SEARCHLIST_H
#define SEARCHLIST_H

#include <QListWidget>
#include <QWheelEvent>
#include <QEvent>
#include <QScrollBar>
#include <QDebug>
#include <QDialog>
#include <memory>
#include "userdata.h"
#include "loadingdlg.h"
/******************************************************************************
 *
 * @file       searchlist.h
 * @brief      搜索结果列表控件, 用于显示搜索结果的列表, 包括搜索提示项和搜索结果项
 *
 * @author     西故
 * @date       2026/05/09
 * @history
 *****************************************************************************/

class SearchList: public QListWidget
{
    Q_OBJECT //使得SearchList可以使用Qt的信号和槽机制

public:
    SearchList(QWidget *parent = nullptr);
    void CloseFindDlg();
    void SetSearchEdit(QWidget* edit);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void waitPending(bool pending = true);
    bool _send_pending;
    void addTipItem();
    std::shared_ptr<QDialog> _find_dlg;
    QWidget* _search_edit;
    LoadingDlg * _loadingDialog;

private slots:
    void slot_item_clicked(QListWidgetItem *item);
    void slot_user_search(std::shared_ptr<SearchInfo> si);

signals:

};


#endif // SEARCHLIST_H
