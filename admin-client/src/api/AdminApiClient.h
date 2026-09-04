#pragma once

#include "model/ChargerStatusOverview.h"
#include "model/Charger.h"
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
    QString pendingAccount_;
    QString pendingPassword_;
};
