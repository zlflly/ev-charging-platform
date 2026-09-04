#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QtGlobal>

struct Station final
{
    qint64 stationId = 0;
    QString name;
    QString address;
    double latitude = 0.0;
    double longitude = 0.0;
    double pricePerKwh = 0.0;
    qint64 totalCount = 0;
    double onlineRate = 0.0;
    qint64 version = 0;

    static bool fromJson(const QJsonObject& json,
                         Station* station,
                         QString* errorMessage);
    static bool listFromJson(const QJsonArray& json,
                             QList<Station>* stations,
                             QString* errorMessage);
};

struct StationFilter final
{
    QString keyword;

    bool matches(const Station& station) const;
};

struct StationCharger final
{
    qint64 chargerId = 0;
    QString code;
    int type = 0;
    double powerKw = 0.0;
    int status = 0;
    double pricePerKwh = 0.0;

    QString typeLabel() const;
    QString statusLabel() const;

    static bool fromDetailJson(const QJsonObject& json,
                               StationCharger* charger,
                               QString* errorMessage);
};

struct StationDetail final
{
    qint64 stationId = 0;
    QString name;
    QString address;
    double latitude = 0.0;
    double longitude = 0.0;
    double pricePerKwh = 0.0;
    qint64 availableCount = 0;
    qint64 totalCount = 0;
    QList<StationCharger> chargers;

    static bool fromJson(const QJsonObject& json,
                         StationDetail* detail,
                         QString* errorMessage);
};

struct StationCreateRequest final
{
    QString name;
    QString address;
    double latitude = 0.0;
    double longitude = 0.0;
    double pricePerKwh = 0.0;
    int chargerCount = 0;

    bool validate(QString* errorMessage) const;
    QJsonObject toJson() const;
};

struct StationCreateResult final
{
    qint64 stationId = 0;
    qint64 createdChargerCount = 0;

    static bool fromJson(const QJsonObject& json,
                         StationCreateResult* result,
                         QString* errorMessage);
};

struct StationUpdateRequest final
{
    qint64 stationId = 0;
    qint64 expectedVersion = 0;
    QString name;
    QString address;
    double latitude = 0.0;
    double longitude = 0.0;
    double pricePerKwh = 0.0;

    bool validate(QString* errorMessage) const;
    QJsonObject toJson() const;
};

struct StationUpdateResult final
{
    qint64 stationId = 0;
    qint64 version = 0;

    static bool fromJson(const QJsonObject& json,
                         StationUpdateResult* result,
                         QString* errorMessage);
};
