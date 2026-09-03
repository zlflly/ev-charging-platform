#include "ui/ChargingHeroWidget.h"

#include "ui/theme/Theme.h"

#include <QPainter>
#include <QPainterPath>

ChargingHeroWidget::ChargingHeroWidget(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setStyleSheet(QStringLiteral("background: transparent;"));
}

void ChargingHeroWidget::setProgress(double percent)
{
    progressPercent_ = qBound(0.0, percent, 100.0);
    update();
}

void ChargingHeroWidget::setElapsedText(const QString& text)
{
    elapsedText_ = text;
    update();
}

void ChargingHeroWidget::setSubtitleVisible(bool visible)
{
    showSubtitle_ = visible;
    update();
}

void ChargingHeroWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QPointF center(width() / 2.0, 147.0);
    const qreal radius = qMin(width() * 0.29, 126.0);
    const qreal ringWidth = 10.0;
    const QRectF ringRect(center.x() - radius, center.y() - radius,
                          radius * 2.0, radius * 2.0);

    QPen outerPen(theme::cardBorder());
    outerPen.setWidthF(1.0);
    painter.setPen(outerPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(center, radius + 22, radius + 22);

    QPen trackPen(theme::inputFill());
    trackPen.setWidthF(ringWidth);
    trackPen.setCapStyle(Qt::RoundCap);
    painter.setPen(trackPen);
    painter.drawEllipse(ringRect);

    if (progressPercent_ > 0) {
        QPen progressPen(theme::primaryBlue());
        progressPen.setWidthF(ringWidth);
        progressPen.setCapStyle(Qt::RoundCap);
        painter.setPen(progressPen);
        // QPainter 的角度单位是 1/16 度，保留到这个精度才能让弧线
        // 在 250ms 刷新周期内连续移动，而不是等整数百分比变化。
        const int span16 = qRound(360.0 * progressPercent_ * 16.0 / 100.0);
        painter.drawArc(ringRect, 90 * 16, -span16);
    }

    QFont percentFont(QStringLiteral("Microsoft YaHei UI"));
    percentFont.setPixelSize(56);
    percentFont.setWeight(QFont::Bold);
    painter.setFont(percentFont);
    painter.setPen(theme::textPrimary());
    painter.drawText(QRectF(center.x() - radius, center.y() - 43,
                            radius * 2, 64),
                     Qt::AlignCenter,
                     QStringLiteral("%1%").arg(qRound(progressPercent_)));

    if (showSubtitle_ && !elapsedText_.isEmpty()) {
        QFont subFont(QStringLiteral("Microsoft YaHei UI"));
        subFont.setPixelSize(16);
        painter.setFont(subFont);
        painter.setPen(theme::textSecondary());
        painter.drawText(QRectF(center.x() - radius, center.y() + 31,
                                radius * 2, 28),
                         Qt::AlignCenter, QStringLiteral("已充 %1").arg(elapsedText_));
        painter.setPen(theme::primaryBlue());
        painter.setBrush(theme::primaryBlue());
        QPainterPath bolt;
        bolt.moveTo(center.x() - 76, center.y() + 37);
        bolt.lineTo(center.x() - 84, center.y() + 51);
        bolt.lineTo(center.x() - 78, center.y() + 50);
        bolt.lineTo(center.x() - 82, center.y() + 62);
        bolt.lineTo(center.x() - 70, center.y() + 44);
        bolt.lineTo(center.x() - 76, center.y() + 45);
        bolt.closeSubpath();
        painter.drawPath(bolt);
    }
}
