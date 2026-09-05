#pragma once

// 与成员 2 客户端采用相同 framing 和消息外壳。
// 管理员认证约定同步记录在 docs/admin-protocol.md。

#include <QJsonObject>
#include <QString>
#include <QtGlobal>

namespace protocol {

namespace action {
inline constexpr const char* kPing = "PING";
inline constexpr const char* kAdminLogin = "admin.login";
inline constexpr const char* kAdminChargerOverview = "admin.charger.overview";
inline constexpr const char* kAdminChargerList = "admin.chargers.list";
inline constexpr const char* kAdminChargerRestart = "admin.chargers.restart";
inline constexpr const char* kAdminChargerStatusUpdate = "admin.chargers.status.update";
inline constexpr const char* kAdminStationList = "admin.stations.list";
inline constexpr const char* kAdminStationCreate = "admin.stations.create";
inline constexpr const char* kAdminStationUpdate = "admin.stations.update";
inline constexpr const char* kAdminUserList = "admin.users.list";
inline constexpr const char* kAdminUserFreeze = "admin.users.freeze";
inline constexpr const char* kAdminRevenueSummary = "admin.revenue.summary";
inline constexpr const char* kAdminRevenueTrend = "admin.revenue.trend";
inline constexpr const char* kAdminOrderList = "admin.orders.list";
inline constexpr const char* kStationDetail = "station.detail";
} // namespace action

namespace orderStatus {
inline constexpr const char* kReserved = "RESERVED";
inline constexpr const char* kCharging = "CHARGING";
inline constexpr const char* kWaitSettlement = "WAIT_SETTLEMENT";
inline constexpr const char* kFinished = "FINISHED";
} // namespace orderStatus

inline constexpr int kFrameLengthPrefixBytes = 4;
inline constexpr qint64 kMaxPayloadBytes = 16 * 1024 * 1024;
inline constexpr int kDefaultRequestTimeoutMs = 10 * 1000;

// 与用户端保持一致；聚合接口使用具名字段，后续单桩列表按此枚举解析。
enum ChargerStatus {
    ChargerStatusIdle = 0,
    ChargerStatusCharging = 1,
    ChargerStatusFault = 2,
    ChargerStatusOffline = 3,
};

enum ChargerType {
    ChargerTypeFast = 0,
    ChargerTypeSlow = 1,
};

enum UserStatus {
    UserStatusNormal = 0,
    UserStatusFrozen = 1,
};

enum ErrorCode {
    CodeOk = 0,
    CodeBadRequest = 1001,
    CodeUserFrozen = 1002,
    CodeNotLoggedIn = 1003,
    CodeInvalidAdminCredentials = 1101,
    CodeOrderConflict = 2003,
    CodeChargerOperationRejected = 2101,
    CodeStationVersionConflict = 2102,
    CodeChargerStateConflict = 2103,
    CodeUserStateConflict = 2104,
    CodeServerError = 5000,
};

enum TransportErrorCode {
    CodeNotConnected = -1,
    CodeConnectionLost = -2,
    CodeRequestTimeout = -3,
    CodeBadPayload = -4,
};

inline QJsonObject buildEnvelope(const QString& actionName,
                                 const QString& requestId,
                                 const QJsonObject& data)
{
    return {
        {QStringLiteral("action"), actionName},
        {QStringLiteral("requestId"), requestId},
        {QStringLiteral("data"), data},
    };
}

struct Response {
    QString requestId;
    int code = CodeServerError;
    QString message;
    QJsonObject data;

    bool isOk() const { return code == CodeOk; }

    static Response fromJson(const QJsonObject& object)
    {
        Response response;
        response.requestId = object.value(QStringLiteral("requestId")).toString();
        response.code = object.value(QStringLiteral("code")).toInt(CodeServerError);
        response.message = object.value(QStringLiteral("message")).toString();
        response.data = object.value(QStringLiteral("data")).toObject();
        return response;
    }
};

inline QString describeError(int code, const QString& serverMessage = {})
{
    switch (code) {
    case CodeNotConnected:
        return QStringLiteral("未连接到服务器");
    case CodeConnectionLost:
        return QStringLiteral("与服务器的连接已断开");
    case CodeRequestTimeout:
        return QStringLiteral("请求超时，请稍后重试");
    case CodeBadPayload:
        return QStringLiteral("服务器响应格式异常");
    case CodeBadRequest:
        return serverMessage.isEmpty() ? QStringLiteral("提交的参数不正确") : serverMessage;
    case CodeUserFrozen:
        return serverMessage.isEmpty()
            ? QStringLiteral("用户账号已被冻结") : serverMessage;
    case CodeNotLoggedIn:
        return QStringLiteral("登录状态已失效，请重新登录");
    case CodeInvalidAdminCredentials:
        return QStringLiteral("管理员账号或密码错误");
    case CodeOrderConflict:
        return serverMessage.isEmpty()
            ? QStringLiteral("订单状态冲突") : serverMessage;
    case CodeChargerOperationRejected:
        return serverMessage.isEmpty()
            ? QStringLiteral("充电桩正在服务订单，当前禁止执行运维操作")
            : serverMessage;
    case CodeStationVersionConflict:
        return serverMessage.isEmpty()
            ? QStringLiteral("站点资料已被其他管理员修改，请刷新后重试")
            : serverMessage;
    case CodeChargerStateConflict:
        return serverMessage.isEmpty()
            ? QStringLiteral("充电桩状态已变化，请刷新后重试")
            : serverMessage;
    case CodeUserStateConflict:
        return serverMessage.isEmpty()
            ? QStringLiteral("用户状态已变化，请刷新后重试")
            : serverMessage;
    case CodeServerError:
        return serverMessage.isEmpty() ? QStringLiteral("服务器内部错误") : serverMessage;
    default:
        return serverMessage.isEmpty()
            ? QStringLiteral("操作失败（错误码 %1）").arg(code)
            : serverMessage;
    }
}

} // namespace protocol
