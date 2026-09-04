#pragma once

// ============================================================================
// RequestDispatcher：action → handler 的路由表 + 通用拦截（登录检查）
//
// Commit 0 实现：
// - 只注册 PING handler
// - 其他 action 在后续 commit 补充（Commit 1: 用户登录/充值等）
// ============================================================================

#include <QDateTime>
#include <QHash>
#include <QObject>

#include "net/TcpConnection.h"
#include "protocol/Protocol.h"

namespace net {

class RequestDispatcher : public QObject {
    Q_OBJECT

    using Handler = void (RequestDispatcher::*)(const protocol::Request&, TcpConnection*);

public:
    explicit RequestDispatcher(QObject* parent = nullptr)
        : QObject(parent)
    {
        registerHandler(protocol::action::kPing, &RequestDispatcher::handlePing);
        // Commit 1+ 会在这里注册 user.login、user.recharge、station.nearby 等
    }

    void dispatch(const protocol::Request& request, TcpConnection* connection)
    {
        const QString& action = request.action;

        if (!m_handlers.contains(action)) {
            qWarning() << "[Dispatcher] unknown action" << action
                       << "from" << connection->peerAddress();
            connection->sendResponse(protocol::buildErrorResponse(
                request.requestId, protocol::CodeBadRequest,
                QStringLiteral("unknown action: ") + action));
            return;
        }

        // Commit 1 会在这里加登录拦截：
        // if (!isPublicAction(action) && !connection->isUserLoggedIn() && !connection->isAdminLoggedIn()) {
        //     connection->sendResponse(protocol::buildErrorResponse(..., CodeNotLoggedIn, ...));
        //     return;
        // }

        (this->*m_handlers[action])(request, connection);
    }

private:
    void registerHandler(const QString& action, Handler handler)
    {
        m_handlers[action] = handler;
    }

    // ========================================================================
    // Action handlers（Commit 0 只有 PING，其他在后续 commit）
    // ========================================================================

    void handlePing(const protocol::Request& request, TcpConnection* connection)
    {
        QJsonObject data;
        data.insert(QStringLiteral("timestamp"),
                    QString::number(QDateTime::currentMSecsSinceEpoch()));
        data.insert(QStringLiteral("message"), QStringLiteral("pong"));

        connection->sendResponse(protocol::buildSuccessResponse(request.requestId, data));
    }

    // Commit 1+ handlers:
    // - handleUserLogin
    // - handleUserProfileUpdate
    // - handleUserRecharge
    // - handleStationNearby
    // - handleStationDetail
    // - handleOrderReserve
    // - handleOrderStart
    // - handleOrderStop
    // - handleOrderSettle
    // - handleOrderHistory
    // - handleOrderActive
    // - handleOrderStatus
    //
    // Commit 6 admin handlers:
    // - handleAdminLogin
    // - handleAdminUsersList
    // - handleAdminUsersFreeze
    // - handleAdminStationsList
    // - handleAdminStationsCreate
    // - handleAdminChargersList
    // - handleAdminChargersRestart
    // - handleAdminRevenueSummary
    // - handleAdminRevenueTrend
    // - handleAdminChargersStats

private:
    QHash<QString, Handler> m_handlers;
};

} // namespace net
