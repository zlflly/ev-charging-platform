#pragma once

// 这是与成员 2 当前实现同构的协议草案。
// framing、消息外壳、action 和错误码最终以成员 1 冻结版本为准。

#include <QJsonObject>
#include <QString>
#include <QtGlobal>

namespace protocol {

namespace action {
inline constexpr const char* kPing = "PING";
// 管理员业务 action 暂不在客户端单方面命名。
} // namespace action

inline constexpr int kFrameLengthPrefixBytes = 4;
inline constexpr qint64 kMaxPayloadBytes = 16 * 1024 * 1024;
inline constexpr int kDefaultRequestTimeoutMs = 10 * 1000;

enum ErrorCode {
    CodeOk = 0,
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
    case CodeServerError:
        return serverMessage.isEmpty() ? QStringLiteral("服务器内部错误") : serverMessage;
    default:
        return serverMessage.isEmpty()
            ? QStringLiteral("操作失败（错误码 %1）").arg(code)
            : serverMessage;
    }
}

} // namespace protocol
