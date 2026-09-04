#include "model/ChargerStatusOverview.h"

#include <QJsonValue>

#include <cmath>

namespace {

constexpr double kMaxSafeJsonInteger = 9007199254740991.0;

bool readCount(const QJsonObject& json,
               const QString& key,
               qint64* target,
               QString* errorMessage)
{
    const QJsonValue value = json.value(key);
    const double number = value.toDouble(-1.0);
    if (!value.isDouble() || !std::isfinite(number) || number < 0.0
        || std::floor(number) != number
        || number > kMaxSafeJsonInteger) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("字段 %1 必须是非负整数").arg(key);
        }
        return false;
    }
    *target = static_cast<qint64>(number);
    return true;
}

bool readPercent(const QJsonObject& json,
                 const QString& key,
                 double* target,
                 QString* errorMessage)
{
    const QJsonValue value = json.value(key);
    const double number = value.toDouble(-1.0);
    if (!value.isDouble() || !std::isfinite(number) || number < 0.0 || number > 100.0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("字段 %1 必须位于 0 到 100 之间").arg(key);
        }
        return false;
    }
    *target = number;
    return true;
}

} // namespace

bool ChargerStatusOverview::fromJson(const QJsonObject& json,
                                     ChargerStatusOverview* overview,
                                     QString* errorMessage)
{
    if (!overview) {
        return false;
    }

    ChargerStatusOverview parsed;
    if (!readCount(json, QStringLiteral("total"), &parsed.total, errorMessage)
        || !readCount(json, QStringLiteral("idle"), &parsed.idle, errorMessage)
        || !readCount(json, QStringLiteral("charging"), &parsed.charging, errorMessage)
        || !readCount(json, QStringLiteral("fault"), &parsed.fault, errorMessage)
        || !readCount(json, QStringLiteral("offline"), &parsed.offline, errorMessage)
        || !readPercent(json, QStringLiteral("idlePercent"),
                        &parsed.idlePercent, errorMessage)
        || !readPercent(json, QStringLiteral("chargingPercent"),
                        &parsed.chargingPercent, errorMessage)
        || !readPercent(json, QStringLiteral("faultPercent"),
                        &parsed.faultPercent, errorMessage)
        || !readPercent(json, QStringLiteral("offlinePercent"),
                        &parsed.offlinePercent, errorMessage)) {
        return false;
    }

    qint64 statusTotal = 0;
    const qint64 counts[] = {parsed.idle, parsed.charging,
                             parsed.fault, parsed.offline};
    for (const qint64 count : counts) {
        if (count > parsed.total - statusTotal) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("状态数量之和超过充电桩总数");
            }
            return false;
        }
        statusTotal += count;
    }
    if (statusTotal != parsed.total) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("充电桩总数与各状态数量之和不一致");
        }
        return false;
    }

    const double percentTotal = parsed.idlePercent + parsed.chargingPercent
        + parsed.faultPercent + parsed.offlinePercent;
    if ((parsed.total == 0 && std::abs(percentTotal) > 0.001)
        || (parsed.total > 0 && std::abs(percentTotal - 100.0) > 0.25)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("各状态占比之和与总数不一致");
        }
        return false;
    }

    const QJsonValue updatedAt = json.value(QStringLiteral("updatedAt"));
    if (!updatedAt.isUndefined() && !updatedAt.isNull()) {
        const double number = updatedAt.toDouble(-1.0);
        if (!updatedAt.isDouble() || !std::isfinite(number) || number < 0.0
            || std::floor(number) != number
            || number > kMaxSafeJsonInteger) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("字段 updatedAt 必须是非负整数时间戳");
            }
            return false;
        }
        parsed.updatedAtMs = static_cast<qint64>(number);
    }

    *overview = parsed;
    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}
