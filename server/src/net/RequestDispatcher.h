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
#include "service/AdminService.h"
#include "service/MLService.h"

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
        , m_adminService(new service::AdminService(this))
        , m_mlService(new service::MLService(this))
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

        // 管理员端接口（Commit 6）
        registerHandler(protocol::action::kAdminLogin, &RequestDispatcher::handleAdminLogin);
        registerHandler(protocol::action::kAdminChargerOverview, &RequestDispatcher::handleAdminChargerOverview);
        registerHandler(protocol::action::kAdminChargersList, &RequestDispatcher::handleAdminChargersList);
        registerHandler(protocol::action::kAdminChargersRestart, &RequestDispatcher::handleAdminChargersRestart);
        registerHandler(protocol::action::kAdminChargersStatusUpdate, &RequestDispatcher::handleAdminChargersStatusUpdate);
        registerHandler(protocol::action::kAdminStationsList, &RequestDispatcher::handleAdminStationsList);
        registerHandler(protocol::action::kAdminStationsCreate, &RequestDispatcher::handleAdminStationsCreate);
        registerHandler(protocol::action::kAdminStationsUpdate, &RequestDispatcher::handleAdminStationsUpdate);
        registerHandler(protocol::action::kAdminUsersList, &RequestDispatcher::handleAdminUsersList);
        registerHandler(protocol::action::kAdminUsersFreeze, &RequestDispatcher::handleAdminUsersFreeze);
        registerHandler(protocol::action::kAdminRevenueSummary, &RequestDispatcher::handleAdminRevenueSummary);
        registerHandler(protocol::action::kAdminRevenueTrend, &RequestDispatcher::handleAdminRevenueTrend);
        registerHandler(protocol::action::kAdminOrdersList, &RequestDispatcher::handleAdminOrdersList);

        // 机器学习数据接口（Commit 7）
        registerHandler(protocol::action::kMLOrdersExport, &RequestDispatcher::handleMLOrdersExport);
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

        // ml.* 不需要登录（供成员4访问）
        if (action.startsWith("ml.")) {
            return false;
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

    // Commit 6: 管理员接口

    void handleAdminLogin(const protocol::Request& request, TcpConnection* connection)
    {
        QJsonObject response = m_adminService->handleLogin(request.data, connection->socket());
        connection->sendResponse(protocol::buildResponse(request.requestId, response));
    }

    void handleAdminChargerOverview(const protocol::Request& request, TcpConnection* connection)
    {
        QJsonObject response = m_adminService->handleChargerOverview(request.data, connection->socket());
        connection->sendResponse(protocol::buildResponse(request.requestId, response));
    }

    void handleAdminChargersList(const protocol::Request& request, TcpConnection* connection)
    {
        QJsonObject response = m_adminService->handleChargersList(request.data, connection->socket());
        connection->sendResponse(protocol::buildResponse(request.requestId, response));
    }

    void handleAdminChargersRestart(const protocol::Request& request, TcpConnection* connection)
    {
        QJsonObject response = m_adminService->handleChargersRestart(request.data, connection->socket());
        connection->sendResponse(protocol::buildResponse(request.requestId, response));
    }

    void handleAdminChargersStatusUpdate(const protocol::Request& request, TcpConnection* connection)
    {
        QJsonObject response = m_adminService->handleChargersStatusUpdate(request.data, connection->socket());
        connection->sendResponse(protocol::buildResponse(request.requestId, response));
    }

    void handleAdminStationsList(const protocol::Request& request, TcpConnection* connection)
    {
        QJsonObject response = m_adminService->handleStationsList(request.data, connection->socket());
        connection->sendResponse(protocol::buildResponse(request.requestId, response));
    }

    void handleAdminStationsCreate(const protocol::Request& request, TcpConnection* connection)
    {
        QJsonObject response = m_adminService->handleStationsCreate(request.data, connection->socket());
        connection->sendResponse(protocol::buildResponse(request.requestId, response));
    }

    void handleAdminStationsUpdate(const protocol::Request& request, TcpConnection* connection)
    {
        QJsonObject response = m_adminService->handleStationsUpdate(request.data, connection->socket());
        connection->sendResponse(protocol::buildResponse(request.requestId, response));
    }

    void handleAdminUsersList(const protocol::Request& request, TcpConnection* connection)
    {
        QJsonObject response = m_adminService->handleUsersList(request.data, connection->socket());
        connection->sendResponse(protocol::buildResponse(request.requestId, response));
    }

    void handleAdminUsersFreeze(const protocol::Request& request, TcpConnection* connection)
    {
        QJsonObject response = m_adminService->handleUsersFreeze(request.data, connection->socket());
        connection->sendResponse(protocol::buildResponse(request.requestId, response));
    }

    void handleAdminRevenueSummary(const protocol::Request& request, TcpConnection* connection)
    {
        QJsonObject response = m_adminService->handleRevenueSummary(request.data, connection->socket());
        connection->sendResponse(protocol::buildResponse(request.requestId, response));
    }

    void handleAdminRevenueTrend(const protocol::Request& request, TcpConnection* connection)
    {
        QJsonObject response = m_adminService->handleRevenueTrend(request.data, connection->socket());
        connection->sendResponse(protocol::buildResponse(request.requestId, response));
    }

    void handleAdminOrdersList(const protocol::Request& request, TcpConnection* connection)
    {
        QJsonObject response = m_adminService->handleOrdersList(request.data, connection->socket());
        connection->sendResponse(protocol::buildResponse(request.requestId, response));
    }

    // Commit 7: 机器学习数据接口

    void handleMLOrdersExport(const protocol::Request& request, TcpConnection* connection)
    {
        QJsonObject response = m_mlService->handleOrdersExport(request.data, connection->socket());
        connection->sendResponse(protocol::buildResponse(request.requestId, response));
    }

private:
    QHash<QString, Handler> m_handlers;
    service::UserService* m_userService;
    service::StationService* m_stationService;
    service::OrderService* m_orderService;
    service::AdminService* m_adminService;
    service::MLService* m_mlService;
};

} // namespace net
