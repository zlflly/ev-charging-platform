#include "model/Charger.h"

#include "protocol/Protocol.h"

#include <QJsonValue>
#include <QSet>

#include <cmath>

namespace {

constexpr double kMaxSafeJsonInteger = 9007199254740991.0;

bool readNonNegativeInteger(const QJsonObject& json,
                            const QString& key,
                            qint64* target,
                            QString* errorMessage,
                            bool requirePositive = false)
{
    const QJsonValue value = json.value(key);
    const double number = value.toDouble(-1.0);
    if (!value.isDouble() || !std::isfinite(number)
        || number < (requirePositive ? 1.0 : 0.0)
        || std::floor(number) != number || number > kMaxSafeJsonInteger) {
        if (errorMessage) {
            *errorMessage = requirePositive
                ? QStringLiteral("字段 %1 必须是正整数").arg(key)
                : QStringLiteral("字段 %1 必须是非负整数").arg(key);
        }
        return false;
    }
    *target = static_cast<qint64>(number);
    return true;
}

} // namespace

QString Charger::typeLabel() const
{
    return type == protocol::ChargerTypeFast
        ? QStringLiteral("快充") : QStringLiteral("慢充");
}

QString Charger::statusLabel() const
{
    switch (status) {
    case protocol::ChargerStatusIdle:     return QStringLiteral("空闲");
    case protocol::ChargerStatusCharging: return QStringLiteral("在用");
    case protocol::ChargerStatusFault:    return QStringLiteral("故障");
    case protocol::ChargerStatusOffline:  return QStringLiteral("离线");
    default:                              return QStringLiteral("未知");
    }
}

bool Charger::fromJson(const QJsonObject& json,
                       Charger* charger,
                       QString* errorMessage)
{
    if (!charger) {
        return false;
    }

    Charger parsed;
    if (!readNonNegativeInteger(json, QStringLiteral("chargerId"),
                                &parsed.chargerId, errorMessage, true)
        || !readNonNegativeInteger(json, QStringLiteral("stationId"),
                                   &parsed.stationId, errorMessage, true)
        || !readNonNegativeInteger(json, QStringLiteral("totalChargeCount"),
                                   &parsed.totalChargeCount, errorMessage)
        || !readNonNegativeInteger(json,
                                   QStringLiteral("totalChargeDurationSeconds"),
                                   &parsed.totalChargeDurationSeconds,
                                   errorMessage)) {
        return false;
    }

    parsed.code = json.value(QStringLiteral("code")).toString().trimmed();
    parsed.stationName = json.value(QStringLiteral("stationName")).toString().trimmed();
    if (parsed.code.isEmpty() || parsed.stationName.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("充电桩编号或所属电站为空");
        }
        return false;
    }

    qint64 parsedType = 0;
    if (!readNonNegativeInteger(json, QStringLiteral("type"),
                                &parsedType, errorMessage)
        || (parsedType != protocol::ChargerTypeFast
            && parsedType != protocol::ChargerTypeSlow)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("充电桩类型取值无效");
        }
        return false;
    }
    parsed.type = static_cast<int>(parsedType);

    qint64 parsedStatus = 0;
    if (!readNonNegativeInteger(json, QStringLiteral("status"),
                                &parsedStatus, errorMessage)
        || parsedStatus < protocol::ChargerStatusIdle
        || parsedStatus > protocol::ChargerStatusOffline) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("充电桩状态取值无效");
        }
        return false;
    }
    parsed.status = static_cast<int>(parsedStatus);

    const QJsonValue powerValue = json.value(QStringLiteral("powerKw"));
    parsed.powerKw = powerValue.toDouble(-1.0);
    if (!powerValue.isDouble() || !std::isfinite(parsed.powerKw)
        || parsed.powerKw <= 0.0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("充电桩功率必须是正数");
        }
        return false;
    }

    *charger = parsed;
    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}

bool Charger::listFromJson(const QJsonArray& json,
                           QList<Charger>* chargers,
                           QString* errorMessage)
{
    if (!chargers) {
        return false;
    }

    QList<Charger> parsed;
    QSet<qint64> chargerIds;
    parsed.reserve(json.size());
    for (int index = 0; index < json.size(); ++index) {
        if (!json.at(index).isObject()) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("第 %1 条充电桩数据不是对象").arg(index + 1);
            }
            return false;
        }
        Charger charger;
        QString itemError;
        if (!fromJson(json.at(index).toObject(), &charger, &itemError)) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("第 %1 条充电桩数据异常：%2")
                                    .arg(index + 1)
                                    .arg(itemError);
            }
            return false;
        }
        if (chargerIds.contains(charger.chargerId)) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("充电桩 ID %1 重复").arg(charger.chargerId);
            }
            return false;
        }
        chargerIds.insert(charger.chargerId);
        parsed.append(charger);
    }

    *chargers = parsed;
    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}

bool ChargerFilter::matches(const Charger& charger) const
{
    const QString normalizedKeyword = keyword.trimmed();
    const bool keywordMatches = normalizedKeyword.isEmpty()
        || charger.code.contains(normalizedKeyword, Qt::CaseInsensitive)
        || charger.stationName.contains(normalizedKeyword, Qt::CaseInsensitive);
    const bool stationMatches = stationId <= 0 || charger.stationId == stationId;
    const bool typeMatches = type < 0 || charger.type == type;
    const bool statusMatches = status < 0 || charger.status == status;
    return keywordMatches && stationMatches && typeMatches && statusMatches;
}

bool ChargerStatusUpdateRequest::validate(QString* errorMessage) const
{
    const auto isKnownStatus = [](int status) {
        return status >= protocol::ChargerStatusIdle
            && status <= protocol::ChargerStatusOffline;
    };
    const auto isAdminTarget = [](int status) {
        return status == protocol::ChargerStatusIdle
            || status == protocol::ChargerStatusFault
            || status == protocol::ChargerStatusOffline;
    };
    const QString normalizedReason = reason.trimmed();
    if (chargerId <= 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("充电桩 ID 必须是正整数");
        }
        return false;
    }
    if (!isKnownStatus(expectedStatus)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("当前充电桩状态无效");
        }
        return false;
    }
    if (expectedStatus == protocol::ChargerStatusCharging) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("在用状态由订单流程控制，管理员不能强制修改");
        }
        return false;
    }
    if (!isAdminTarget(targetStatus)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("管理员只能将设备调整为空闲、故障或离线");
        }
        return false;
    }
    if (targetStatus == expectedStatus) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("目标状态不能与当前状态相同");
        }
        return false;
    }
    if (normalizedReason.size() < 2 || normalizedReason.size() > 200) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("变更原因必须为 2～200 个字符");
        }
        return false;
    }
    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}

QJsonObject ChargerStatusUpdateRequest::toJson() const
{
    return {
        {QStringLiteral("chargerId"), chargerId},
        {QStringLiteral("expectedStatus"), expectedStatus},
        {QStringLiteral("targetStatus"), targetStatus},
        {QStringLiteral("reason"), reason.trimmed()},
    };
}

bool ChargerStatusUpdateResult::fromJson(const QJsonObject& json,
                                         ChargerStatusUpdateResult* result,
                                         QString* errorMessage)
{
    if (!result) {
        return false;
    }
    ChargerStatusUpdateResult parsed;
    qint64 previousStatus = 0;
    qint64 status = 0;
    if (!readNonNegativeInteger(json, QStringLiteral("chargerId"),
                                &parsed.chargerId, errorMessage, true)
        || !readNonNegativeInteger(json, QStringLiteral("previousStatus"),
                                   &previousStatus, errorMessage)
        || !readNonNegativeInteger(json, QStringLiteral("status"),
                                   &status, errorMessage)
        || previousStatus > protocol::ChargerStatusOffline
        || status > protocol::ChargerStatusOffline) {
        if (errorMessage && errorMessage->isEmpty()) {
            *errorMessage = QStringLiteral("状态变更响应包含无效状态");
        }
        return false;
    }
    parsed.previousStatus = static_cast<int>(previousStatus);
    parsed.status = static_cast<int>(status);
    *result = parsed;
    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}
