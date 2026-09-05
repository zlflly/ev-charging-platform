#ifndef ORDER_SERVICE_H
#define ORDER_SERVICE_H

#include <QObject>
#include <QJsonObject>
#include <QTcpSocket>
#include "repository/OrderRepository.h"

namespace service {

/**
 * 订单业务服务层
 * 处理 order.* 相关的 action
 */
class OrderService : public QObject {
    Q_OBJECT

public:
    explicit OrderService(QObject* parent = nullptr);

    // order.active: 查询当前活跃订单（未完成的订单）
    QJsonObject handleActive(const QJsonObject& data, QTcpSocket* socket);

    // order.reserve: 预约充电桩
    QJsonObject handleReserve(const QJsonObject& data, QTcpSocket* socket);

    // order.start: 开始充电
    QJsonObject handleStart(const QJsonObject& data, QTcpSocket* socket);

    // order.status: 查询订单实时状态（CHARGING时返回实时电量和估算金额）
    QJsonObject handleStatus(const QJsonObject& data, QTcpSocket* socket);

    // order.stop: 停止充电（幂等）
    QJsonObject handleStop(const QJsonObject& data, QTcpSocket* socket);

    // order.settle: 结算订单
    QJsonObject handleSettle(const QJsonObject& data, QTcpSocket* socket);

    // order.history: 历史订单列表
    QJsonObject handleHistory(const QJsonObject& data, QTcpSocket* socket);

private:
    // 构造订单JSON对象（包含关联的站点和桩信息）
    QJsonObject buildOrderJson(const repository::Order& order) const;
};

} // namespace service

#endif // ORDER_SERVICE_H
