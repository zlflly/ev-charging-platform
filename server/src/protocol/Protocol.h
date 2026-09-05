#pragma once

// ============================================================================
// 服务端协议定义：与客户端冻结对齐
//
// 本文件镜像 reference-member2/user-client/src/protocol/Protocol.h 的已冻结部分。
// 任何变更必须同步更新 docs/协议冻结说明.md 并通知受影响成员。
// ============================================================================

#include <QString>
#include <QJsonObject>
#include <QtGlobal>

namespace protocol {

// ----------------------------------------------------------------------------
// 传输层常量（已冻结）
// ----------------------------------------------------------------------------

// 帧格式：4 字节大端 payload 长度 + UTF-8 JSON payload（长度不含自身）
inline constexpr int kFrameLengthPrefixBytes = 4;
inline constexpr qint64 kMaxPayloadBytes = 16 * 1024 * 1024; // 16MB 上限

// 默认监听配置
inline constexpr const char* kDefaultHost = "0.0.0.0";
inline constexpr quint16 kDefaultPort = 9000;

// ----------------------------------------------------------------------------
// 业务 action 常量（已冻结）
// ----------------------------------------------------------------------------

namespace action {
// 通用
inline constexpr const char* kPing = "PING";

// 用户端（已冻结）
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

// 管理员端（Commit 6 补齐）
inline constexpr const char* kAdminLogin         = "admin.login";
inline constexpr const char* kAdminChargerOverview = "admin.charger.overview";
inline constexpr const char* kAdminChargersList  = "admin.chargers.list";
inline constexpr const char* kAdminChargersRestart = "admin.chargers.restart";
inline constexpr const char* kAdminChargersStatusUpdate = "admin.chargers.status.update";
inline constexpr const char* kAdminStationsList  = "admin.stations.list";
inline constexpr const char* kAdminStationsCreate = "admin.stations.create";
inline constexpr const char* kAdminStationsUpdate = "admin.stations.update";
inline constexpr const char* kAdminUsersList     = "admin.users.list";
inline constexpr const char* kAdminUsersFreeze   = "admin.users.freeze";
inline constexpr const char* kAdminRevenueSummary = "admin.revenue.summary";
inline constexpr const char* kAdminRevenueTrend  = "admin.revenue.trend";
inline constexpr const char* kAdminOrdersList    = "admin.orders.list";
inline constexpr const char* kAdminChargersStats = "admin.chargers.stats";

// 机器学习端（Commit 7 补齐）
inline constexpr const char* kMlOrdersExport = "ml.orders.export";
inline constexpr const char* kMlForecastGet  = "ml.forecast.get";
} // namespace action

// ----------------------------------------------------------------------------
// 错误码（已冻结）
// ----------------------------------------------------------------------------

enum ErrorCode {
    CodeOk = 0,                     // 成功

    // 通用业务错误
    CodeBadRequest = 1001,          // 参数格式错误
    CodeUserFrozen = 1002,          // 账号被冻结
    CodeNotLoggedIn = 1003,         // 未登录（连接未绑定用户）

    // 管理员错误
    CodeAdminAuthFailed = 1101,     // 管理员账号或密码错误

    // 充电业务错误
    CodeChargerUnavailable = 2001,  // 桩不可用/被占用
    CodeBalanceInsufficient = 2002, // 余额不足
    CodeOrderConflict = 2003,       // 订单状态冲突

    // 服务端内部错误
    CodeServerError = 5000,         // 服务端内部错误（fallback）
};

// ----------------------------------------------------------------------------
// 枚举定义（已冻结）
// ----------------------------------------------------------------------------

// 充电桩状态（数据库存储整数）
enum ChargerStatus {
    ChargerStatusIdle = 0,      // 空闲（可预约）
    ChargerStatusCharging = 1,  // 充电中（在用）
    ChargerStatusFault = 2,     // 故障
    ChargerStatusOffline = 3,   // 离线
};

// 充电桩类型（数据库存储整数）
enum ChargerType {
    ChargerTypeFast = 0,  // 快充
    ChargerTypeSlow = 1,  // 慢充
};

// 用户状态（数据库存储整数）
enum UserStatus {
    UserStatusNormal = 0,  // 正常
    UserStatusFrozen = 1,  // 冻结
};

// 订单状态（数据库存储字符串枚举，服务端是唯一状态机）
inline constexpr const char* kOrderStatusReserved       = "RESERVED";
inline constexpr const char* kOrderStatusCharging       = "CHARGING";
inline constexpr const char* kOrderStatusWaitSettlement = "WAIT_SETTLEMENT";
inline constexpr const char* kOrderStatusFinished       = "FINISHED";

// ----------------------------------------------------------------------------
// 消息外壳（已冻结）
// ----------------------------------------------------------------------------

// 请求外壳：{"action": "...", "requestId": "...", "data": {...}}
inline QJsonObject buildRequest(const QString& action,
                                const QString& requestId,
                                const QJsonObject& data)
{
    QJsonObject request;
    request.insert(QStringLiteral("action"), action);
    request.insert(QStringLiteral("requestId"), requestId);
    request.insert(QStringLiteral("data"), data);
    return request;
}

// 响应外壳：{"requestId": "...", "code": 0, "message": "ok", "data": {...}}
inline QJsonObject buildResponse(const QString& requestId,
                                 int code,
                                 const QString& message,
                                 const QJsonObject& data)
{
    QJsonObject response;
    response.insert(QStringLiteral("requestId"), requestId);
    response.insert(QStringLiteral("code"), code);
    response.insert(QStringLiteral("message"), message);
    response.insert(QStringLiteral("data"), data);
    return response;
}

// 快捷构造成功响应
inline QJsonObject buildSuccessResponse(const QString& requestId,
                                        const QJsonObject& data = QJsonObject())
{
    return buildResponse(requestId, CodeOk, QStringLiteral("ok"), data);
}

// 快捷构造错误响应
inline QJsonObject buildErrorResponse(const QString& requestId,
                                      int code,
                                      const QString& message)
{
    return buildResponse(requestId, code, message, QJsonObject());
}

// 从 JSON 提取请求字段
struct Request {
    QString action;
    QString requestId;
    QJsonObject data;

    static Request fromJson(const QJsonObject& jsonObject)
    {
        Request request;
        request.action = jsonObject.value(QStringLiteral("action")).toString();
        request.requestId = jsonObject.value(QStringLiteral("requestId")).toString();
        request.data = jsonObject.value(QStringLiteral("data")).toObject();
        return request;
    }

    bool isValid() const {
        return !action.isEmpty() && !requestId.isEmpty();
    }
};

// ----------------------------------------------------------------------------
// 业务规则常量
// ----------------------------------------------------------------------------

// 手机号正则校验（11 位，1 开头）
inline constexpr const char* kPhonePattern = "^1\\d{10}$";

// 默认昵称生成规则："用户" + 手机号后 4 位
inline QString generateDefaultNickname(const QString& phone) {
    if (phone.length() == 11) {
        return QStringLiteral("用户") + phone.right(4);
    }
    return QStringLiteral("用户");
}

// 初始管理员种子数据
inline constexpr const char* kInitialAdminAccount = "admin";
inline constexpr const char* kInitialAdminPassword = "123456";
inline constexpr const char* kInitialAdminDisplayName = "系统管理员";

} // namespace protocol
