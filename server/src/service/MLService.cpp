#include "MLService.h"
#include "repository/OrderRepository.h"
#include "repository/ChargerRepository.h"
#include "repository/StationRepository.h"
#include "protocol/ProtocolHelper.h"
#include <QJsonArray>
#include <QDebug>
#include <QDateTime>
#include <QRegularExpression>

namespace service {

MLService::MLService(QObject* parent)
    : QObject(parent)
{
}

QJsonObject MLService::handleOrdersExport(const QJsonObject& data, QTcpSocket* socket) {
    Q_UNUSED(socket)

    // 提取参数
    QString startDate = data.value("startDate").toString();
    QString endDate = data.value("endDate").toString();
    QString format = data.value("format").toString("json");

    // 验证日期格式
    if (!isValidDateFormat(startDate) || !isValidDateFormat(endDate)) {
        return protocol::makeErrorResponse(protocol::CodeBadRequest,
            "日期格式错误，请使用 YYYY-MM-DD 格式");
    }

    // 解析时间戳
    qint64 startTimestamp = parseDateToTimestamp(startDate);
    qint64 endTimestamp = parseDateToTimestamp(endDate);

    if (startTimestamp >= endTimestamp) {
        return protocol::makeErrorResponse(protocol::CodeBadRequest,
            "开始日期必须早于结束日期");
    }

    // 检查时间范围（最大1年）
    qint64 oneYear = 365LL * 24 * 60 * 60 * 1000;
    if (endTimestamp - startTimestamp > oneYear) {
        return protocol::makeErrorResponse(protocol::CodeBadRequest,
            "时间范围不能超过1年");
    }

    // 查询已完成的订单
    auto orders = repository::OrderRepository::findFinishedInRange(
        startTimestamp,
        endTimestamp
    );

    qInfo() << "[MLService] Exporting" << orders.size() << "finished orders from"
            << startDate << "to" << endDate;

    // 构建导出数据
    QJsonArray ordersArray;

    for (const auto& order : orders) {
        // 查询充电桩信息以获取类型和功率
        auto chargerOpt = repository::ChargerRepository::findById(order.chargerId);
        if (!chargerOpt.has_value()) {
            qWarning() << "[MLService] Charger not found:" << order.chargerId;
            continue;
        }

        const auto& charger = chargerOpt.value();

        QJsonObject orderObj;
        orderObj["orderId"] = order.orderId;
        orderObj["userId"] = order.userId;
        orderObj["stationId"] = order.stationId;
        orderObj["chargerId"] = order.chargerId;
        orderObj["chargerType"] = charger.type;  // 0=快充, 1=慢充
        orderObj["power"] = charger.powerKw;

        // 时间字段（ISO 8601格式）
        // 注意：order.createdAt对应预约时间
        orderObj["reservedAt"] = QDateTime::fromMSecsSinceEpoch(order.createdAt)
            .toString(Qt::ISODate);

        if (order.startTime > 0) {
            orderObj["startedAt"] = QDateTime::fromMSecsSinceEpoch(order.startTime)
                .toString(Qt::ISODate);
        } else {
            orderObj["startedAt"] = QJsonValue(QJsonValue::Null);
        }

        if (order.stopTime > 0) {
            orderObj["stoppedAt"] = QDateTime::fromMSecsSinceEpoch(order.stopTime)
                .toString(Qt::ISODate);
        } else {
            orderObj["stoppedAt"] = QJsonValue(QJsonValue::Null);
        }

        // 充电时长（分钟）- 数据库已存储duration字段（秒）
        int durationMinutes = order.duration / 60;
        orderObj["duration"] = durationMinutes;

        orderObj["totalKwh"] = order.energyKwh;
        orderObj["totalCost"] = order.amount;
        orderObj["status"] = order.status;  // FINISHED

        ordersArray.append(orderObj);
    }

    QJsonObject result;
    result["orders"] = ordersArray;
    result["count"] = ordersArray.size();
    result["startDate"] = startDate;
    result["endDate"] = endDate;
    result["exportedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    return protocol::makeSuccessResponse(result);
}

bool MLService::isValidDateFormat(const QString& date) const {
    // 验证格式：YYYY-MM-DD
    QRegularExpression regex(R"(^\d{4}-\d{2}-\d{2}$)");
    return regex.match(date).hasMatch();
}

qint64 MLService::parseDateToTimestamp(const QString& date) const {
    // 解析日期字符串（00:00:00 UTC）
    QDateTime dt = QDateTime::fromString(date, "yyyy-MM-dd");
    dt.setTimeSpec(Qt::UTC);

    if (!dt.isValid()) {
        return 0;
    }

    return dt.toMSecsSinceEpoch();
}

} // namespace service
