#include "api/AdminApiClient.h"

#include "config/AppConfig.h"
#include "protocol/Protocol.h"
#include "session/AdminSession.h"

#include <QJsonObject>

#include <utility>

AdminApiClient::AdminApiClient(NetworkClient* network,
                               AdminSession* session,
                               QObject* parent)
    : QObject(parent)
    , network_(network)
    , session_(session)
{
    Q_ASSERT(network_);
    Q_ASSERT(session_);

    connect(network_, &NetworkClient::connected,
            this, &AdminApiClient::sendPendingLogin);
    connect(network_, &NetworkClient::transportError, this,
            [this](int code, const QString& message) {
        if (loginInFlight_ && !pendingAccount_.isEmpty()) {
            finishLoginFailure(code, protocol::describeError(code, message));
        }
    });
    connect(network_, &NetworkClient::disconnected, this, [this] {
        if (!session_->isAuthenticated()) {
            return;
        }
        session_->clear();
        emit sessionExpired(QStringLiteral("与服务器的连接已断开，请重新登录"));
    });
}

bool AdminApiClient::login(const QString& account, const QString& password)
{
    if (loginInFlight_) {
        return false;
    }

    const QString normalizedAccount = account.trimmed();
    if (normalizedAccount.isEmpty() || password.isEmpty()) {
        emit loginFailed(protocol::CodeBadRequest,
                         QStringLiteral("请输入管理员账号和密码"));
        return false;
    }

    loginInFlight_ = true;
    pendingAccount_ = normalizedAccount;
    pendingPassword_ = password;

    if (network_->isConnected()) {
        sendPendingLogin();
    } else {
        network_->connectToServer(QString::fromUtf8(config::kDefaultServerHost),
                                  config::kDefaultServerPort);
    }
    return true;
}

void AdminApiClient::logout()
{
    loginInFlight_ = false;
    clearPendingCredentials();
    session_->clear();
    // 当前认证与 TCP 连接绑定，退出时主动断开，避免服务端保留旧身份。
    network_->disconnectFromServer();
}

bool AdminApiClient::isLoginInFlight() const
{
    return loginInFlight_;
}

QString AdminApiClient::sendAuthenticated(const QString& action,
                                          const QJsonObject& data,
                                          NetworkClient::ResponseCallback callback,
                                          int timeoutMs)
{
    if (!session_->isAuthenticated()) {
        protocol::Response response;
        response.code = protocol::CodeNotLoggedIn;
        response.message = QStringLiteral("管理员尚未登录");
        if (callback) {
            callback(response);
        }
        return {};
    }

    return network_->sendRequest(
        action, data,
        [this, callback = std::move(callback)](const protocol::Response& response) {
            if (response.code == protocol::CodeNotLoggedIn) {
                session_->clear();
                emit sessionExpired(protocol::describeError(response.code, response.message));
            }
            if (callback) {
                callback(response);
            }
        },
        timeoutMs);
}

void AdminApiClient::sendPendingLogin()
{
    if (!loginInFlight_ || pendingAccount_.isEmpty()) {
        return;
    }

    const QString account = pendingAccount_;
    const QString password = pendingPassword_;
    clearPendingCredentials();

    QJsonObject data;
    data.insert(QStringLiteral("account"), account);
    data.insert(QStringLiteral("password"), password);

    network_->sendRequest(
        QString::fromUtf8(protocol::action::kAdminLogin), data,
        [this, account](const protocol::Response& response) {
            loginInFlight_ = false;
            if (!response.isOk()) {
                emit loginFailed(response.code,
                                 protocol::describeError(response.code, response.message));
                return;
            }

            const qint64 adminId = static_cast<qint64>(
                response.data.value(QStringLiteral("adminId")).toDouble());
            const QString responseAccount =
                response.data.value(QStringLiteral("account")).toString().trimmed();
            QString displayName =
                response.data.value(QStringLiteral("displayName")).toString().trimmed();
            if (adminId <= 0 || responseAccount.isEmpty()) {
                emit loginFailed(protocol::CodeBadPayload,
                                 QStringLiteral("登录响应缺少管理员信息"));
                return;
            }
            if (displayName.isEmpty()) {
                displayName = responseAccount;
            }

            session_->authenticate(adminId, responseAccount, displayName);
            emit loginSucceeded();
        });
}

void AdminApiClient::finishLoginFailure(int code, const QString& message)
{
    loginInFlight_ = false;
    clearPendingCredentials();
    emit loginFailed(code, message);
}

void AdminApiClient::clearPendingCredentials()
{
    pendingAccount_.clear();
    pendingPassword_.fill(QChar(u'\0'));
    pendingPassword_.clear();
}
