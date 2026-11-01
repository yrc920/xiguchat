#ifndef IMAGECROPPER_H
#define IMAGECROPPER_H

#include <QColor>
#include <QImage>
#include <QQuickPaintedItem>
#include <QRectF>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

// 头像编辑视图：图片解码、正方形选区与 PNG 输出留在 Gui/Quick 边界。
class ImageCropper : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool ready READ ready NOTIFY imageChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString previewSource READ previewSource NOTIFY cropChanged)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor maskColor READ maskColor WRITE setMaskColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor frameColor READ frameColor WRITE setFrameColor NOTIFY colorsChanged)

public:
    explicit ImageCropper(QQuickItem *parent = nullptr);
    bool ready() const { return !_image.isNull(); }
    QString error() const { return _error; }
    QString previewSource() const { return _preview; }
    QColor backgroundColor() const { return _backgroundColor; }
    QColor maskColor() const { return _maskColor; }
    QColor frameColor() const { return _frameColor; }
    void setBackgroundColor(const QColor &color);
    void setMaskColor(const QColor &color);
    void setFrameColor(const QColor &color);
    void paint(QPainter *painter) override;

    Q_INVOKABLE bool loadImage(const QUrl &url);
    Q_INVOKABLE void resetCrop();
    Q_INVOKABLE void zoom(qreal factor);
    Q_INVOKABLE void moveCrop(qreal dx, qreal dy);
    Q_INVOKABLE bool saveAvatar();
    Q_INVOKABLE void clear();

signals:
    void imageChanged();
    void errorChanged();
    void cropChanged();
    void colorsChanged();
    void saved();

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseUngrabEvent() override;
    void hoverMoveEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    enum Handle { Outside, Inside, Left, Right, Top, Bottom, TopLeft, TopRight, BottomLeft, BottomRight };
    QRectF imageRect() const;
    QRectF displayCrop() const;
    qreal displayScale() const;
    qreal minimumSide() const;
    Handle handleAt(const QPointF &position) const;
    void updateCursor(Handle handle);
    void setCrop(const QRectF &crop);
    void setError(const QString &message);
    QImage croppedImage(int side) const;

    QImage _image;
    QRectF _crop;
    QString _error;
    QString _preview;
    QColor _backgroundColor = QColor(0x29, 0x38, 0x30);
    QColor _maskColor = QColor(0, 0, 0, 150);
    QColor _frameColor = Qt::white;
    Handle _dragHandle = Outside;
    QPointF _dragStart;
    QRectF _dragCrop;
    int _ownerUid = 0;
    QString _ownerPath;
};

#endif // IMAGECROPPER_H
