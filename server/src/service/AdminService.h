#pragma once

// ============================================================================
// AdminService：管理员端业务逻辑
//
// Commit 6 实现：管理员登录、充电桩管理、站点管理、用户管理、营收统计、订单查询
// ============================================================================

#include <QJsonObject>
#include <QObject>
#include <QTcpSocket>

namespace service {

class AdminService : public QObject {
    Q_OBJECT

public:
    explicit AdminService(QObject* parent = nullptr);

    // 管理员认证
    QJsonObject handleLogin(const QJsonObject& data, QTcpSocket* socket);

    // 充电桩管理
    QJsonObject handleChargerOverview(const QJsonObject& data, QTcpSocket* socket);
    QJsonObject handleChargersList(const QJsonObject& data, QTcpSocket* socket);
    QJsonObject handleChargersRestart(const QJsonObject& data, QTcpSocket* socket);
    QJsonObject handleChargersStatusUpdate(const QJsonObject& data, QTcpSocket* socket);

    // 充电站管理
    QJsonObject handleStationsList(const QJsonObject& data, QTcpSocket* socket);
    QJsonObject handleStationsCreate(const QJsonObject& data, QTcpSocket* socket);
    QJsonObject handleStationsUpdate(const QJsonObject& data, QTcpSocket* socket);

    // 用户管理
    QJsonObject handleUsersList(const QJsonObject& data, QTcpSocket* socket);
    QJsonObject handleUsersFreeze(const QJsonObject& data, QTcpSocket* socket);

    // 营收统计
    QJsonObject handleRevenueSummary(const QJsonObject& data, QTcpSocket* socket);
    QJsonObject handleRevenueTrend(const QJsonObject& data, QTcpSocket* socket);

    // 订单查询
    QJsonObject handleOrdersList(const QJsonObject& data, QTcpSocket* socket);
};

} // namespace service
