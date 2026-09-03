#pragma once

#include "model/Station.h"

#include <QFrame>

// 移动端附近站点卡片。整张卡可点击，业务详情页由上层通过 selected 信号接管。
class StationCard : public QFrame
{
    Q_OBJECT

public:
    explicit StationCard(const StationInfo& station, QWidget* parent = nullptr);

    qint64 stationId() const { return station_.stationId; }

signals:
    void selected(qint64 stationId);

protected:
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    StationInfo station_;
};
