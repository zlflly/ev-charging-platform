#pragma once

#include "model/ChargerStatusOverview.h"
#include "model/Charger.h"
#include "model/Station.h"
#include "model/User.h"
#include "model/Revenue.h"
#include "model/Order.h"
#include "net/NetworkClient.h"

#include <QJsonObject>
#include <QObject>
#include <QString>

#include <functional>
#include <optional>

class AdminSession;

// 管理员业务请求的统一入口。
// 当前协议把身份绑定到 TCP 连接；若以后改为 token，只需在这里统一携带。
class AdminApiClient final : public QObject
{
    Q_OBJECT

public:
    using ChargerOverviewCallback = std::function<void(
        std::optional<ChargerStatusOverview> overview,
        const QString& errorMessage)>;
    using ChargerListCallback = std::function<void(
        std::optional<QList<Charger>> chargers,
        const QString& errorMessage)>;
    using OperationCallback = std::function<void(bool ok, const QString& message)>;
    using StationListCallback = std::function<void(
        std::optional<QList<Station>> stations,
        const QString& errorMessage)>;
    using StationDetailCallback = std::function<void(
        std::optional<StationDetail> detail,
        const QString& errorMessage)>;
    using StationCreateCallback = std::function<void(
        std::optional<StationCreateResult> result,
        const QString& errorMessage)>;
    using StationUpdateCallback = std::function<void(
        std::optional<StationUpdateResult> result,
        const QString& errorMessage)>;
    using ChargerStatusUpdateCallback = std::function<void(
        std::optional<ChargerStatusUpdateResult> result,
        const QString& errorMessage)>;
    using UserListCallback = std::function<void(
        std::optional<UserListPage> page,
        const QString& errorMessage)>;
    using UserStatusUpdateCallback = std::function<void(
        std::optional<UserStatusUpdateResult> result,
        const QString& errorMessage)>;
    using RevenueSummaryCallback = std::function<void(
        std::optional<RevenueSummary> summary,
        const QString& errorMessage)>;
    using RevenueTrendCallback = std::function<void(
        std::optional<RevenueTrend> trend,
        const QString& errorMessage)>;
    using OrderListCallback = std::function<void(
        std::optional<OrderListPage> page,
        const QString& errorMessage)>;

    explicit AdminApiClient(NetworkClient* network,
                            AdminSession* session,
                            QObject* parent = nullptr);

    bool login(const QString& account, const QString& password);
    void logout();
    bool isLoginInFlight() const;
    bool requestChargerOverview(ChargerOverviewCallback callback);
    bool isChargerOverviewInFlight() const;
    bool requestChargers(ChargerListCallback callback);
    bool isChargerListInFlight() const;
    bool restartCharger(qint64 chargerId, OperationCallback callback);
    bool isChargerRestartInFlight() const;
    bool requestStations(StationListCallback callback);
    bool isStationListInFlight() const;
    bool requestStationDetail(qint64 stationId, StationDetailCallback callback);
    bool isStationDetailInFlight() const;
    bool createStation(const StationCreateRequest& request,
                       StationCreateCallback callback);
    bool isStationCreateInFlight() const;
    bool updateStation(const StationUpdateRequest& request,
                       StationUpdateCallback callback);
    bool isStationUpdateInFlight() const;
    bool updateChargerStatus(const ChargerStatusUpdateRequest& request,
                             ChargerStatusUpdateCallback callback);
    bool isChargerStatusUpdateInFlight() const;
    bool requestUsers(const UserListQuery& query, UserListCallback callback);
    bool isUserListInFlight() const;
    bool updateUserStatus(const UserStatusUpdateRequest& request,
                          UserStatusUpdateCallback callback);
    bool isUserStatusUpdateInFlight() const;
    bool requestRevenueSummary(RevenueSummaryCallback callback);
    bool isRevenueSummaryInFlight() const;
    bool requestRevenueTrend(int days, RevenueTrendCallback callback);
    bool isRevenueTrendInFlight() const;
    bool requestOrders(const OrderListQuery& query, OrderListCallback callback);
    bool isOrderListInFlight() const;

    QString sendAuthenticated(const QString& action,
                              const QJsonObject& data,
                              NetworkClient::ResponseCallback callback,
                              int timeoutMs = protocol::kDefaultRequestTimeoutMs);

signals:
    void loginSucceeded();
    void loginFailed(int code, const QString& message);
    void sessionExpired(const QString& message);

private:
    void sendPendingLogin();
    void finishLoginFailure(int code, const QString& message);
    void clearPendingCredentials();

    NetworkClient* network_ = nullptr;
    AdminSession* session_ = nullptr;
    bool loginInFlight_ = false;
    bool chargerOverviewInFlight_ = false;
    bool chargerListInFlight_ = false;
    bool chargerRestartInFlight_ = false;
    bool stationListInFlight_ = false;
    int stationDetailInFlightCount_ = 0;
    bool stationCreateInFlight_ = false;
    bool stationUpdateInFlight_ = false;
    bool chargerStatusUpdateInFlight_ = false;
    bool userListInFlight_ = false;
    bool userStatusUpdateInFlight_ = false;
    bool revenueSummaryInFlight_ = false;
    int revenueTrendInFlightCount_ = 0;
    bool orderListInFlight_ = false;
    QString pendingAccount_;
    QString pendingPassword_;
};
