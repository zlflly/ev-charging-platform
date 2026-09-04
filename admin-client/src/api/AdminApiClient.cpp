#include "api/AdminApiClient.h"

#include "config/AppConfig.h"
#include "protocol/Protocol.h"
#include "session/AdminSession.h"

#include <QJsonObject>

#include <cmath>
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

bool AdminApiClient::requestChargerOverview(ChargerOverviewCallback callback)
{
    if (chargerOverviewInFlight_) {
        return false;
    }

    chargerOverviewInFlight_ = true;
    sendAuthenticated(
        QString::fromUtf8(protocol::action::kAdminChargerOverview), {},
        [this, callback = std::move(callback)](const protocol::Response& response) {
            chargerOverviewInFlight_ = false;
            if (!response.isOk()) {
                if (callback) {
                    callback(std::nullopt,
                             protocol::describeError(response.code, response.message));
                }
                return;
            }

            ChargerStatusOverview overview;
            QString parseError;
            if (!ChargerStatusOverview::fromJson(response.data, &overview, &parseError)) {
                if (callback) {
                    callback(std::nullopt,
                             QStringLiteral("充电桩状态数据异常：%1").arg(parseError));
                }
                return;
            }

            if (callback) {
                callback(overview, {});
            }
        });
    return true;
}

bool AdminApiClient::isChargerOverviewInFlight() const
{
    return chargerOverviewInFlight_;
}

bool AdminApiClient::requestChargers(ChargerListCallback callback)
{
    if (chargerListInFlight_ || chargerRestartInFlight_
        || chargerStatusUpdateInFlight_) {
        return false;
    }

    chargerListInFlight_ = true;
    sendAuthenticated(
        QString::fromUtf8(protocol::action::kAdminChargerList), {},
        [this, callback = std::move(callback)](const protocol::Response& response) {
            chargerListInFlight_ = false;
            if (!response.isOk()) {
                if (callback) {
                    callback(std::nullopt,
                             protocol::describeError(response.code, response.message));
                }
                return;
            }

            const QJsonValue chargersValue =
                response.data.value(QStringLiteral("chargers"));
            if (!chargersValue.isArray()) {
                if (callback) {
                    callback(std::nullopt,
                             QStringLiteral("充电桩列表响应缺少 chargers 数组"));
                }
                return;
            }

            QList<Charger> chargers;
            QString parseError;
            if (!Charger::listFromJson(chargersValue.toArray(), &chargers, &parseError)) {
                if (callback) {
                    callback(std::nullopt,
                             QStringLiteral("充电桩列表数据异常：%1").arg(parseError));
                }
                return;
            }

            if (callback) {
                callback(chargers, {});
            }
        });
    return true;
}

bool AdminApiClient::isChargerListInFlight() const
{
    return chargerListInFlight_;
}

bool AdminApiClient::restartCharger(qint64 chargerId, OperationCallback callback)
{
    if (chargerRestartInFlight_ || chargerStatusUpdateInFlight_
        || chargerListInFlight_ || chargerId <= 0) {
        return false;
    }

    chargerRestartInFlight_ = true;
    QJsonObject data;
    data.insert(QStringLiteral("chargerId"), chargerId);
    sendAuthenticated(
        QString::fromUtf8(protocol::action::kAdminChargerRestart), data,
        [this, chargerId, callback = std::move(callback)](
            const protocol::Response& response) {
            chargerRestartInFlight_ = false;
            if (!response.isOk()) {
                if (callback) {
                    callback(false,
                             protocol::describeError(response.code, response.message));
                }
                return;
            }

            const QJsonValue responseIdValue =
                response.data.value(QStringLiteral("chargerId"));
            const double responseIdNumber = responseIdValue.toDouble();
            if (!responseIdValue.isDouble() || !std::isfinite(responseIdNumber)
                || std::floor(responseIdNumber) != responseIdNumber
                || responseIdNumber != static_cast<double>(chargerId)) {
                if (callback) {
                    callback(false, QStringLiteral("重启响应中的充电桩 ID 不匹配"));
                }
                return;
            }

            if (callback) {
                callback(true, response.message.isEmpty()
                                   ? QStringLiteral("服务器已接受重启指令")
                                   : response.message);
            }
        });
    return true;
}

bool AdminApiClient::isChargerRestartInFlight() const
{
    return chargerRestartInFlight_;
}

bool AdminApiClient::requestStations(StationListCallback callback)
{
    if (stationListInFlight_ || stationCreateInFlight_ || stationUpdateInFlight_
        || chargerStatusUpdateInFlight_) {
        return false;
    }

    stationListInFlight_ = true;
    sendAuthenticated(
        QString::fromUtf8(protocol::action::kAdminStationList), {},
        [this, callback = std::move(callback)](const protocol::Response& response) {
            stationListInFlight_ = false;
            if (!response.isOk()) {
                if (callback) {
                    callback(std::nullopt,
                             protocol::describeError(response.code, response.message));
                }
                return;
            }

            const QJsonValue stationsValue =
                response.data.value(QStringLiteral("stations"));
            if (!stationsValue.isArray()) {
                if (callback) {
                    callback(std::nullopt,
                             QStringLiteral("充电站列表响应缺少 stations 数组"));
                }
                return;
            }

            QList<Station> stations;
            QString parseError;
            if (!Station::listFromJson(stationsValue.toArray(),
                                       &stations, &parseError)) {
                if (callback) {
                    callback(std::nullopt,
                             QStringLiteral("充电站列表数据异常：%1").arg(parseError));
                }
                return;
            }
            if (callback) {
                callback(stations, {});
            }
        });
    return true;
}

bool AdminApiClient::isStationListInFlight() const
{
    return stationListInFlight_;
}

bool AdminApiClient::requestStationDetail(qint64 stationId,
                                          StationDetailCallback callback)
{
    if (stationId <= 0) {
        return false;
    }

    ++stationDetailInFlightCount_;
    QJsonObject data;
    data.insert(QStringLiteral("stationId"), stationId);
    sendAuthenticated(
        QString::fromUtf8(protocol::action::kStationDetail), data,
        [this, stationId, callback = std::move(callback)](
            const protocol::Response& response) {
            stationDetailInFlightCount_ = qMax(0, stationDetailInFlightCount_ - 1);
            if (!response.isOk()) {
                if (callback) {
                    callback(std::nullopt,
                             protocol::describeError(response.code, response.message));
                }
                return;
            }

            StationDetail detail;
            QString parseError;
            if (!StationDetail::fromJson(response.data, &detail, &parseError)) {
                if (callback) {
                    callback(std::nullopt,
                             QStringLiteral("站内详情数据异常：%1").arg(parseError));
                }
                return;
            }
            if (detail.stationId != stationId) {
                if (callback) {
                    callback(std::nullopt,
                             QStringLiteral("站内详情返回了不匹配的站点 ID"));
                }
                return;
            }
            if (callback) {
                callback(detail, {});
            }
        });
    return true;
}

bool AdminApiClient::isStationDetailInFlight() const
{
    return stationDetailInFlightCount_ > 0;
}

bool AdminApiClient::createStation(const StationCreateRequest& request,
                                   StationCreateCallback callback)
{
    if (stationCreateInFlight_ || stationUpdateInFlight_ || stationListInFlight_) {
        return false;
    }

    QString validationError;
    if (!request.validate(&validationError)) {
        if (callback) {
            callback(std::nullopt, validationError);
        }
        return false;
    }

    stationCreateInFlight_ = true;
    sendAuthenticated(
        QString::fromUtf8(protocol::action::kAdminStationCreate), request.toJson(),
        [this, expectedCount = request.chargerCount,
         callback = std::move(callback)](const protocol::Response& response) {
            stationCreateInFlight_ = false;
            if (!response.isOk()) {
                if (callback) {
                    callback(std::nullopt,
                             protocol::describeError(response.code, response.message));
                }
                return;
            }

            StationCreateResult result;
            QString parseError;
            if (!StationCreateResult::fromJson(response.data, &result, &parseError)) {
                if (callback) {
                    callback(std::nullopt,
                             QStringLiteral("新增站点响应异常：%1").arg(parseError));
                }
                return;
            }
            if (result.createdChargerCount != expectedCount) {
                if (callback) {
                    callback(std::nullopt,
                             QStringLiteral("服务端创建的初始电桩数量与请求不一致"));
                }
                return;
            }
            if (callback) {
                callback(result, {});
            }
        });
    return true;
}

bool AdminApiClient::isStationCreateInFlight() const
{
    return stationCreateInFlight_;
}

bool AdminApiClient::updateStation(const StationUpdateRequest& request,
                                   StationUpdateCallback callback)
{
    if (stationUpdateInFlight_ || stationCreateInFlight_ || stationListInFlight_) {
        return false;
    }

    QString validationError;
    if (!request.validate(&validationError)) {
        if (callback) {
            callback(std::nullopt, validationError);
        }
        return false;
    }

    stationUpdateInFlight_ = true;
    sendAuthenticated(
        QString::fromUtf8(protocol::action::kAdminStationUpdate), request.toJson(),
        [this, expectedId = request.stationId,
         callback = std::move(callback)](const protocol::Response& response) {
            stationUpdateInFlight_ = false;
            if (!response.isOk()) {
                if (callback) {
                    callback(std::nullopt,
                             protocol::describeError(response.code, response.message));
                }
                return;
            }

            StationUpdateResult result;
            QString parseError;
            if (!StationUpdateResult::fromJson(response.data, &result, &parseError)) {
                if (callback) {
                    callback(std::nullopt,
                             QStringLiteral("编辑站点响应异常：%1").arg(parseError));
                }
                return;
            }
            if (result.stationId != expectedId) {
                if (callback) {
                    callback(std::nullopt, QStringLiteral("编辑站点响应中的 ID 不匹配"));
                }
                return;
            }
            if (callback) {
                callback(result, {});
            }
        });
    return true;
}

bool AdminApiClient::isStationUpdateInFlight() const
{
    return stationUpdateInFlight_;
}

bool AdminApiClient::updateChargerStatus(
    const ChargerStatusUpdateRequest& request,
    ChargerStatusUpdateCallback callback)
{
    if (chargerStatusUpdateInFlight_ || chargerRestartInFlight_
        || chargerListInFlight_) {
        return false;
    }

    QString validationError;
    if (!request.validate(&validationError)) {
        if (callback) {
            callback(std::nullopt, validationError);
        }
        return false;
    }

    chargerStatusUpdateInFlight_ = true;
    sendAuthenticated(
        QString::fromUtf8(protocol::action::kAdminChargerStatusUpdate),
        request.toJson(),
        [this, expectedId = request.chargerId,
         expectedPrevious = request.expectedStatus,
         expectedTarget = request.targetStatus,
         callback = std::move(callback)](const protocol::Response& response) {
            chargerStatusUpdateInFlight_ = false;
            if (!response.isOk()) {
                if (callback) {
                    callback(std::nullopt,
                             protocol::describeError(response.code, response.message));
                }
                return;
            }

            ChargerStatusUpdateResult result;
            QString parseError;
            if (!ChargerStatusUpdateResult::fromJson(
                    response.data, &result, &parseError)) {
                if (callback) {
                    callback(std::nullopt,
                             QStringLiteral("状态变更响应异常：%1").arg(parseError));
                }
                return;
            }
            if (result.chargerId != expectedId
                || result.previousStatus != expectedPrevious
                || result.status != expectedTarget) {
                if (callback) {
                    callback(std::nullopt, QStringLiteral("状态变更响应与请求不匹配"));
                }
                return;
            }
            if (callback) {
                callback(result, {});
            }
        });
    return true;
}

bool AdminApiClient::isChargerStatusUpdateInFlight() const
{
    return chargerStatusUpdateInFlight_;
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
