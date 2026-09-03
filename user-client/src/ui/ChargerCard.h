#pragma once

#include "model/Station.h"

#include <QFrame>

// 站点详情中的充电桩行：编号、类型、功率和状态；空闲桩可继续进入充电流程。
class ChargerCard : public QFrame
{
    Q_OBJECT

public:
    explicit ChargerCard(const ChargerInfo& charger, int displayIndex,
                         QWidget* parent = nullptr);

    qint64 chargerId() const { return charger_.chargerId; }
    bool isIdle() const { return charger_.isIdle(); }

signals:
    void selected(qint64 chargerId);

protected:
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    ChargerInfo charger_;
};
