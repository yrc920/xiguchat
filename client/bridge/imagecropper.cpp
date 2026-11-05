#include "imagecropper.h"

#include <QBuffer>
#include <QColorSpace>
#include <QCursor>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHoverEvent>
#include <QImageReader>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QSaveFile>
#include <QWheelEvent>
#include <cmath>
#include <utility>

#include "usermgr.h"

namespace {
constexpr qint64 MaximumFileBytes = 16 * 1024 * 1024;
constexpr qint64 MaximumPixels = 32 * 1000 * 1000;
constexpr qreal HandleRadius = 8;
constexpr qreal ImageMargin = 18;
}

ImageCropper::ImageCropper(QQuickItem *parent) : QQuickPaintedItem(parent)
{
    setAntialiasing(true);
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);
    setActiveFocusOnTab(true);
}

void ImageCropper::setBackgroundColor(const QColor &color)
{
    if (_backgroundColor == color)
        return;
    _backgroundColor = color;
    emit colorsChanged();
    update();
}

void ImageCropper::setMaskColor(const QColor &color)
{
    if (_maskColor == color)
        return;
    _maskColor = color;
    emit colorsChanged();
    update();
}

void ImageCropper::setFrameColor(const QColor &color)
{
    if (_frameColor == color)
        return;
    _frameColor = color;
    emit colorsChanged();
    update();
}

//选图入口：先按文件大小和像素总量挡掉大图，再解码。
//自动应用 EXIF 方向并统一到 sRGB，避免手机照片在裁剪框里是横的或偏色
bool ImageCropper::loadImage(const QUrl &url)
{
    if (url.isEmpty())
        return false;
    clear();
    if (!url.isLocalFile()) {
        setError(tr("请选择保存在电脑上的图片"));
        return false;
    }

    QFile file(url.toLocalFile());
    if (!QFileInfo(file).isFile() || !file.open(QIODevice::ReadOnly)) {
        setError(tr("无法读取这张图片，请检查文件是否存在或是否有读取权限"));
        return false;
    }
    if (file.size() <= 0 || file.size() > MaximumFileBytes) {
        setError(tr("请选择不超过 16 MB 的有效图片"));
        return false;
    }

    QImageReader reader(&file);
    reader.setAutoTransform(true);
    const QByteArray format = reader.format().toLower();
    if (format != "png" && format != "jpg" && format != "jpeg"
        && format != "bmp" && format != "webp") {
        setError(tr("图片格式不支持，请选择 PNG、JPEG、BMP 或 WebP 图片"));
        return false;
    }
    const QSize size = reader.size();
    if (!size.isValid() || qint64(size.width()) * size.height() > MaximumPixels) {
        setError(tr("图片尺寸无效或过大，请选择不超过 3200 万像素的图片"));
        return false;
    }
    QImage decoded = reader.read();
    if (decoded.isNull()) {
        setError(tr("图片无法打开，可能已损坏或格式不受支持"));
        return false;
    }
    if (qint64(decoded.width()) * decoded.height() > MaximumPixels) {
        setError(tr("图片尺寸过大，请选择不超过 3200 万像素的图片"));
        return false;
    }

    if (decoded.colorSpace().isValid())
        decoded.convertToColorSpace(QColorSpace::SRgb);
    decoded.setDevicePixelRatio(1);
    _image = std::move(decoded);
    _ownerUid = UserMgr::GetInstance()->GetUid();
    _ownerPath = UserMgr::LocalAvatarPath();
    emit imageChanged();
    resetCrop();
    return true;
}

void ImageCropper::clear()
{
    _dragHandle = Outside;
    _image = QImage();
    _crop = QRectF();
    _preview.clear();
    _ownerUid = 0;
    _ownerPath.clear();
    unsetCursor();
    setError({});
    emit imageChanged();
    emit cropChanged();
    update();
}

//图片按比例完整放进控件（上下或左右留白），裁剪坐标仍以原图像素为准
QRectF ImageCropper::imageRect() const
{
    if (!ready() || width() <= ImageMargin * 2 || height() <= ImageMargin * 2)
        return {};
    QSizeF size = _image.size();
    size.scale(QSizeF(width() - ImageMargin * 2, height() - ImageMargin * 2), Qt::KeepAspectRatio);
    return QRectF(QPointF((width() - size.width()) / 2, (height() - size.height()) / 2), size);
}

//原图与控件之间的比例；显示、拖动都要经过它换算
qreal ImageCropper::displayScale() const
{
    return ready() ? imageRect().width() / _image.width() : 0;
}

//把原图坐标系里的选区换算成控件坐标系，绘制和命中测试共用
QRectF ImageCropper::displayCrop() const
{
    const qreal scale = displayScale();
    return QRectF(imageRect().topLeft() + _crop.topLeft() * scale, _crop.size() * scale);
}

//选区最小 32 像素，既保证输出可用，也不让手柄挤在一起
qreal ImageCropper::minimumSide() const
{
    return qMin(32, qMin(_image.width(), _image.height()));
}

//默认取原图中央最大的正方形，短边用满
void ImageCropper::resetCrop()
{
    if (!ready())
        return;
    const qreal side = qMin(_image.width(), _image.height());
    setCrop(QRectF((_image.width() - side) / 2, (_image.height() - side) / 2, side, side));
}

//所有交互最终都落到这里：强制正方形、限制在图片内，并重新生成预览
void ImageCropper::setCrop(const QRectF &crop)
{
    if (!ready())
        return;
    const qreal side = qBound(minimumSide(), crop.width(), qreal(qMin(_image.width(), _image.height())));
    const QRectF bounded(qBound(qreal(0), crop.x(), _image.width() - side),
                         qBound(qreal(0), crop.y(), _image.height() - side), side, side);
    if (_crop == bounded && !_preview.isEmpty())
        return;
    _crop = bounded;
    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    croppedImage(128).save(&buffer, "PNG");
    _preview = QStringLiteral("data:image/png;base64,") + QString::fromLatin1(png.toBase64());
    emit cropChanged();
    update();
}

//按选区输出正方形图；预览用 128，保存用 512
QImage ImageCropper::croppedImage(int side) const
{
    if (!ready() || _crop.isEmpty())
        return {};
    QImage result(side, side, QImage::Format_ARGB32_Premultiplied);
    result.setColorSpace(QColorSpace::SRgb);
    result.fill(Qt::transparent);
    QPainter painter(&result);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.drawImage(QRectF(0, 0, side, side), _image, _crop);
    return result;
}

//以选区中心缩放：factor > 1 缩小选区（画面看起来放大），< 1 反之
void ImageCropper::zoom(qreal factor)
{
    if (!ready() || !std::isfinite(factor) || factor <= 0)
        return;
    const qreal side = qBound(minimumSide(), _crop.width() / factor,
                             qreal(qMin(_image.width(), _image.height())));
    setCrop(QRectF(_crop.center() - QPointF(side / 2, side / 2), QSizeF(side, side)));
}

//dx/dy 是控件像素，换算成原图像素后再移动
void ImageCropper::moveCrop(qreal dx, qreal dy)
{
    const qreal scale = displayScale();
    if (!ready() || scale <= 0 || !std::isfinite(dx) || !std::isfinite(dy))
        return;
    setCrop(_crop.translated(dx / scale, dy / scale));
}

//绘制顺序：底图 → 选区外压暗 → 圆角框与虚线 → 8 个手柄
void ImageCropper::paint(QPainter *painter)
{
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    painter->fillRect(boundingRect(), _backgroundColor);
    if (!ready() || imageRect().isEmpty())
        return;

    painter->drawImage(imageRect(), _image);
    const QRectF crop = displayCrop();
    QPainterPath outside;
    outside.addRect(imageRect());
    QPainterPath selected;
    selected.addRoundedRect(crop, crop.width() * 0.24, crop.height() * 0.24);
    painter->fillPath(outside.subtracted(selected), _maskColor);
    painter->setPen(QPen(_frameColor, 1.5));
    painter->drawPath(selected);
    painter->setPen(QPen(_frameColor, 1, Qt::DashLine));
    painter->drawRect(crop);
    painter->setPen(Qt::NoPen);
    painter->setBrush(_frameColor);
    const QPointF handles[] = {
        crop.topLeft(), QPointF(crop.center().x(), crop.top()), crop.topRight(),
        QPointF(crop.left(), crop.center().y()), QPointF(crop.right(), crop.center().y()),
        crop.bottomLeft(), QPointF(crop.center().x(), crop.bottom()), crop.bottomRight()
    };
    for (const auto &point : handles)
        painter->drawRoundedRect(QRectF(point - QPointF(3, 3), QSizeF(6, 6)), 1, 1);
}

//先判 8 个角/边手柄，再判是否落在选区内；手柄半径比视觉尺寸大，便于点中
ImageCropper::Handle ImageCropper::handleAt(const QPointF &position) const
{
    if (!ready() || imageRect().isEmpty())
        return Outside;
    const QRectF crop = displayCrop();
    const std::pair<QPointF, Handle> handles[] = {
        {crop.topLeft(), TopLeft}, {crop.topRight(), TopRight},
        {crop.bottomLeft(), BottomLeft}, {crop.bottomRight(), BottomRight},
        {QPointF(crop.left(), crop.center().y()), Left},
        {QPointF(crop.right(), crop.center().y()), Right},
        {QPointF(crop.center().x(), crop.top()), Top},
        {QPointF(crop.center().x(), crop.bottom()), Bottom}
    };
    for (const auto &handle : handles) {
        const QPointF offset = position - handle.first;
        if (qAbs(offset.x()) <= HandleRadius && qAbs(offset.y()) <= HandleRadius)
            return handle.second;
    }
    return crop.contains(position) ? Inside : Outside;
}

void ImageCropper::updateCursor(Handle handle)
{
    switch (handle) {
    case Inside: setCursor(Qt::SizeAllCursor); break;
    case Left: case Right: setCursor(Qt::SizeHorCursor); break;
    case Top: case Bottom: setCursor(Qt::SizeVerCursor); break;
    case TopLeft: case BottomRight: setCursor(Qt::SizeFDiagCursor); break;
    case TopRight: case BottomLeft: setCursor(Qt::SizeBDiagCursor); break;
    default: unsetCursor(); break;
    }
}

void ImageCropper::mousePressEvent(QMouseEvent *event)
{
    _dragHandle = handleAt(event->position());
    if (event->button() != Qt::LeftButton || _dragHandle == Outside) {
        _dragHandle = Outside;
        event->ignore();
        return;
    }
    forceActiveFocus(Qt::MouseFocusReason);
    _dragStart = event->position();
    _dragCrop = _crop;
    updateCursor(_dragHandle);
    event->accept();
}

void ImageCropper::mouseMoveEvent(QMouseEvent *event)
{
    const qreal scale = displayScale();
    if (_dragHandle == Outside || scale <= 0) {
        event->ignore();
        return;
    }
    const QPointF delta = (event->position() - _dragStart) / scale;
    if (_dragHandle == Inside) {
        setCrop(_dragCrop.translated(delta));
        event->accept();
        return;
    }

    //拖动边或角时保持正方形：以对角为锚点，边长同时受图片边界约束
    const int sx = (_dragHandle == Left || _dragHandle == TopLeft || _dragHandle == BottomLeft) ? -1
        : (_dragHandle == Right || _dragHandle == TopRight || _dragHandle == BottomRight) ? 1 : 0;
    const int sy = (_dragHandle == Top || _dragHandle == TopLeft || _dragHandle == TopRight) ? -1
        : (_dragHandle == Bottom || _dragHandle == BottomLeft || _dragHandle == BottomRight) ? 1 : 0;
    const qreal anchorX = sx < 0 ? _dragCrop.right() : _dragCrop.left();
    const qreal anchorY = sy < 0 ? _dragCrop.bottom() : _dragCrop.top();
    qreal change = sx ? sx * delta.x() : sy * delta.y();
    if (sx && sy && qAbs(delta.y()) > qAbs(delta.x()))
        change = sy * delta.y();
    const qreal maximumX = sx ? (sx > 0 ? _image.width() - anchorX : anchorX)
        : 2 * qMin(_dragCrop.center().x(), _image.width() - _dragCrop.center().x());
    const qreal maximumY = sy ? (sy > 0 ? _image.height() - anchorY : anchorY)
        : 2 * qMin(_dragCrop.center().y(), _image.height() - _dragCrop.center().y());
    const qreal side = qBound(minimumSide(), _dragCrop.width() + change, qMin(maximumX, maximumY));
    const qreal x = sx ? (sx > 0 ? anchorX : anchorX - side) : _dragCrop.center().x() - side / 2;
    const qreal y = sy ? (sy > 0 ? anchorY : anchorY - side) : _dragCrop.center().y() - side / 2;
    setCrop(QRectF(x, y, side, side));
    event->accept();
}

void ImageCropper::mouseReleaseEvent(QMouseEvent *event)
{
    _dragHandle = Outside;
    updateCursor(handleAt(event->position()));
    event->accept();
}

void ImageCropper::mouseUngrabEvent()
{
    _dragHandle = Outside;
    unsetCursor();
}

void ImageCropper::hoverMoveEvent(QHoverEvent *event)
{
    if (_dragHandle == Outside)
        updateCursor(handleAt(event->position()));
}

void ImageCropper::hoverLeaveEvent(QHoverEvent *)
{
    if (_dragHandle == Outside)
        unsetCursor();
}

//滚轮按 120 一格换算成缩放倍数；触控板给的是像素增量
void ImageCropper::wheelEvent(QWheelEvent *event)
{
    if (!ready()) {
        event->ignore();
        return;
    }
    const qreal delta = event->angleDelta().y() != 0 ? event->angleDelta().y() / 120.0
                                                   : event->pixelDelta().y() / 40.0;
    zoom(std::pow(1.12, delta));
    event->accept();
}

//键盘：方向键移动 1 像素（Shift 为 10），+/- 缩放，Home 重置
void ImageCropper::keyPressEvent(QKeyEvent *event)
{
    if (!ready()) {
        QQuickPaintedItem::keyPressEvent(event);
        return;
    }
    const qreal step = event->modifiers().testFlag(Qt::ShiftModifier) ? 10 : 1;
    switch (event->key()) {
    case Qt::Key_Left: moveCrop(-step, 0); break;
    case Qt::Key_Right: moveCrop(step, 0); break;
    case Qt::Key_Up: moveCrop(0, -step); break;
    case Qt::Key_Down: moveCrop(0, step); break;
    case Qt::Key_Plus: case Qt::Key_Equal: zoom(1.1); break;
    case Qt::Key_Minus: zoom(1 / 1.1); break;
    case Qt::Key_Home: resetCrop(); break;
    default: QQuickPaintedItem::keyPressEvent(event); return;
    }
    event->accept();
}

void ImageCropper::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
    _dragHandle = Outside;
    update();
}

void ImageCropper::setError(const QString &message)
{
    if (_error == message)
        return;
    _error = message;
    emit errorChanged();
}

//保存前重新核对账号与路径：打开编辑器后切换账号或网关时不能写进别人的目录
bool ImageCropper::saveAvatar()
{
    setError({});
    if (!ready()) {
        setError(tr("请先选择一张图片"));
        return false;
    }
    auto mgr = UserMgr::GetInstance();
    const QString path = mgr->LocalAvatarPath();
    if (_ownerUid <= 0 || mgr->GetUid() != _ownerUid || path != _ownerPath) {
        setError(tr("登录账号已变化，请重新打开头像编辑"));
        return false;
    }
    if (path.isEmpty() || !QDir().mkpath(QFileInfo(path).absolutePath())) {
        setError(tr("无法创建头像文件夹，请检查应用数据目录的权限和磁盘空间"));
        return false;
    }
    QByteArray png;
    QBuffer buffer(&png);
    if (!buffer.open(QIODevice::WriteOnly) || !croppedImage(512).save(&buffer, "PNG")) {
        setError(tr("头像生成失败，请重新选择图片后重试"));
        return false;
    }
    //QSaveFile 先写临时文件再原子替换，写到一半失败不会留下半个头像
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(png) != png.size() || !file.commit()) {
        setError(tr("头像保存失败，请检查磁盘空间和写入权限"));
        return false;
    }
    if (!mgr->SetLocalAvatar(png)) {
        setError(tr("账号状态已变化，请重新打开头像编辑"));
        return false;
    }
    emit saved();
    return true;
}
