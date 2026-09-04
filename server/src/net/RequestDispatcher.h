#pragma once

// ============================================================================
// RequestDispatcher：action → handler 的路由表 + 通用拦截（登录检查）
//
// Commit 0 实现：PING handler
// Commit 3 实现：user.login / user.profile.update / user.recharge
// ============================================================================

#include <QDateTime>
#include <QHash>
#include <QObject>

#include "net/TcpConnection.h"
#include "net/SessionManager.h"
#include "protocol/Protocol.h"
#include "service/UserService.h"
#include "service/StationService.h"
#include "service/OrderService.h"

namespace net {

class RequestDispatcher : public QObject {
    Q_OBJECT

    using Handler = void (RequestDispatcher::*)(const protocol::Request&, TcpConnection*);

public:
    explicit RequestDispatcher(QObject* parent = nullptr)
        : QObject(parent)
        , m_userService(new service::UserService(this))
        , m_stationService(new service::StationService(this))
        , m_orderService(new service::OrderService(this))
    {
        // 公共接口
        registerHandler(protocol::action::kPing, &RequestDispatcher::handlePing);

        // 用户端接口（Commit 3）
        registerHandler(protocol::action::kUserLogin, &RequestDispatcher::handleUserLogin);
        registerHandler(protocol::action::kUserProfileUpdate, &RequestDispatcher::handleUserProfileUpdate);
        registerHandler(protocol::action::kUserRecharge, &RequestDispatcher::handleUserRecharge);

        // 站点查询接口（Commit 4）
        registerHandler(protocol::action::kStationNearby, &RequestDispatcher::handleStationNearby);
        registerHandler(protocol::action::kStationDetail, &RequestDispatcher::handleStationDetail);

        // 订单接口（Commit 5）
        registerHandler(protocol::action::kOrderActive, &RequestDispatcher::handleOrderActive);
        registerHandler(protocol::action::kOrderReserve, &RequestDispatcher::handleOrderReserve);
        registerHandler(protocol::action::kOrderStart, &RequestDispatcher::handleOrderStart);
        registerHandler(protocol::action::kOrderStatus, &RequestDispatcher::handleOrderStatus);
        registerHandler(protocol::action::kOrderStop, &RequestDispatcher::handleOrderStop);
        registerHandler(protocol::action::kOrderSettle, &RequestDispatcher::handleOrderSettle);
        registerHandler(protocol::action::kOrderHistory, &RequestDispatcher::handleOrderHistory);

        // 后续 Commit 补充：
        // Commit 6: admin.*
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

        // 登录拦截：某些 action 需要登录状态
        if (requiresAuth(action) && !SessionManager::instance().isUserLoggedIn(connection->socket())
            && !SessionManager::instance().isAdminLoggedIn(connection->socket())) {
            qWarning() << "[Dispatcher] action" << action << "requires login";
            connection->sendResponse(protocol::buildErrorResponse(
                request.requestId, protocol::CodeNotLoggedIn,
                QStringLiteral("请先登录")));
            return;
        }

        (this->*m_handlers[action])(request, connection);
    }

private:
    void registerHandler(const QString& action, Handler handler)
    {
        m_handlers[action] = handler;
    }

    // 判断 action 是否需要登录
    bool requiresAuth(const QString& action) const
    {
        // 公共接口无需登录
        if (action == protocol::action::kPing) {
            return false;
        }

        // user.login 无需登录（登录接口本身）
        if (action == protocol::action::kUserLogin) {
            return false;
        }

        // admin.login 无需登录
        if (action == protocol::action::kAdminLogin) {
            return false;
        }

        // 其他 user.* 和 order.* 需要登录
        if (action.startsWith("user.") || action.startsWith("order.")) {
            return true;
        }

        // admin.* 需要管理员登录（后续 Commit 6 补充细粒度检查）
        if (action.startsWith("admin.")) {
            return true;
        }

        // station.* 不需要登录（公开查询）
        return false;
    }

    // ========================================================================
    // Action handlers
    // ========================================================================

    void handlePing(const protocol::Request& request, TcpConnection* connection)
    {
        QJsonObject data;
        data.insert(QStringLiteral("timestamp"),
                    QString::number(QDateTime::currentMSecsSinceEpoch()));
        data.insert(QStringLiteral("message"), QStringLiteral("pong"));

        connection->sendResponse(protocol::buildSuccessResponse(request.requestId, data));
    }

    // Commit 3: 用户业务接口

    void handleUserLogin(const protocol::Request& request, TcpConnection* connection)
    {
        QJsonObject response = m_userService->handleLogin(request.data, connection->socket());
        connection->sendResponse(protocol::buildResponse(request.requestId, response));
    }

    void handleUserProfileUpdate(const protocol::Request& request, TcpConnection* connection)
    {
        QJsonObject response = m_userService->handleProfileUpdate(request.data, connection->socket());
        connection->sendResponse(protocol::buildResponse(request.requestId, response));
    }

    void handleUserRecharge(const protocol::Request& request, TcpConnection* connection)
    {
        QJsonObject response = m_userService->handleRecharge(request.data, connection->socket());
        connection->sendResponse(protocol::buildResponse(request.requestId, response));
    }

    // Commit 4: 站点查询接口

    void handleStationNearby(const protocol::Request& request, TcpConnection* connection)
    {
        QJsonObject response = m_stationService->handleNearby(request.data, connection->socket());
        connection->sendResponse(protocol::buildResponse(request.requestId, response));
    }

    void handleStationDetail(const protocol::Request& request, TcpConnection* connection)
    {
        QJsonObject response = m_stationService->handleDetail(request.data, connection->socket());
        connection->sendResponse(protocol::buildResponse(request.requestId, response));
    }

    // Commit 5: 订单接口

    void handleOrderActive(const protocol::Request& request, TcpConnection* connection)
    {
        QJsonObject response = m_orderService->handleActive(request.data, connection->socket());
        connection->sendResponse(protocol::buildResponse(request.requestId, response));
    }

    void handleOrderReserve(const protocol::Request& request, TcpConnection* connection)
    {
        QJsonObject response = m_orderService->handleReserve(request.data, connection->socket());
        connection->sendResponse(protocol::buildResponse(request.requestId, response));
    }

    void handleOrderStart(const protocol::Request& request, TcpConnection* connection)
    {
        QJsonObject response = m_orderService->handleStart(request.data, connection->socket());
        connection->sendResponse(protocol::buildResponse(request.requestId, response));
    }

    void handleOrderStatus(const protocol::Request& request, TcpConnection* connection)
    {
        QJsonObject response = m_orderService->handleStatus(request.data, connection->socket());
        connection->sendResponse(protocol::buildResponse(request.requestId, response));
    }

    void handleOrderStop(const protocol::Request& request, TcpConnection* connection)
    {
        QJsonObject response = m_orderService->handleStop(request.data, connection->socket());
        connection->sendResponse(protocol::buildResponse(request.requestId, response));
    }

    void handleOrderSettle(const protocol::Request& request, TcpConnection* connection)
    {
        QJsonObject response = m_orderService->handleSettle(request.data, connection->socket());
        connection->sendResponse(protocol::buildResponse(request.requestId, response));
    }

    void handleOrderHistory(const protocol::Request& request, TcpConnection* connection)
    {
        QJsonObject response = m_orderService->handleHistory(request.data, connection->socket());
        connection->sendResponse(protocol::buildResponse(request.requestId, response));
    }

private:
    QHash<QString, Handler> m_handlers;
    service::UserService* m_userService;
    service::StationService* m_stationService;
    service::OrderService* m_orderService;
};

} // namespace net
