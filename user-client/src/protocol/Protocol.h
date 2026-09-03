#pragma once

// ============================================================================
// 协议常量与消息外壳定义（沿用 WSL 参考项目的接口；变更需同步服务端）
// ============================================================================

#include <QJsonObject>
#include <QString>
#include <QtGlobal>

namespace protocol {

// 服务器地址默认值（后续可从配置文件读取）
inline constexpr const char* kDefaultHost = "127.0.0.1";
inline constexpr quint16 kDefaultPort = 9000;

// 业务 action 名统一收口在这里，避免各页面硬编码字符串。
namespace action {
inline constexpr const char* kUserLogin         = "user.login";
inline constexpr const char* kUserProfileUpdate = "user.profile.update";
inline constexpr const char* kUserRecharge      = "user.recharge";
inline constexpr const char* kStationNearby     = "station.nearby";
inline constexpr const char* kStationDetail     = "station.detail";
inline constexpr const char* kOrderActive       = "order.active";
inline constexpr const char* kOrderReserve      = "order.reserve";
inline constexpr const char* kOrderStart        = "order.start";
inline constexpr const char* kOrderStatus       = "order.status";
inline constexpr const char* kOrderStop         = "order.stop";
inline constexpr const char* kOrderSettle       = "order.settle";
inline constexpr const char* kOrderHistory      = "order.history";
inline constexpr const char* kPing              = "PING";
} // namespace action

// 充电桩状态枚举
enum ChargerStatus {
    ChargerStatusIdle = 0,
    ChargerStatusCharging = 1,
    ChargerStatusFault = 2,
    ChargerStatusOffline = 3,
};

// 充电桩类型
enum ChargerType {
    ChargerTypeFast = 0,
    ChargerTypeSlow = 1,
};

// 帧格式：4 字节大端 payload 长度 + UTF-8 JSON payload（长度不含自身）
inline constexpr int kFrameLengthPrefixBytes = 4;
inline constexpr qint64 kMaxPayloadBytes = 16 * 1024 * 1024;
inline constexpr int kDefaultRequestTimeoutMs = 10 * 1000;

// 业务错误码（0 = 成功；1000+ = 服务端业务错误；负数 = 客户端传输层错误）
enum ErrorCode {
    CodeOk = 0,
    CodeBadRequest = 1001,
    CodeUserFrozen = 1002,
    CodeNotLoggedIn = 1003,
    CodeChargerUnavailable = 2001,
    CodeBalanceInsufficient = 2002,
    CodeOrderConflict = 2003,
    CodeServerError = 5000,
};

enum TransportErrorCode {
    CodeNotConnected = -1,
    CodeConnectionLost = -2,
    CodeRequestTimeout = -3,
    CodeBadPayload = -4,
};

inline QJsonObject buildEnvelope(const QString& action,
                                 const QString& requestId,
                                 const QJsonObject& data)
{
    QJsonObject envelope;
    envelope.insert(QStringLiteral("action"), action);
    envelope.insert(QStringLiteral("requestId"), requestId);
    envelope.insert(QStringLiteral("data"), data);
    return envelope;
}

struct Response {
    QString requestId;
    int code = CodeOk;
    QString message;
    QJsonObject data;

    bool isOk() const { return code == CodeOk; }

    static Response fromJson(const QJsonObject& jsonObject)
    {
        Response response;
        response.requestId = jsonObject.value(QStringLiteral("requestId")).toString();
        response.code = jsonObject.value(QStringLiteral("code")).toInt(CodeServerError);
        response.message = jsonObject.value(QStringLiteral("message")).toString();
        response.data = jsonObject.value(QStringLiteral("data")).toObject();
        return response;
    }
};

inline QString describeError(int code, const QString& serverMessage)
{
    switch (code) {
    case CodeNotConnected:
        return QStringLiteral("未连接到服务器，请确认服务端已启动。");
    case CodeConnectionLost:
        return QStringLiteral("与服务器连接已断开，请稍候。");
    case CodeRequestTimeout:
        return QStringLiteral("请求超时，请检查网络后重试。");
    case CodeBadPayload:
        return QStringLiteral("服务器返回数据异常，请重试。");
    case CodeBadRequest:
        return QStringLiteral("提交的数据格式不正确，请检查后重试。");
    case CodeUserFrozen:
        return QStringLiteral("该账号已被冻结，请联系客服处理。");
    case CodeNotLoggedIn:
        return QStringLiteral("登录状态已失效，请重新登录。");
    case CodeChargerUnavailable:
        return QStringLiteral("该充电桩当前不可用或已被占用，请重新选择。");
    case CodeBalanceInsufficient:
        return QStringLiteral("余额不足，请先充值。");
    case CodeOrderConflict:
        return QStringLiteral("订单状态已变化，请刷新后重试。");
    case CodeServerError:
        return QStringLiteral("服务器暂时不可用，请稍后重试。");
    default:
        break;
    }
    if (!serverMessage.isEmpty()) {
        return serverMessage;
    }
    return QStringLiteral("操作失败（错误码 %1）").arg(code);
}

} // namespace protocol