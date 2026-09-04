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
inline constexpr const char* kAdminChargerList = "admin.charger.list";
inline constexpr const char* kAdminChargerRestart = "admin.charger.restart";
} // namespace action

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

enum ErrorCode {
    CodeOk = 0,
    CodeBadRequest = 1001,
    CodeNotLoggedIn = 1003,
    CodeInvalidAdminCredentials = 1101,
    CodeChargerOperationRejected = 2101,
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
    case CodeNotLoggedIn:
        return QStringLiteral("登录状态已失效，请重新登录");
    case CodeInvalidAdminCredentials:
        return QStringLiteral("管理员账号或密码错误");
    case CodeChargerOperationRejected:
        return serverMessage.isEmpty()
            ? QStringLiteral("充电桩正在服务订单，当前禁止执行运维操作")
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
