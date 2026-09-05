#include "ui/widgets/RevenueTrendChart.h"

#include <QEvent>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QToolTip>

#include <algorithm>
#include <cmath>

namespace {

QString formatAxisMoney(double value)
{
    if (value >= 100000000.0) {
        return QStringLiteral("¥%1亿").arg(value / 100000000.0, 0, 'f', 1);
    }
    if (value >= 10000.0) {
        return QStringLiteral("¥%1万").arg(value / 10000.0, 0, 'f', 1);
    }
    return QStringLiteral("¥%1").arg(value, 0, 'f', value < 10.0 ? 1 : 0);
}

double niceMaximum(double value)
{
    if (value <= 0.0) return 1.0;
    const double exponent = std::pow(10.0, std::floor(std::log10(value)));
    const double normalized = value / exponent;
    const double rounded = normalized <= 1.0 ? 1.0
        : normalized <= 2.0 ? 2.0
        : normalized <= 5.0 ? 5.0 : 10.0;
    return rounded * exponent;
}

} // namespace

RevenueTrendChart::RevenueTrendChart(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(330);
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    showLoading(7);
}

void RevenueTrendChart::showLoading(int days)
{
    state_ = State::Loading;
    expectedDays_ = days;
    hoveredIndex_ = -1;
    trend_ = {};
    stateMessage_ = QStringLiteral("正在加载最近 %1 日已结算营收…").arg(days);
    update();
}

void RevenueTrendChart::showError(const QString& message)
{
    state_ = State::Error;
    hoveredIndex_ = -1;
    trend_ = {};
    stateMessage_ = message.isEmpty()
        ? QStringLiteral("营收趋势加载失败，请重试") : message;
    update();
}

void RevenueTrendChart::showTrend(const RevenueTrend& trend)
{
    state_ = State::Ready;
    expectedDays_ = trend.days;
    hoveredIndex_ = -1;
    trend_ = trend;
    stateMessage_.clear();
    update();
}

QRectF RevenueTrendChart::plotRect() const
{
    return QRectF(72.0, 24.0,
                  std::max(10.0, width() - 98.0),
                  std::max(10.0, height() - 82.0));
}

double RevenueTrendChart::axisMaximum() const
{
    double maximum = 0.0;
    for (const auto& point : trend_.points) {
        maximum = std::max(maximum, point.revenue);
    }
    return niceMaximum(maximum * 1.12);
}

QPointF RevenueTrendChart::pointPosition(int index, double maximumRevenue) const
{
    const QRectF area = plotRect();
    const int count = trend_.points.size();
    const double x = count <= 1 ? area.center().x()
        : area.left() + area.width() * static_cast<double>(index)
              / static_cast<double>(count - 1);
    const double ratio = maximumRevenue <= 0.0 ? 0.0
        : trend_.points.at(index).revenue / maximumRevenue;
    return QPointF(x, area.bottom() - ratio * area.height());
}

void RevenueTrendChart::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor(QStringLiteral("#FFFFFF")));

    if (state_ != State::Ready || trend_.points.isEmpty()) {
        painter.setPen(QColor(state_ == State::Error
                                  ? QStringLiteral("#C43742")
                                  : QStringLiteral("#64778B")));
        QFont font = painter.font();
        font.setPointSize(11);
        font.setWeight(QFont::DemiBold);
        painter.setFont(font);
        painter.drawText(rect().adjusted(24, 24, -24, -24),
                         Qt::AlignCenter | Qt::TextWordWrap, stateMessage_);
        return;
    }

    const QRectF area = plotRect();
    const double maximumRevenue = axisMaximum();
    painter.setFont(QFont(painter.font().family(), 9));
    for (int line = 0; line <= 4; ++line) {
        const double y = area.bottom() - area.height() * line / 4.0;
        painter.setPen(QPen(QColor(QStringLiteral("#E4EAF0")), 1.0));
        painter.drawLine(QPointF(area.left(), y), QPointF(area.right(), y));
        painter.setPen(QColor(QStringLiteral("#718399")));
        painter.drawText(QRectF(0.0, y - 10.0, area.left() - 10.0, 20.0),
                         Qt::AlignRight | Qt::AlignVCenter,
                         formatAxisMoney(maximumRevenue * line / 4.0));
    }

    const int labelStep = trend_.days == 30 ? 5 : 1;
    for (int index = 0; index < trend_.points.size(); ++index) {
        if (index % labelStep != 0 && index != trend_.points.size() - 1) continue;
        const QPointF position = pointPosition(index, maximumRevenue);
        painter.setPen(QColor(QStringLiteral("#718399")));
        painter.drawText(QRectF(position.x() - 34.0, area.bottom() + 10.0, 68.0, 22.0),
                         Qt::AlignHCenter | Qt::AlignTop,
                         trend_.points.at(index).date.toString(QStringLiteral("MM-dd")));
    }

    QPainterPath linePath;
    for (int index = 0; index < trend_.points.size(); ++index) {
        const QPointF point = pointPosition(index, maximumRevenue);
        if (index == 0) linePath.moveTo(point);
        else linePath.lineTo(point);
    }
    QPainterPath fillPath = linePath;
    fillPath.lineTo(area.right(), area.bottom());
    fillPath.lineTo(area.left(), area.bottom());
    fillPath.closeSubpath();
    QLinearGradient fill(area.topLeft(), area.bottomLeft());
    fill.setColorAt(0.0, QColor(36, 107, 253, 70));
    fill.setColorAt(1.0, QColor(36, 107, 253, 4));
    painter.fillPath(fillPath, fill);
    painter.setPen(QPen(QColor(QStringLiteral("#246BFD")), 2.5,
                        Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(linePath);

    for (int index = 0; index < trend_.points.size(); ++index) {
        const QPointF point = pointPosition(index, maximumRevenue);
        const bool hovered = index == hoveredIndex_;
        painter.setPen(QPen(QColor(QStringLiteral("#246BFD")), 2.0));
        painter.setBrush(hovered ? QColor(QStringLiteral("#246BFD"))
                                 : QColor(QStringLiteral("#FFFFFF")));
        painter.drawEllipse(point, hovered ? 5.5 : 3.5, hovered ? 5.5 : 3.5);
    }

    const bool allZero = std::all_of(
        trend_.points.cbegin(), trend_.points.cend(),
        [](const RevenueTrendPoint& point) { return point.revenue == 0.0; });
    if (allZero) {
        painter.setPen(QColor(QStringLiteral("#64778B")));
        painter.drawText(QRectF(area.left(), area.top() + 10.0,
                                area.width(), 24.0),
                         Qt::AlignHCenter | Qt::AlignTop,
                         QStringLiteral("当前周期暂无已结算营收"));
    }

    if (hoveredIndex_ >= 0 && hoveredIndex_ < trend_.points.size()) {
        const QPointF point = pointPosition(hoveredIndex_, maximumRevenue);
        painter.setPen(QPen(QColor(QStringLiteral("#8DB7EA")), 1.0, Qt::DashLine));
        painter.drawLine(QPointF(point.x(), area.top()),
                         QPointF(point.x(), area.bottom()));
    }
}

void RevenueTrendChart::mouseMoveEvent(QMouseEvent* event)
{
    updateHover(event->position().toPoint());
    QWidget::mouseMoveEvent(event);
}

void RevenueTrendChart::leaveEvent(QEvent* event)
{
    hoveredIndex_ = -1;
    QToolTip::hideText();
    update();
    QWidget::leaveEvent(event);
}

void RevenueTrendChart::updateHover(const QPoint& position)
{
    if (state_ != State::Ready || trend_.points.isEmpty()
        || !plotRect().adjusted(-8, -8, 8, 8).contains(position)) {
        if (hoveredIndex_ != -1) {
            hoveredIndex_ = -1;
            QToolTip::hideText();
            update();
        }
        return;
    }
    const QRectF area = plotRect();
    const double step = trend_.points.size() <= 1 ? area.width()
        : area.width() / static_cast<double>(trend_.points.size() - 1);
    const int index = qBound(0, qRound((position.x() - area.left()) / step),
                             trend_.points.size() - 1);
    if (index == hoveredIndex_) return;
    hoveredIndex_ = index;
    const auto& point = trend_.points.at(index);
    QToolTip::showText(mapToGlobal(position),
                       QStringLiteral("%1\n已结算营收：¥ %2")
                           .arg(point.date.toString(QStringLiteral("yyyy-MM-dd")))
                           .arg(point.revenue, 0, 'f', 2), this);
    update();
}
