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
    void CloseFindDlg(); //关闭搜索结果对话框
    void SetSearchEdit(QWidget* edit); //设置搜索框的指针, 用于在搜索结果列表中操作搜索框, 如清空文本等

protected:
    bool eventFilter(QObject *watched, QEvent *event) override; //事件过滤器

private:
    void waitPending(bool pending = true); //等待搜索请求, 显示加载状态
    void addTipItem(); //添加搜索提示项, 用于在搜索结果列表中显示搜索提示信息, 如"没有找到相关用户"等

    bool _send_pending; //是否正在发送搜索请求的标志, 用于防止重复发送搜索请求
    std::shared_ptr<QDialog> _find_dlg; //搜索结果对话框的智能指针, 用于在搜索结果列表中操作搜索结果对话框
    QWidget* _search_edit; //搜索框的指针, 用于在搜索结果列表中操作搜索框, 如清空文本等
    LoadingDlg * _loadingDialog; //加载对话框的指针, 用于在搜索结果列表中显示加载状态

private slots:
    void slot_item_clicked(QListWidgetItem *item); //搜索结果列表项点击的槽函数
    void slot_user_search(std::shared_ptr<SearchInfo> si);

signals:

};


#endif // SEARCHLIST_H
