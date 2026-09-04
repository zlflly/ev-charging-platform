#pragma once

#include <QJsonObject>
#include <QString>
#include <QtGlobal>

// 服务端聚合后的全局充电桩状态快照。
// 客户端只校验和展示，不通过下载单桩列表重复统计。
struct ChargerStatusOverview final
{
    qint64 total = 0;
    qint64 idle = 0;
    qint64 charging = 0;
    qint64 fault = 0;
    qint64 offline = 0;

    double idlePercent = 0.0;
    double chargingPercent = 0.0;
    double faultPercent = 0.0;
    double offlinePercent = 0.0;

    qint64 updatedAtMs = 0;

    static bool fromJson(const QJsonObject& json,
                         ChargerStatusOverview* overview,
                         QString* errorMessage);
};
