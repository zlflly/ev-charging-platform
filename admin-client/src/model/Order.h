#pragma once

#include <QJsonObject>
#include <QList>
#include <QString>
#include <QtGlobal>

struct AdminOrder final
{
    qint64 orderId = 0;
    qint64 userId = 0;
    QString phone;
    QString nickname;
    QString status;
    QString paymentStatus;
    QString amountKind;
    qint64 stationId = 0;
    QString stationName;
    qint64 chargerId = 0;
    QString chargerCode;
    int chargerType = 0;
    double powerKw = 0.0;
    double pricePerKwh = 0.0;
    double energyKwh = 0.0;
    double amount = 0.0;
    qint64 createdAtEpochMs = 0;
    qint64 startTimeEpochMs = 0;
    qint64 stopTimeEpochMs = 0;
    qint64 settleTimeEpochMs = 0;
    qint64 durationSeconds = 0;

    QString statusLabel() const;
    QString paymentStatusLabel() const;
    QString billingAmountLabel() const;
    QString formattedAmount() const;
    QString formattedEnergy() const;
    QString formattedPrice() const;
    QString formattedDuration() const;
    QString formattedCreatedAt() const;
    QString formattedStartTime() const;
    QString formattedStopTime() const;
    QString formattedSettleTime() const;
    QString userLabel() const;
    QString deviceLabel() const;

    static bool fromJson(const QJsonObject& json,
                         AdminOrder* order,
                         QString* errorMessage);
};

struct OrderListQuery final
{
    int page = 1;
    int pageSize = 20;
    QString keyword;
    QString status = QStringLiteral("ALL");
    QString paymentStatus = QStringLiteral("ALL");

    bool validate(QString* errorMessage) const;
    QJsonObject toJson() const;
};

struct OrderPlatformSummary final
{
    qint64 totalOrders = 0;
    qint64 reservedCount = 0;
    qint64 chargingCount = 0;
    qint64 waitSettlementCount = 0;
    qint64 finishedCount = 0;
    double paidAmount = 0.0;
};

struct OrderListPage final
{
    QList<AdminOrder> orders;
    qint64 total = 0;
    int page = 1;
    int pageSize = 20;
    qint64 generatedAtEpochMs = 0;
    OrderPlatformSummary platformSummary;

    QString formattedGeneratedAt() const;

    static bool fromJson(const QJsonObject& json,
                         const OrderListQuery& query,
                         OrderListPage* result,
                         QString* errorMessage);
};
