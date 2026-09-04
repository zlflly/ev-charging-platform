#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QString>

#include "protocol/Protocol.h"

// 站点 DTO：服务端 station.nearby / station.detail 响应 -> 页面模型。
struct StationInfo {
    qint64 stationId = 0;
    QString name;
    double pricePerKwh = 0.0;
    int totalChargers = 0;
    int availableChargers = 0;
    double distanceKm = 0.0;
    double latitude = 0.0;
    double longitude = 0.0;

    bool valid() const { return stationId > 0; }

    static StationInfo fromJson(const QJsonObject& json)
    {
        StationInfo station;
        station.stationId = static_cast<qint64>(
            json.value(QStringLiteral("stationId")).toDouble());
        station.name = json.value(QStringLiteral("name")).toString();
        station.pricePerKwh = json.value(QStringLiteral("price")).toDouble();
        station.totalChargers = json.value(QStringLiteral("totalChargers")).toInt();
        station.availableChargers =
            json.value(QStringLiteral("availableChargers")).toInt();
        station.distanceKm = json.value(QStringLiteral("distanceKm")).toDouble();
        station.latitude = json.value(QStringLiteral("latitude")).toDouble();
        station.longitude = json.value(QStringLiteral("longitude")).toDouble();
        return station;
    }

    static QList<StationInfo> fromJsonArray(const QJsonArray& array)
    {
        QList<StationInfo> stations;
        stations.reserve(array.size());
        for (const QJsonValue& value : array) {
            const StationInfo station = fromJson(value.toObject());
            if (station.valid()) {
                stations.append(station);
            }
        }
        return stations;
    }
};

struct ChargerInfo {
    qint64 chargerId = 0;
    QString code;
    int type = protocol::ChargerTypeFast;
    int status = protocol::ChargerStatusOffline;
    double powerKw = 0.0;

    bool valid() const { return chargerId > 0; }
    bool isIdle() const { return status == protocol::ChargerStatusIdle; }

    QString typeLabel() const
    {
        return type == protocol::ChargerTypeFast
            ? QStringLiteral("快充") : QStringLiteral("慢充");
    }

    QString statusLabel() const
    {
        switch (status) {
        case protocol::ChargerStatusIdle:     return QStringLiteral("空闲");
        case protocol::ChargerStatusCharging: return QStringLiteral("充电中");
        case protocol::ChargerStatusFault:    return QStringLiteral("故障");
        default:                              return QStringLiteral("离线");
        }
    }

    static ChargerInfo fromJson(const QJsonObject& json)
    {
        ChargerInfo charger;
        charger.chargerId = static_cast<qint64>(
            json.value(QStringLiteral("chargerId")).toDouble());
        charger.code = json.value(QStringLiteral("code")).toString();
        charger.type = json.value(QStringLiteral("type")).toInt();
        charger.status = json.value(QStringLiteral("status")).toInt();
        charger.powerKw = json.value(QStringLiteral("powerKw")).toDouble();
        return charger;
    }
};

struct StationDetail {
    StationInfo station;
    QString address;
    double latitude = 0.0;
    double longitude = 0.0;
    QList<ChargerInfo> chargers;
};
