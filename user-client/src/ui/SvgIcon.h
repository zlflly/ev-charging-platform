#pragma once

#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QSvgRenderer>

namespace svg {

// Render SVG resources explicitly instead of relying on QIcon's optional SVG
// icon engine. QIcon returns a null pixmap when that engine is not deployed.
inline QPixmap tintedPixmap(const QString& resourcePath, const QSize& size,
                            const QColor& color)
{
    if (size.isEmpty()) {
        return {};
    }

    QSvgRenderer renderer(resourcePath);
    if (!renderer.isValid()) {
        return {};
    }

    QImage source(size, QImage::Format_ARGB32_Premultiplied);
    source.fill(Qt::transparent);
    QPainter sourcePainter(&source);
    sourcePainter.setRenderHint(QPainter::Antialiasing, true);
    sourcePainter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    renderer.render(&sourcePainter, QRectF(QPointF(0, 0), QSizeF(size)));

    QImage tinted(size, QImage::Format_ARGB32_Premultiplied);
    tinted.fill(color);
    QPainter tintPainter(&tinted);
    tintPainter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    tintPainter.drawImage(0, 0, source);
    return QPixmap::fromImage(tinted);
}

} // namespace svg
