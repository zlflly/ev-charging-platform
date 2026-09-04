#include "OrderService.h"
#include "repository/OrderRepository.h"
#include "repository/ChargerRepository.h"
#include "repository/UserRepository.h"
#include "repository/StationRepository.h"
#include "net/SessionManager.h"
#include "protocol/ProtocolHelper.h"
#include <QDebug>
#include <QDateTime>

namespace service {

OrderService::OrderService(QObject* parent)
    : QObject(parent)
{
}

QJsonObject OrderService::handleActive(const QJsonObject& data, QTcpSocket* socket) {
    Q_UNUSED(data)

    int userId = net::SessionManager::instance().getUserId(socket);

    // 查询当前用户的活跃订单
    auto orderOpt = repository::OrderRepository::findActiveByUser(userId);

    QJsonObject result;

    if (!orderOpt.has_value()) {
        // 无活跃订单
        result["order"] = QJsonValue(QJsonValue::Null);
    } else {
        result["order"] = buildOrderJson(orderOpt.value());
    }

    return protocol::makeSuccessResponse(result);
}

QJsonObject OrderService::handleReserve(const QJsonObject& data, QTcpSocket* socket) {
    int userId = net::SessionManager::instance().getUserId(socket);

    // 提取充电桩ID
    int chargerId = data.value("chargerId").toInt(0);

    if (chargerId <= 0) {
        return protocol::makeErrorResponse(protocol::CodeBadRequest, "充电桩ID非法");
    }

    // 检查是否已有未完成订单
    auto existingOrder = repository::OrderRepository::findActiveByUser(userId);
    if (existingOrder.has_value()) {
        return protocol::makeErrorResponse(protocol::CodeOrderConflict,
            "您已有进行中的订单，请先完成或取消");
    }

    // 查询充电桩信息
    auto chargerOpt = repository::ChargerRepository::findById(chargerId);
    if (!chargerOpt.has_value()) {
        return protocol::makeErrorResponse(protocol::CodeBadRequest, "充电桩不存在");
    }

    const auto& charger = chargerOpt.value();

    // 检查桩状态（只有空闲才能预约）
    if (charger.status != protocol::ChargerStatusIdle) {
        return protocol::makeErrorResponse(protocol::CodeChargerUnavailable,
            "该充电桩不可用（被占用、故障或离线）");
    }

    // 查询站点信息
    auto stationOpt = repository::StationRepository::findById(charger.stationId);
    if (!stationOpt.has_value()) {
        return protocol::makeErrorResponse(protocol::CodeServerError, "站点信息查询失败");
    }

    const auto& station = stationOpt.value();

    // 创建订单
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    auto newOrderOpt = repository::OrderRepository::create(
        userId,
        charger.stationId,
        chargerId,
        now
    );

    if (!newOrderOpt.has_value()) {
        return protocol::makeErrorResponse(protocol::CodeServerError, "订单创建失败");
    }

    // 更新桩状态为充电中（预约即占用）
    if (!repository::ChargerRepository::updateStatus(chargerId, protocol::ChargerStatusCharging)) {
        qWarning() << "[OrderService] Failed to update charger status after reserve";
    }

    qInfo() << "[OrderService] Order reserved: orderId=" << newOrderOpt.value().orderId
            << "userId=" << userId << "chargerId=" << chargerId;

    QJsonObject result;
    result["order"] = buildOrderJson(newOrderOpt.value());

    return protocol::makeSuccessResponse(result);
}

QJsonObject OrderService::handleStart(const QJsonObject& data, QTcpSocket* socket) {
    int userId = net::SessionManager::instance().getUserId(socket);

    // 提取订单ID
    int orderId = data.value("orderId").toInt(0);

    if (orderId <= 0) {
        return protocol::makeErrorResponse(protocol::CodeBadRequest, "订单ID非法");
    }

    // 查询订单
    auto orderOpt = repository::OrderRepository::findById(orderId);
    if (!orderOpt.has_value()) {
        return protocol::makeErrorResponse(protocol::CodeBadRequest, "订单不存在");
    }

    auto order = orderOpt.value();

    // 检查订单归属
    if (order.userId != userId) {
        return protocol::makeErrorResponse(protocol::CodeBadRequest, "无权操作此订单");
    }

    // 检查订单状态（只有RESERVED才能start）
    if (order.status != protocol::kOrderStatusReserved) {
        return protocol::makeErrorResponse(protocol::CodeOrderConflict,
            "订单状态不允许开始充电");
    }

    // 开始充电：更新订单状态和开始时间
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    if (!repository::OrderRepository::updateStatus(orderId, protocol::kOrderStatusCharging, now, 0, 0)) {
        return protocol::makeErrorResponse(protocol::CodeServerError, "订单状态更新失败");
    }

    // 重新查询订单
    orderOpt = repository::OrderRepository::findById(orderId);
    if (!orderOpt.has_value()) {
        return protocol::makeErrorResponse(protocol::CodeServerError, "订单查询失败");
    }

    qInfo() << "[OrderService] Charging started: orderId=" << orderId << "userId=" << userId;

    QJsonObject result;
    result["order"] = buildOrderJson(orderOpt.value());

    return protocol::makeSuccessResponse(result);
}

QJsonObject OrderService::handleStatus(const QJsonObject& data, QTcpSocket* socket) {
    int userId = net::SessionManager::instance().getUserId(socket);

    // 提取订单ID
    int orderId = data.value("orderId").toInt(0);

    if (orderId <= 0) {
        return protocol::makeErrorResponse(protocol::CodeBadRequest, "订单ID非法");
    }

    // 查询订单
    auto orderOpt = repository::OrderRepository::findById(orderId);
    if (!orderOpt.has_value()) {
        return protocol::makeErrorResponse(protocol::CodeBadRequest, "订单不存在");
    }

    auto order = orderOpt.value();

    // 检查订单归属
    if (order.userId != userId) {
        return protocol::makeErrorResponse(protocol::CodeBadRequest, "无权查询此订单");
    }

    // 如果是CHARGING状态，计算实时电量和估算金额
    if (order.status == protocol::kOrderStatusCharging && order.startTime > 0) {
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        qint64 elapsedMs = now - order.startTime;
        double elapsedHours = elapsedMs / 3600000.0;

        // 模拟充电：假设以桩功率的80%充电
        auto chargerOpt = repository::ChargerRepository::findById(order.chargerId);
        if (chargerOpt.has_value()) {
            double powerKw = chargerOpt.value().powerKw;
            order.energyKwh = elapsedHours * powerKw * 0.8;

            // 查询站点价格
            auto stationOpt = repository::StationRepository::findById(order.stationId);
            if (stationOpt.has_value()) {
                order.amount = order.energyKwh * stationOpt.value().pricePerKwh;
            }
        }
    }

    QJsonObject result;
    result["order"] = buildOrderJson(order);

    return protocol::makeSuccessResponse(result);
}

QJsonObject OrderService::handleStop(const QJsonObject& data, QTcpSocket* socket) {
    int userId = net::SessionManager::instance().getUserId(socket);

    // 提取订单ID
    int orderId = data.value("orderId").toInt(0);

    if (orderId <= 0) {
        return protocol::makeErrorResponse(protocol::CodeBadRequest, "订单ID非法");
    }

    // 查询订单
    auto orderOpt = repository::OrderRepository::findById(orderId);
    if (!orderOpt.has_value()) {
        return protocol::makeErrorResponse(protocol::CodeBadRequest, "订单不存在");
    }

    auto order = orderOpt.value();

    // 检查订单归属
    if (order.userId != userId) {
        return protocol::makeErrorResponse(protocol::CodeBadRequest, "无权操作此订单");
    }

    // 幂等：如果已经是WAIT_SETTLEMENT，直接返回
    if (order.status == protocol::kOrderStatusWaitSettlement) {
        qInfo() << "[OrderService] Order already stopped (idempotent): orderId=" << orderId;
        QJsonObject result;
        result["order"] = buildOrderJson(order);
        return protocol::makeSuccessResponse(result);
    }

    // 检查订单状态（只有CHARGING才能stop）
    if (order.status != protocol::kOrderStatusCharging) {
        return protocol::makeErrorResponse(protocol::CodeOrderConflict,
            "订单状态不允许停止充电");
    }

    // 计算最终电量和金额
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    qint64 elapsedMs = now - order.startTime;
    double elapsedHours = elapsedMs / 3600000.0;

    double energyKwh = 0.0;
    double amount = 0.0;

    auto chargerOpt = repository::ChargerRepository::findById(order.chargerId);
    if (chargerOpt.has_value()) {
        double powerKw = chargerOpt.value().powerKw;
        energyKwh = elapsedHours * powerKw * 0.8;

        auto stationOpt = repository::StationRepository::findById(order.stationId);
        if (stationOpt.has_value()) {
            amount = energyKwh * stationOpt.value().pricePerKwh;
            // 四舍五入到分
            amount = qRound(amount * 100) / 100.0;
        }
    }

    // 更新订单：CHARGING → WAIT_SETTLEMENT
    if (!repository::OrderRepository::updateStatus(orderId, protocol::kOrderStatusWaitSettlement,
                                                   order.startTime, now, 0)) {
        return protocol::makeErrorResponse(protocol::CodeServerError, "订单状态更新失败");
    }

    // 更新电量和金额
    if (!repository::OrderRepository::updateEnergyAndAmount(orderId, energyKwh, amount)) {
        qWarning() << "[OrderService] Failed to update energy and amount";
    }

    // 重新查询订单
    orderOpt = repository::OrderRepository::findById(orderId);
    if (!orderOpt.has_value()) {
        return protocol::makeErrorResponse(protocol::CodeServerError, "订单查询失败");
    }

    qInfo() << "[OrderService] Charging stopped: orderId=" << orderId
            << "energyKwh=" << energyKwh << "amount=" << amount;

    QJsonObject result;
    result["order"] = buildOrderJson(orderOpt.value());

    return protocol::makeSuccessResponse(result);
}

QJsonObject OrderService::handleSettle(const QJsonObject& data, QTcpSocket* socket) {
    int userId = net::SessionManager::instance().getUserId(socket);

    // 提取订单ID
    int orderId = data.value("orderId").toInt(0);

    if (orderId <= 0) {
        return protocol::makeErrorResponse(protocol::CodeBadRequest, "订单ID非法");
    }

    // 查询订单
    auto orderOpt = repository::OrderRepository::findById(orderId);
    if (!orderOpt.has_value()) {
        return protocol::makeErrorResponse(protocol::CodeBadRequest, "订单不存在");
    }

    auto order = orderOpt.value();

    // 检查订单归属
    if (order.userId != userId) {
        return protocol::makeErrorResponse(protocol::CodeBadRequest, "无权操作此订单");
    }

    // 检查订单状态（只有WAIT_SETTLEMENT才能settle）
    if (order.status != protocol::kOrderStatusWaitSettlement) {
        return protocol::makeErrorResponse(protocol::CodeOrderConflict,
            "订单状态不允许结算");
    }

    // 查询用户余额
    auto userOpt = repository::UserRepository::findById(userId);
    if (!userOpt.has_value()) {
        return protocol::makeErrorResponse(protocol::CodeServerError, "用户信息查询失败");
    }

    double balance = userOpt.value().balance;
    double amount = order.amount;

    // 检查余额是否充足
    if (balance < amount) {
        double deficit = amount - balance;
        QJsonObject result;
        result["deficit"] = deficit;

        QJsonObject response = protocol::makeErrorResponse(
            protocol::CodeBalanceInsufficient,
            QString("余额不足，还需充值%1元").arg(deficit, 0, 'f', 2)
        );
        response["data"] = result;
        return response;
    }

    // 扣款
    if (!repository::UserRepository::updateBalance(userId, -amount)) {
        return protocol::makeErrorResponse(protocol::CodeServerError, "扣款失败");
    }

    // 更新订单状态：WAIT_SETTLEMENT → FINISHED
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (!repository::OrderRepository::updateStatus(orderId, protocol::kOrderStatusFinished,
                                                   order.startTime, order.stopTime, now)) {
        // 扣款已成功但订单状态更新失败，需要回滚（这里简化处理）
        qCritical() << "[OrderService] Order status update failed after deduction!";
        return protocol::makeErrorResponse(protocol::CodeServerError, "订单状态更新失败");
    }

    // 释放充电桩（恢复为空闲）
    if (!repository::ChargerRepository::updateStatus(order.chargerId, protocol::ChargerStatusIdle)) {
        qWarning() << "[OrderService] Failed to release charger after settlement";
    }

    // 重新查询订单和余额
    orderOpt = repository::OrderRepository::findById(orderId);
    userOpt = repository::UserRepository::findById(userId);

    if (!orderOpt.has_value() || !userOpt.has_value()) {
        return protocol::makeErrorResponse(protocol::CodeServerError, "查询失败");
    }

    qInfo() << "[OrderService] Order settled: orderId=" << orderId
            << "amount=" << amount << "newBalance=" << userOpt.value().balance;

    QJsonObject result;
    result["order"] = buildOrderJson(orderOpt.value());
    result["balance"] = userOpt.value().balance;

    return protocol::makeSuccessResponse(result);
}

QJsonObject OrderService::handleHistory(const QJsonObject& data, QTcpSocket* socket) {
    int userId = net::SessionManager::instance().getUserId(socket);

    // 提取分页参数（可选）
    int limit = data.value("limit").toInt(20);
    int offset = data.value("offset").toInt(0);

    if (limit <= 0 || limit > 100) {
        limit = 20;
    }

    // 查询历史订单（已完成的订单）
    QVector<repository::Order> orders = repository::OrderRepository::findByUser(userId);

    // 过滤出已完成订单
    QVector<repository::Order> finishedOrders;
    for (const auto& order : orders) {
        if (order.status == protocol::kOrderStatusFinished) {
            finishedOrders.append(order);
        }
    }

    // 按订单ID降序排序（最新的在前）
    std::sort(finishedOrders.begin(), finishedOrders.end(),
              [](const repository::Order& a, const repository::Order& b) {
        return a.orderId > b.orderId;
    });

    // 分页
    int total = finishedOrders.size();
    int start = qMin(offset, total);
    int end = qMin(offset + limit, total);

    QJsonArray ordersArray;
    for (int i = start; i < end; ++i) {
        ordersArray.append(buildOrderJson(finishedOrders[i]));
    }

    // 统计信息
    double totalEnergy = 0.0;
    double totalAmount = 0.0;
    for (const auto& order : finishedOrders) {
        totalEnergy += order.energyKwh;
        totalAmount += order.amount;
    }

    QJsonObject summary;
    summary["totalOrders"] = finishedOrders.size();
    summary["totalEnergy"] = totalEnergy;
    summary["totalAmount"] = totalAmount;

    QJsonObject result;
    result["orders"] = ordersArray;
    result["total"] = total;
    result["summary"] = summary;

    qInfo() << "[OrderService] Order history: userId=" << userId << "total=" << total;

    return protocol::makeSuccessResponse(result);
}

QJsonObject OrderService::buildOrderJson(const repository::Order& order) const {
    QJsonObject obj;
    obj["orderId"] = order.orderId;
    obj["status"] = order.status;
    obj["chargerId"] = order.chargerId;
    obj["stationId"] = order.stationId;
    obj["startTime"] = order.startTime;
    obj["stopTime"] = order.stopTime;
    obj["settleTime"] = order.settleTime;
    obj["energyKwh"] = order.energyKwh;
    obj["amount"] = order.amount;

    // 查询充电桩信息
    auto chargerOpt = repository::ChargerRepository::findById(order.chargerId);
    if (chargerOpt.has_value()) {
        obj["chargerCode"] = chargerOpt.value().code;
        obj["type"] = chargerOpt.value().type;
        obj["powerKw"] = chargerOpt.value().powerKw;
    }

    // 查询站点信息
    auto stationOpt = repository::StationRepository::findById(order.stationId);
    if (stationOpt.has_value()) {
        obj["stationName"] = stationOpt.value().name;
        obj["pricePerKwh"] = stationOpt.value().pricePerKwh;
    }

    return obj;
}

} // namespace service
