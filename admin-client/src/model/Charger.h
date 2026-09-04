#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QtGlobal>

struct Charger final
{
    qint64 chargerId = 0;
    QString code;
    qint64 stationId = 0;
    QString stationName;
    int type = 0;
    double powerKw = 0.0;
    int status = 0;
    qint64 totalChargeCount = 0;
    qint64 totalChargeDurationSeconds = 0;

    QString typeLabel() const;
    QString statusLabel() const;

    static bool fromJson(const QJsonObject& json,
                         Charger* charger,
                         QString* errorMessage);
    static bool listFromJson(const QJsonArray& json,
                             QList<Charger>* chargers,
                             QString* errorMessage);
};

struct ChargerFilter final
{
    QString keyword;
    qint64 stationId = 0;
    int type = -1;
    int status = -1;

    bool matches(const Charger& charger) const;
};
