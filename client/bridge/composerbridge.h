#ifndef COMPOSERBRIDGE_H
#define COMPOSERBRIDGE_H

#include <QObject>
#include <QHash>
#include <QImage>
#include <QQuickTextDocument>
#include <QUrl>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

// 富文本输入边界：插入本地图片、粘贴纯文本、按文档顺序读取内容。
class ComposerBridge : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString errorText READ errorText NOTIFY errorTextChanged)

public:
    explicit ComposerBridge(QObject *parent = nullptr);
    QString errorText() const { return _errorText; }
    Q_INVOKABLE int insertImage(QQuickTextDocument *document, int position, const QUrl &url);
    Q_INVOKABLE int paste(QQuickTextDocument *document, int position, int selectionEnd);
    Q_INVOKABLE void switchConversation(QQuickTextDocument *document, int uid);
    Q_INVOKABLE QVariantList parts(QQuickTextDocument *document) const;

signals:
    void errorTextChanged();

private:
    int insertImageData(QQuickTextDocument *document, int position, const QImage &image);
    void setError(const QString &message);
    QString _errorText;
    int _uid = 0;
    QHash<int, QString> _drafts;
};

#endif // COMPOSERBRIDGE_H
