#pragma once

#include <QDateTime>
#include <QJsonObject>
#include <QString>

// 充电订单 DTO。订单状态以服务端为准，客户端只负责呈现和发起动作。
struct OrderInfo
{
    enum Status {
        StatusUnknown,
        StatusReserved,
        StatusCharging,
        StatusWaitSettlement,
        StatusFinished,
        StatusCancelled,
    };

    qint64 orderId = 0;
    QString status;
    qint64 chargerId = 0;
    QString chargerCode;
    int chargerType = 0;
    qint64 stationId = 0;
    QString stationName;
    QString stationRegion;
    QString stationAddress;
    double latitude = 0.0;
    double longitude = 0.0;
    double pricePerKwh = 0.0;
    double powerKw = 0.0;
    qint64 createTimeMs = 0;
    qint64 startTimeMs = 0;
    qint64 stopTimeMs = 0;
    qint64 settleTimeMs = 0;
    double energyKwh = 0.0;
    // 目标充电量/电池容量（服务端若提供，用于把实际电量映射为进度）。
    double targetEnergyKwh = 0.0;
    // 服务端可直接返回 0~100 的真实进度；缺失时由电量/展示容量计算。
    double progressPercent = -1.0;
    double estimatedAmount = 0.0;
    double amount = 0.0;
    double electricityFee = 0.0;
    double serviceFee = 0.0;

    bool valid() const { return orderId > 0 && !status.isEmpty(); }

    Status statusEnum() const
    {
        if (status == QStringLiteral("RESERVED")) return StatusReserved;
        if (status == QStringLiteral("CHARGING")) return StatusCharging;
        if (status == QStringLiteral("WAIT_SETTLEMENT")) return StatusWaitSettlement;
        if (status == QStringLiteral("FINISHED")) return StatusFinished;
        if (status == QStringLiteral("CANCELLED")) return StatusCancelled;
        return StatusUnknown;
    }

    qint64 durationMs() const
    {
        if (startTimeMs <= 0) return 0;
        if (stopTimeMs > startTimeMs) return stopTimeMs - startTimeMs;
        if (statusEnum() == StatusCharging) {
            return QDateTime::currentMSecsSinceEpoch() - startTimeMs;
        }
        return 0;
    }

    static OrderInfo fromJson(const QJsonObject& json)
    {
        OrderInfo order;
        order.orderId = static_cast<qint64>(
            json.value(QStringLiteral("orderId")).toDouble());
        order.status = json.value(QStringLiteral("status")).toString();
        order.chargerId = static_cast<qint64>(
            json.value(QStringLiteral("chargerId")).toDouble());
        order.chargerCode = json.value(QStringLiteral("chargerCode")).toString();
        order.chargerType = json.value(QStringLiteral("type")).toInt();
        order.stationId = static_cast<qint64>(
            json.value(QStringLiteral("stationId")).toDouble());
        order.stationName = json.value(QStringLiteral("stationName")).toString();
        order.stationRegion = json.value(QStringLiteral("stationRegion")).toString();
        order.stationAddress = json.value(QStringLiteral("address")).toString();
        order.latitude = json.value(QStringLiteral("latitude")).toDouble();
        order.longitude = json.value(QStringLiteral("longitude")).toDouble();
        order.pricePerKwh = json.value(QStringLiteral("pricePerKwh")).toDouble();
        order.powerKw = json.value(QStringLiteral("powerKw")).toDouble();
        order.createTimeMs = static_cast<qint64>(
            json.value(QStringLiteral("createTime")).toDouble());
        order.startTimeMs = static_cast<qint64>(
            json.value(QStringLiteral("startTime")).toDouble());
        order.stopTimeMs = static_cast<qint64>(
            json.value(QStringLiteral("stopTime")).toDouble());
        order.settleTimeMs = static_cast<qint64>(
            json.value(QStringLiteral("settleTime")).toDouble());
        order.energyKwh = json.value(QStringLiteral("energyKwh")).toDouble();
        order.targetEnergyKwh = json.value(QStringLiteral("targetEnergyKwh")).toDouble();
        if (order.targetEnergyKwh <= 0.0) {
            order.targetEnergyKwh = json.value(QStringLiteral("batteryCapacityKwh")).toDouble();
        }
        if (order.targetEnergyKwh <= 0.0) {
            order.targetEnergyKwh = json.value(QStringLiteral("targetKwh")).toDouble();
        }
        order.progressPercent = json.value(QStringLiteral("progressPercent")).toDouble(-1.0);
        if (order.progressPercent < 0.0) {
            order.progressPercent = json.value(QStringLiteral("progress")).toDouble(-1.0);
        }
        order.estimatedAmount = json.value(QStringLiteral("estimatedAmount")).toDouble();
        order.amount = json.value(QStringLiteral("amount")).toDouble();
        order.electricityFee = json.value(QStringLiteral("electricityFee")).toDouble();
        order.serviceFee = json.value(QStringLiteral("serviceFee")).toDouble();
        return order;
    }
};
