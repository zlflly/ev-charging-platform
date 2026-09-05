#include "model/Order.h"

#include "protocol/Protocol.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonValue>
#include <QRegularExpression>
#include <QSet>

#include <cmath>

namespace {

constexpr double kMaxSafeJsonInteger = 9007199254740991.0;

bool readInteger(const QJsonObject& json, const QString& key, qint64* target,
                 bool positive, QString* errorMessage)
{
    const QJsonValue value = json.value(key);
    const double number = value.toDouble(-1.0);
    const double minimum = positive ? 1.0 : 0.0;
    if (!value.isDouble() || !std::isfinite(number) || number < minimum
        || number > kMaxSafeJsonInteger || std::floor(number) != number) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("字段 %1 必须是%2整数")
                .arg(key, positive ? QStringLiteral("正") : QStringLiteral("非负"));
        }
        return false;
    }
    *target = static_cast<qint64>(number);
    return true;
}

bool readNumber(const QJsonObject& json, const QString& key, double* target,
                QString* errorMessage)
{
    const QJsonValue value = json.value(key);
    const double number = value.toDouble(-1.0);
    if (!value.isDouble() || !std::isfinite(number) || number < 0.0) {
        if (errorMessage) *errorMessage = QStringLiteral("字段 %1 必须是非负数").arg(key);
        return false;
    }
    *target = number;
    return true;
}

bool readText(const QJsonObject& json, const QString& key, int maximum,
              QString* target, QString* errorMessage)
{
    const QJsonValue value = json.value(key);
    const QString text = value.toString().trimmed();
    if (!value.isString() || text.isEmpty() || text.size() > maximum) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("字段 %1 必须为 1～%2 个字符").arg(key).arg(maximum);
        }
        return false;
    }
    *target = text;
    return true;
}

QDateTime parseIsoTime(const QString& value)
{
    QDateTime parsed = QDateTime::fromString(value, Qt::ISODateWithMs);
    if (!parsed.isValid()) parsed = QDateTime::fromString(value, Qt::ISODate);
    return parsed;
}

bool readTime(const QJsonObject& json, const QString& key, qint64* target,
              bool allowZero, QString* errorMessage)
{
    const QJsonValue value = json.value(key);
    if (value.isDouble()) {
        qint64 parsed = 0;
        if (!readInteger(json, key, &parsed, !allowZero, errorMessage)) return false;
        *target = parsed;
        return true;
    }
    if (value.isString()) {
        const QDateTime parsed = parseIsoTime(value.toString());
        if (parsed.isValid() && (allowZero || parsed.toMSecsSinceEpoch() > 0)) {
            *target = parsed.toMSecsSinceEpoch();
            return true;
        }
    }
    if (errorMessage) {
        *errorMessage = QStringLiteral("字段 %1 必须是 epoch 毫秒或 ISO 8601 时间").arg(key);
    }
    return false;
}

bool validOrderStatus(const QString& status)
{
    return status == QString::fromUtf8(protocol::orderStatus::kReserved)
        || status == QString::fromUtf8(protocol::orderStatus::kCharging)
        || status == QString::fromUtf8(protocol::orderStatus::kWaitSettlement)
        || status == QString::fromUtf8(protocol::orderStatus::kFinished);
}

bool validOrderFilter(const QString& status)
{
    return status == QStringLiteral("ALL") || validOrderStatus(status);
}

bool validPaymentStatus(const QString& status)
{
    return status == QStringLiteral("NOT_DUE")
        || status == QStringLiteral("UNPAID")
        || status == QStringLiteral("PAID");
}

bool validPaymentFilter(const QString& status)
{
    return status == QStringLiteral("ALL") || validPaymentStatus(status);
}

QString expectedPaymentStatus(const QString& orderStatus)
{
    if (orderStatus == QString::fromUtf8(protocol::orderStatus::kWaitSettlement)) {
        return QStringLiteral("UNPAID");
    }
    if (orderStatus == QString::fromUtf8(protocol::orderStatus::kFinished)) {
        return QStringLiteral("PAID");
    }
    return QStringLiteral("NOT_DUE");
}

QString expectedAmountKind(const QString& orderStatus)
{
    if (orderStatus == QString::fromUtf8(protocol::orderStatus::kReserved)) {
        return QStringLiteral("NONE");
    }
    if (orderStatus == QString::fromUtf8(protocol::orderStatus::kCharging)) {
        return QStringLiteral("ESTIMATED");
    }
    return QStringLiteral("FINAL");
}

QString formatTime(qint64 epochMs)
{
    if (epochMs <= 0) return QStringLiteral("—");
    return QDateTime::fromMSecsSinceEpoch(epochMs).toLocalTime()
        .toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
}

} // namespace

QString AdminOrder::statusLabel() const
{
    if (status == QString::fromUtf8(protocol::orderStatus::kReserved)) return QStringLiteral("已预约");
    if (status == QString::fromUtf8(protocol::orderStatus::kCharging)) return QStringLiteral("充电中");
    if (status == QString::fromUtf8(protocol::orderStatus::kWaitSettlement)) return QStringLiteral("待支付");
    return QStringLiteral("已完成");
}

QString AdminOrder::paymentStatusLabel() const
{
    if (paymentStatus == QStringLiteral("UNPAID")) return QStringLiteral("待支付");
    if (paymentStatus == QStringLiteral("PAID")) return QStringLiteral("已支付");
    return QStringLiteral("未到结算");
}

QString AdminOrder::billingAmountLabel() const
{
    if (amountKind == QStringLiteral("ESTIMATED")) return QStringLiteral("实时预估");
    if (amountKind == QStringLiteral("FINAL")) return QStringLiteral("最终金额");
    return QStringLiteral("尚未计费");
}

QString AdminOrder::formattedAmount() const
{
    if (amountKind == QStringLiteral("NONE")) return QStringLiteral("—");
    return QStringLiteral("¥ %1%2").arg(amount, 0, 'f', 2)
        .arg(amountKind == QStringLiteral("ESTIMATED") ? QStringLiteral("（预估）") : QString());
}

QString AdminOrder::formattedEnergy() const
{
    if (amountKind == QStringLiteral("NONE")) return QStringLiteral("—");
    return QStringLiteral("%1 kWh").arg(energyKwh, 0, 'f', 2);
}

QString AdminOrder::formattedPrice() const
{
    return QStringLiteral("¥ %1 / 度").arg(pricePerKwh, 0, 'f', 2);
}

QString AdminOrder::formattedDuration() const
{
    const qint64 hours = durationSeconds / 3600;
    const qint64 minutes = (durationSeconds % 3600) / 60;
    const qint64 seconds = durationSeconds % 60;
    if (hours > 0) return QStringLiteral("%1 小时 %2 分").arg(hours).arg(minutes);
    if (minutes > 0) return QStringLiteral("%1 分 %2 秒").arg(minutes).arg(seconds);
    return QStringLiteral("%1 秒").arg(seconds);
}

QString AdminOrder::formattedCreatedAt() const { return formatTime(createdAtEpochMs); }
QString AdminOrder::formattedStartTime() const { return formatTime(startTimeEpochMs); }
QString AdminOrder::formattedStopTime() const { return formatTime(stopTimeEpochMs); }
QString AdminOrder::formattedSettleTime() const { return formatTime(settleTimeEpochMs); }
QString AdminOrder::userLabel() const { return QStringLiteral("%1 · %2").arg(phone, nickname); }
QString AdminOrder::deviceLabel() const
{
    return QStringLiteral("%1 · %2").arg(stationName, chargerCode);
}

bool AdminOrder::fromJson(const QJsonObject& json, AdminOrder* order,
                          QString* errorMessage)
{
    if (!order) return false;
    AdminOrder parsed;
    qint64 type = -1;
    if (!readInteger(json, QStringLiteral("orderId"), &parsed.orderId, true, errorMessage)
        || !readInteger(json, QStringLiteral("userId"), &parsed.userId, true, errorMessage)
        || !readText(json, QStringLiteral("phone"), 20, &parsed.phone, errorMessage)
        || !readText(json, QStringLiteral("nickname"), 40, &parsed.nickname, errorMessage)
        || !readText(json, QStringLiteral("status"), 32, &parsed.status, errorMessage)
        || !validOrderStatus(parsed.status)
        || !readText(json, QStringLiteral("paymentStatus"), 20, &parsed.paymentStatus, errorMessage)
        || !validPaymentStatus(parsed.paymentStatus)
        || !readText(json, QStringLiteral("amountKind"), 20, &parsed.amountKind, errorMessage)
        || !readInteger(json, QStringLiteral("stationId"), &parsed.stationId, true, errorMessage)
        || !readText(json, QStringLiteral("stationName"), 80, &parsed.stationName, errorMessage)
        || !readInteger(json, QStringLiteral("chargerId"), &parsed.chargerId, true, errorMessage)
        || !readText(json, QStringLiteral("chargerCode"), 80, &parsed.chargerCode, errorMessage)
        || !readInteger(json, QStringLiteral("type"), &type, false, errorMessage)
        || (type != protocol::ChargerTypeFast && type != protocol::ChargerTypeSlow)
        || !readNumber(json, QStringLiteral("powerKw"), &parsed.powerKw, errorMessage)
        || !readNumber(json, QStringLiteral("pricePerKwh"), &parsed.pricePerKwh, errorMessage)
        || !readNumber(json, QStringLiteral("energyKwh"), &parsed.energyKwh, errorMessage)
        || !readNumber(json, QStringLiteral("amount"), &parsed.amount, errorMessage)
        || !readTime(json, QStringLiteral("createdAt"), &parsed.createdAtEpochMs, false, errorMessage)
        || !readTime(json, QStringLiteral("startTime"), &parsed.startTimeEpochMs, true, errorMessage)
        || !readTime(json, QStringLiteral("stopTime"), &parsed.stopTimeEpochMs, true, errorMessage)
        || !readTime(json, QStringLiteral("settleTime"), &parsed.settleTimeEpochMs, true, errorMessage)
        || !readInteger(json, QStringLiteral("durationSeconds"), &parsed.durationSeconds, false, errorMessage)) {
        if (errorMessage && errorMessage->isEmpty()) *errorMessage = QStringLiteral("订单字段取值无效");
        return false;
    }
    parsed.chargerType = static_cast<int>(type);
    static const QRegularExpression phonePattern(QStringLiteral("^1\\d{10}$"));
    if (!phonePattern.match(parsed.phone).hasMatch()) {
        if (errorMessage) *errorMessage = QStringLiteral("字段 phone 必须是 11 位手机号");
        return false;
    }
    if (parsed.paymentStatus != expectedPaymentStatus(parsed.status)
        || parsed.amountKind != expectedAmountKind(parsed.status)) {
        if (errorMessage) *errorMessage = QStringLiteral("订单、支付与金额口径不一致");
        return false;
    }
    const bool reserved = parsed.status == QString::fromUtf8(protocol::orderStatus::kReserved);
    const bool charging = parsed.status == QString::fromUtf8(protocol::orderStatus::kCharging);
    const bool waiting = parsed.status == QString::fromUtf8(protocol::orderStatus::kWaitSettlement);
    if ((reserved && (parsed.startTimeEpochMs != 0 || parsed.stopTimeEpochMs != 0
                      || parsed.settleTimeEpochMs != 0 || parsed.durationSeconds != 0))
        || (charging && (parsed.startTimeEpochMs <= 0 || parsed.stopTimeEpochMs != 0
                         || parsed.settleTimeEpochMs != 0))
        || (waiting && (parsed.startTimeEpochMs <= 0 || parsed.stopTimeEpochMs <= 0
                        || parsed.settleTimeEpochMs != 0))
        || (!reserved && !charging && !waiting
            && (parsed.startTimeEpochMs <= 0 || parsed.stopTimeEpochMs <= 0
                || parsed.settleTimeEpochMs <= 0))) {
        if (errorMessage) *errorMessage = QStringLiteral("订单状态与时间字段不一致");
        return false;
    }
    if ((parsed.startTimeEpochMs > 0 && parsed.startTimeEpochMs < parsed.createdAtEpochMs)
        || (parsed.stopTimeEpochMs > 0 && parsed.stopTimeEpochMs < parsed.startTimeEpochMs)
        || (parsed.settleTimeEpochMs > 0 && parsed.settleTimeEpochMs < parsed.stopTimeEpochMs)) {
        if (errorMessage) *errorMessage = QStringLiteral("订单生命周期时间顺序无效");
        return false;
    }
    *order = parsed;
    if (errorMessage) errorMessage->clear();
    return true;
}

bool OrderListQuery::validate(QString* errorMessage) const
{
    if (page < 1 || pageSize < 1 || pageSize > 100) {
        if (errorMessage) *errorMessage = QStringLiteral("订单分页参数无效");
        return false;
    }
    if (keyword.trimmed().size() > 50) {
        if (errorMessage) *errorMessage = QStringLiteral("搜索关键字最多 50 个字符");
        return false;
    }
    if (!validOrderFilter(status) || !validPaymentFilter(paymentStatus)) {
        if (errorMessage) *errorMessage = QStringLiteral("订单筛选条件无效");
        return false;
    }
    if (errorMessage) errorMessage->clear();
    return true;
}

QJsonObject OrderListQuery::toJson() const
{
    return {
        {QStringLiteral("page"), page},
        {QStringLiteral("pageSize"), pageSize},
        {QStringLiteral("keyword"), keyword.trimmed()},
        {QStringLiteral("status"), status},
        {QStringLiteral("paymentStatus"), paymentStatus},
    };
}

QString OrderListPage::formattedGeneratedAt() const { return formatTime(generatedAtEpochMs); }

bool OrderListPage::fromJson(const QJsonObject& json, const OrderListQuery& query,
                             OrderListPage* result, QString* errorMessage)
{
    if (!result) return false;
    qint64 total = 0;
    qint64 page = 0;
    qint64 pageSize = 0;
    qint64 generatedAt = 0;
    if (!readInteger(json, QStringLiteral("total"), &total, false, errorMessage)
        || !readInteger(json, QStringLiteral("page"), &page, true, errorMessage)
        || !readInteger(json, QStringLiteral("pageSize"), &pageSize, true, errorMessage)
        || !readTime(json, QStringLiteral("generatedAt"), &generatedAt, false, errorMessage)
        || page != query.page || pageSize != query.pageSize) {
        if (errorMessage && errorMessage->isEmpty()) *errorMessage = QStringLiteral("订单分页元数据不匹配");
        return false;
    }
    const QJsonValue ordersValue = json.value(QStringLiteral("orders"));
    const QJsonValue summaryValue = json.value(QStringLiteral("platformSummary"));
    if (!ordersValue.isArray() || !summaryValue.isObject()) {
        if (errorMessage) *errorMessage = QStringLiteral("响应缺少 orders 或 platformSummary");
        return false;
    }
    const QJsonArray array = ordersValue.toArray();
    if (array.size() > query.pageSize || total < array.size()) {
        if (errorMessage) *errorMessage = QStringLiteral("订单数量与分页元数据不一致");
        return false;
    }

    OrderListPage parsed;
    parsed.total = total;
    parsed.page = static_cast<int>(page);
    parsed.pageSize = static_cast<int>(pageSize);
    parsed.generatedAtEpochMs = generatedAt;
    QSet<qint64> ids;
    for (int index = 0; index < array.size(); ++index) {
        if (!array.at(index).isObject()) {
            if (errorMessage) *errorMessage = QStringLiteral("第 %1 条订单不是对象").arg(index + 1);
            return false;
        }
        AdminOrder order;
        QString itemError;
        if (!AdminOrder::fromJson(array.at(index).toObject(), &order, &itemError)) {
            if (errorMessage) *errorMessage = QStringLiteral("第 %1 条订单异常：%2").arg(index + 1).arg(itemError);
            return false;
        }
        if (ids.contains(order.orderId)
            || (query.status != QStringLiteral("ALL") && order.status != query.status)
            || (query.paymentStatus != QStringLiteral("ALL")
                && order.paymentStatus != query.paymentStatus)) {
            if (errorMessage) *errorMessage = QStringLiteral("订单结果重复或不符合筛选条件");
            return false;
        }
        const QString searchable = QStringLiteral("%1 %2 %3 %4 %5")
            .arg(QString::number(order.orderId), order.phone, order.nickname,
                 order.stationName, order.chargerCode);
        if (!query.keyword.trimmed().isEmpty()
            && !searchable.contains(query.keyword.trimmed(), Qt::CaseInsensitive)) {
            if (errorMessage) *errorMessage = QStringLiteral("服务端返回了不匹配关键字的订单");
            return false;
        }
        if (!parsed.orders.isEmpty()) {
            const AdminOrder& previous = parsed.orders.constLast();
            if (previous.createdAtEpochMs < order.createdAtEpochMs
                || (previous.createdAtEpochMs == order.createdAtEpochMs
                    && previous.orderId < order.orderId)) {
                if (errorMessage) *errorMessage = QStringLiteral("订单分页顺序不稳定");
                return false;
            }
        }
        ids.insert(order.orderId);
        parsed.orders.append(order);
    }

    const QJsonObject summary = summaryValue.toObject();
    qint64 totalOrders = 0, reserved = 0, chargingCount = 0, waiting = 0, finished = 0;
    if (!readInteger(summary, QStringLiteral("totalOrders"), &totalOrders, false, errorMessage)
        || !readInteger(summary, QStringLiteral("reservedCount"), &reserved, false, errorMessage)
        || !readInteger(summary, QStringLiteral("chargingCount"), &chargingCount, false, errorMessage)
        || !readInteger(summary, QStringLiteral("waitSettlementCount"), &waiting, false, errorMessage)
        || !readInteger(summary, QStringLiteral("finishedCount"), &finished, false, errorMessage)
        || !readNumber(summary, QStringLiteral("paidAmount"), &parsed.platformSummary.paidAmount, errorMessage)
        || totalOrders != reserved + chargingCount + waiting + finished
        || total > totalOrders) {
        if (errorMessage && errorMessage->isEmpty()) *errorMessage = QStringLiteral("平台订单汇总不一致");
        return false;
    }
    parsed.platformSummary.totalOrders = totalOrders;
    parsed.platformSummary.reservedCount = reserved;
    parsed.platformSummary.chargingCount = chargingCount;
    parsed.platformSummary.waitSettlementCount = waiting;
    parsed.platformSummary.finishedCount = finished;
    *result = parsed;
    if (errorMessage) errorMessage->clear();
    return true;
}
