#pragma once

#include "net/NetworkClient.h"

#include <QJsonObject>
#include <QObject>
#include <QString>

class AdminSession;

// 管理员业务请求的统一入口。
// 当前协议把身份绑定到 TCP 连接；若以后改为 token，只需在这里统一携带。
class AdminApiClient final : public QObject
{
    Q_OBJECT

public:
    explicit AdminApiClient(NetworkClient* network,
                            AdminSession* session,
                            QObject* parent = nullptr);

    bool login(const QString& account, const QString& password);
    void logout();
    bool isLoginInFlight() const;

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
    QString pendingAccount_;
    QString pendingPassword_;
};
