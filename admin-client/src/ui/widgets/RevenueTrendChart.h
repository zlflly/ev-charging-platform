#pragma once

#include "model/Revenue.h"

#include <QPointF>
#include <QRectF>
#include <QWidget>

class QEvent;
class QMouseEvent;
class QPaintEvent;

class RevenueTrendChart final : public QWidget
{
public:
    explicit RevenueTrendChart(QWidget* parent = nullptr);

    void showLoading(int days);
    void showError(const QString& message);
    void showTrend(const RevenueTrend& trend);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    enum class State { Loading, Ready, Error };

    QRectF plotRect() const;
    QPointF pointPosition(int index, double maximumRevenue) const;
    double axisMaximum() const;
    void updateHover(const QPoint& position);

    State state_ = State::Loading;
    RevenueTrend trend_;
    QString stateMessage_;
    int expectedDays_ = 7;
    int hoveredIndex_ = -1;
};
