#include "model/Station.h"

#include "protocol/Protocol.h"

#include <QJsonValue>
#include <QSet>

#include <cmath>
#include <limits>

namespace {

constexpr double kMaxSafeJsonInteger = 9007199254740991.0;

bool readInteger(const QJsonObject& json,
                 const QString& key,
                 qint64* target,
                 bool positive,
                 QString* errorMessage)
{
    const QJsonValue value = json.value(key);
    const double number = value.toDouble(-1.0);
    const double minimum = positive ? 1.0 : 0.0;
    if (!value.isDouble() || !std::isfinite(number) || number < minimum
        || number > kMaxSafeJsonInteger || std::floor(number) != number) {
        if (errorMessage) {
            *errorMessage = positive
                ? QStringLiteral("字段 %1 必须是正整数").arg(key)
                : QStringLiteral("字段 %1 必须是非负整数").arg(key);
        }
        return false;
    }
    *target = static_cast<qint64>(number);
    return true;
}

bool readCoordinate(const QJsonObject& json,
                    const QString& key,
                    double minimum,
                    double maximum,
                    double* target,
                    QString* errorMessage)
{
    const QJsonValue value = json.value(key);
    const double number = value.toDouble(std::numeric_limits<double>::quiet_NaN());
    if (!value.isDouble() || !std::isfinite(number)
        || number < minimum || number > maximum) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("字段 %1 必须在 %2～%3 之间")
                                .arg(key)
                                .arg(minimum)
                                .arg(maximum);
        }
        return false;
    }
    *target = number;
    return true;
}

bool readRequiredText(const QJsonObject& json,
                      const QString& key,
                      int maximumLength,
                      QString* target,
                      QString* errorMessage)
{
    const QJsonValue value = json.value(key);
    const QString text = value.toString().trimmed();
    if (!value.isString() || text.isEmpty() || text.size() > maximumLength) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("字段 %1 必须为 1～%2 个字符")
                                .arg(key)
                                .arg(maximumLength);
        }
        return false;
    }
    *target = text;
    return true;
}

} // namespace

bool Station::fromJson(const QJsonObject& json,
                       Station* station,
                       QString* errorMessage)
{
    if (!station) {
        return false;
    }

    Station parsed;
    if (!readInteger(json, QStringLiteral("stationId"), &parsed.stationId,
                     true, errorMessage)
        || !readRequiredText(json, QStringLiteral("name"), 60,
                             &parsed.name, errorMessage)
        || !readRequiredText(json, QStringLiteral("address"), 200,
                             &parsed.address, errorMessage)
        || !readCoordinate(json, QStringLiteral("latitude"), -90.0, 90.0,
                           &parsed.latitude, errorMessage)
        || !readCoordinate(json, QStringLiteral("longitude"), -180.0, 180.0,
                           &parsed.longitude, errorMessage)
        || !readInteger(json, QStringLiteral("totalCount"), &parsed.totalCount,
                        false, errorMessage)
        || !readInteger(json, QStringLiteral("version"), &parsed.version,
                        true, errorMessage)) {
        return false;
    }

    const QJsonValue rateValue = json.value(QStringLiteral("onlineRate"));
    parsed.onlineRate = rateValue.toDouble(-1.0);
    if (!rateValue.isDouble() || !std::isfinite(parsed.onlineRate)
        || parsed.onlineRate < 0.0 || parsed.onlineRate > 100.0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("字段 onlineRate 必须是 0～100 的百分数");
        }
        return false;
    }
    if (parsed.totalCount == 0 && std::abs(parsed.onlineRate) > 0.0001) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("无充电桩站点的在线率必须为 0");
        }
        return false;
    }

    *station = parsed;
    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}

bool Station::listFromJson(const QJsonArray& json,
                           QList<Station>* stations,
                           QString* errorMessage)
{
    if (!stations) {
        return false;
    }

    QList<Station> parsed;
    QSet<qint64> ids;
    parsed.reserve(json.size());
    for (int index = 0; index < json.size(); ++index) {
        if (!json.at(index).isObject()) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("第 %1 条站点数据不是对象").arg(index + 1);
            }
            return false;
        }
        Station station;
        QString itemError;
        if (!fromJson(json.at(index).toObject(), &station, &itemError)) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("第 %1 条站点数据异常：%2")
                                    .arg(index + 1)
                                    .arg(itemError);
            }
            return false;
        }
        if (ids.contains(station.stationId)) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("站点 ID %1 重复").arg(station.stationId);
            }
            return false;
        }
        ids.insert(station.stationId);
        parsed.append(station);
    }

    *stations = parsed;
    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}

bool StationFilter::matches(const Station& station) const
{
    const QString normalized = keyword.trimmed();
    return normalized.isEmpty()
        || station.name.contains(normalized, Qt::CaseInsensitive)
        || station.address.contains(normalized, Qt::CaseInsensitive)
        || QString::number(station.stationId).contains(normalized);
}

QString StationCharger::typeLabel() const
{
    return type == protocol::ChargerTypeFast
        ? QStringLiteral("快充") : QStringLiteral("慢充");
}

QString StationCharger::statusLabel() const
{
    switch (status) {
    case protocol::ChargerStatusIdle:     return QStringLiteral("空闲");
    case protocol::ChargerStatusCharging: return QStringLiteral("在用");
    case protocol::ChargerStatusFault:    return QStringLiteral("故障");
    case protocol::ChargerStatusOffline:  return QStringLiteral("离线");
    default:                              return QStringLiteral("未知");
    }
}

bool StationCharger::fromDetailJson(const QJsonObject& json,
                                    StationCharger* charger,
                                    QString* errorMessage)
{
    if (!charger) {
        return false;
    }

    StationCharger parsed;
    if (!readInteger(json, QStringLiteral("chargerId"), &parsed.chargerId,
                     true, errorMessage)
        || !readRequiredText(json, QStringLiteral("chargerCode"), 80,
                             &parsed.code, errorMessage)) {
        return false;
    }

    qint64 type = 0;
    if (!readInteger(json, QStringLiteral("type"), &type, false, errorMessage)
        || (type != protocol::ChargerTypeFast && type != protocol::ChargerTypeSlow)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("充电桩类型取值无效");
        }
        return false;
    }
    parsed.type = static_cast<int>(type);

    qint64 status = 0;
    if (!readInteger(json, QStringLiteral("status"), &status, false, errorMessage)
        || status < protocol::ChargerStatusIdle
        || status > protocol::ChargerStatusOffline) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("充电桩状态取值无效");
        }
        return false;
    }
    parsed.status = static_cast<int>(status);

    const QJsonValue powerValue = json.value(QStringLiteral("power"));
    const QJsonValue priceValue = json.value(QStringLiteral("pricePerKwh"));
    parsed.powerKw = powerValue.toDouble(-1.0);
    parsed.pricePerKwh = priceValue.toDouble(-1.0);
    if (!powerValue.isDouble() || !std::isfinite(parsed.powerKw)
        || parsed.powerKw <= 0.0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("充电桩功率必须是正数");
        }
        return false;
    }
    if (!priceValue.isDouble() || !std::isfinite(parsed.pricePerKwh)
        || parsed.pricePerKwh < 0.0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("充电单价必须是非负数");
        }
        return false;
    }

    *charger = parsed;
    return true;
}

bool StationDetail::fromJson(const QJsonObject& json,
                             StationDetail* detail,
                             QString* errorMessage)
{
    if (!detail) {
        return false;
    }

    StationDetail parsed;
    if (!readInteger(json, QStringLiteral("stationId"), &parsed.stationId,
                     true, errorMessage)
        || !readInteger(json, QStringLiteral("version"), &parsed.version,
                        true, errorMessage)
        || !readRequiredText(json, QStringLiteral("name"), 60,
                             &parsed.name, errorMessage)
        || !readRequiredText(json, QStringLiteral("address"), 200,
                             &parsed.address, errorMessage)
        || !readCoordinate(json, QStringLiteral("latitude"), -90.0, 90.0,
                           &parsed.latitude, errorMessage)
        || !readCoordinate(json, QStringLiteral("longitude"), -180.0, 180.0,
                           &parsed.longitude, errorMessage)) {
        return false;
    }

    const QJsonValue chargersValue = json.value(QStringLiteral("chargers"));
    if (!chargersValue.isArray()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("站点详情响应缺少 chargers 数组");
        }
        return false;
    }

    QSet<qint64> ids;
    const QJsonArray chargers = chargersValue.toArray();
    parsed.chargers.reserve(chargers.size());
    for (int index = 0; index < chargers.size(); ++index) {
        if (!chargers.at(index).isObject()) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("第 %1 条站内桩数据不是对象").arg(index + 1);
            }
            return false;
        }
        StationCharger charger;
        QString itemError;
        if (!StationCharger::fromDetailJson(chargers.at(index).toObject(),
                                            &charger, &itemError)) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("第 %1 条站内桩数据异常：%2")
                                    .arg(index + 1)
                                    .arg(itemError);
            }
            return false;
        }
        if (ids.contains(charger.chargerId)) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("站内充电桩 ID %1 重复")
                                    .arg(charger.chargerId);
            }
            return false;
        }
        ids.insert(charger.chargerId);
        parsed.chargers.append(charger);
    }

    *detail = parsed;
    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}

bool StationCreateRequest::validate(QString* errorMessage) const
{
    const QString normalizedName = name.trimmed();
    const QString normalizedAddress = address.trimmed();
    if (normalizedName.isEmpty() || normalizedName.size() > 60) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("站名必须为 1～60 个字符");
        }
        return false;
    }
    if (normalizedAddress.isEmpty() || normalizedAddress.size() > 200) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("详细地址必须为 1～200 个字符");
        }
        return false;
    }
    if (!std::isfinite(latitude) || latitude < -90.0 || latitude > 90.0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("纬度必须在 -90～90 之间");
        }
        return false;
    }
    if (!std::isfinite(longitude) || longitude < -180.0 || longitude > 180.0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("经度必须在 -180～180 之间");
        }
        return false;
    }
    if (chargerCount < 0 || chargerCount > 100) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("初始电桩数量必须在 0～100 之间");
        }
        return false;
    }
    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}

QJsonObject StationCreateRequest::toJson() const
{
    return {
        {QStringLiteral("name"), name.trimmed()},
        {QStringLiteral("address"), address.trimmed()},
        {QStringLiteral("latitude"), latitude},
        {QStringLiteral("longitude"), longitude},
        {QStringLiteral("chargerCount"), chargerCount},
    };
}

bool StationCreateResult::fromJson(const QJsonObject& json,
                                   StationCreateResult* result,
                                   QString* errorMessage)
{
    if (!result) {
        return false;
    }
    StationCreateResult parsed;
    if (!readInteger(json, QStringLiteral("stationId"), &parsed.stationId,
                     true, errorMessage)
        || !readInteger(json, QStringLiteral("createdChargerCount"),
                        &parsed.createdChargerCount, false, errorMessage)) {
        return false;
    }
    *result = parsed;
    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}

bool StationUpdateRequest::validate(QString* errorMessage) const
{
    StationCreateRequest common;
    common.name = name;
    common.address = address;
    common.latitude = latitude;
    common.longitude = longitude;
    common.chargerCount = 0;
    if (stationId <= 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("站点 ID 必须是正整数");
        }
        return false;
    }
    if (expectedVersion <= 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("站点版本号无效，请刷新后重试");
        }
        return false;
    }
    return common.validate(errorMessage);
}

QJsonObject StationUpdateRequest::toJson() const
{
    return {
        {QStringLiteral("stationId"), stationId},
        {QStringLiteral("expectedVersion"), expectedVersion},
        {QStringLiteral("name"), name.trimmed()},
        {QStringLiteral("address"), address.trimmed()},
        {QStringLiteral("latitude"), latitude},
        {QStringLiteral("longitude"), longitude},
    };
}

bool StationUpdateResult::fromJson(const QJsonObject& json,
                                   StationUpdateResult* result,
                                   QString* errorMessage)
{
    if (!result) {
        return false;
    }
    StationUpdateResult parsed;
    if (!readInteger(json, QStringLiteral("stationId"), &parsed.stationId,
                     true, errorMessage)
        || !readInteger(json, QStringLiteral("version"), &parsed.version,
                        true, errorMessage)) {
        return false;
    }
    *result = parsed;
    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}
