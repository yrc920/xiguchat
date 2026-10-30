#include "composerbridge.h"

#include <QBuffer>
#include <QClipboard>
#include <QGuiApplication>
#include <QImage>
#include <QImageReader>
#include <QMimeData>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextFragment>

ComposerBridge::ComposerBridge(QObject *parent) : QObject(parent)
{
}

void ComposerBridge::setError(const QString &message)
{
    _errorText = message;
    emit errorTextChanged();
}

int ComposerBridge::insertImage(QQuickTextDocument *document, int position, const QUrl &url)
{
    if (!url.isLocalFile()) {
        setError(tr("请拖入本地图片文件"));
        return position;
    }
    QImageReader reader(url.toLocalFile());
    reader.setAutoTransform(true);
    const QSize size = reader.size();
    if (size.isValid() && (size.width() > 1600 || size.height() > 1600))
        reader.setScaledSize(size.scaled(1600, 1600, Qt::KeepAspectRatio));
    return insertImageData(document, position, reader.read());
}

int ComposerBridge::insertImageData(QQuickTextDocument *document, int position, const QImage &image)
{
    if (!document || !document->textDocument())
        return position;
    if (image.isNull()) {
        setError(tr("无法读取图片；暂不支持文件发送"));
        return position;
    }

    // 预览保存图片内容，原文件移动或输入框清空后仍能显示。
    const QImage preview = image.width() > 800 || image.height() > 600
        ? image.scaled(800, 600, Qt::KeepAspectRatio, Qt::SmoothTransformation) : image;
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    preview.save(&buffer, "PNG");
    const QString source = QStringLiteral("data:image/png;base64,")
        + QString::fromLatin1(bytes.toBase64());
    const QSize size = preview.width() > 160 || preview.height() > 100
        ? preview.size().scaled(160, 100, Qt::KeepAspectRatio) : preview.size();
    QTextImageFormat format;
    format.setName(source);
    format.setWidth(size.width());
    format.setHeight(size.height());
    QTextCursor cursor(document->textDocument());
    cursor.setPosition(qBound(0, position, document->textDocument()->characterCount() - 1));
    cursor.insertImage(format);
    setError({});
    return cursor.position();
}

int ComposerBridge::paste(QQuickTextDocument *document, int position, int selectionEnd)
{
    if (!document || !document->textDocument())
        return position;
    const QMimeData *mime = QGuiApplication::clipboard()->mimeData();
    if (!mime)
        return position;
    QTextCursor cursor(document->textDocument());
    const int end = document->textDocument()->characterCount() - 1;
    cursor.setPosition(qBound(0, position, end));
    cursor.setPosition(qBound(0, selectionEnd, end), QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
    position = cursor.position();
    if (mime->hasUrls()) {
        for (const QUrl &url : mime->urls())
            position = insertImage(document, position, url);
        return position;
    }
    if (mime->hasImage())
        return insertImageData(document, position, qvariant_cast<QImage>(mime->imageData()));
    cursor.insertText(mime->text());
    return cursor.position();
}

QVariantList ComposerBridge::parts(QQuickTextDocument *document) const
{
    QVariantList result;
    if (!document || !document->textDocument())
        return result;
    QString text;
    const auto flush = [&] {
        if (text.isEmpty())
            return;
        result.append(QVariantMap{{"kind", "text"}, {"text", text}});
        text.clear();
    };
    for (QTextBlock block = document->textDocument()->begin(); block.isValid(); block = block.next()) {
        if (block != document->textDocument()->begin())
            text += QLatin1Char('\n');
        for (auto it = block.begin(); !it.atEnd(); ++it) {
            const QTextFragment fragment = it.fragment();
            if (!fragment.isValid())
                continue;
            if (fragment.charFormat().isImageFormat()) {
                flush();
                const auto format = fragment.charFormat().toImageFormat();
                // 一个 fragment 可能包含多个相同图片对象。
                for (int i = 0; i < fragment.length(); ++i)
                    result.append(QVariantMap{{"kind", "image"}, {"source", format.name()},
                        {"width", format.width()}, {"height", format.height()}});
            } else {
                text += fragment.text();
            }
        }
    }
    flush();
    return result;
}

void ComposerBridge::switchConversation(QQuickTextDocument *document, int uid)
{
    if (!document || !document->textDocument() || uid == _uid)
        return;
    auto *doc = document->textDocument();
    if (_uid != 0)
        _drafts.insert(_uid, doc->toHtml());
    _uid = uid;
    doc->setHtml(_drafts.value(uid));
    setError({});
}
