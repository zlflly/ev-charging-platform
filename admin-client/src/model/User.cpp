#include "model/User.h"

#include "protocol/Protocol.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonValue>
#include <QRegularExpression>
#include <QSet>

#include <cmath>

namespace {

constexpr double kMaxSafeJsonInteger = 9007199254740991.0;

bool readInteger(const QJsonObject& json, const QString& key, qint64* target,
                 bool positive, QString* errorMessage)
{
    const QJsonValue value = json.value(key);
    const double number = value.toDouble(-1.0);
    const double minimum = positive ? 1.0 : 0.0;
    if (!value.isDouble() || !std::isfinite(number) || number < minimum
        || number > kMaxSafeJsonInteger || std::floor(number) != number) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("字段 %1 必须是%2整数")
                                .arg(key, positive ? QStringLiteral("正")
                                                   : QStringLiteral("非负"));
        }
        return false;
    }
    *target = static_cast<qint64>(number);
    return true;
}

bool validStatus(qint64 status)
{
    return status == protocol::UserStatusNormal
        || status == protocol::UserStatusFrozen;
}

bool activeOrderStatus(const QString& status)
{
    return status == QString::fromUtf8(protocol::orderStatus::kReserved)
        || status == QString::fromUtf8(protocol::orderStatus::kCharging)
        || status == QString::fromUtf8(protocol::orderStatus::kWaitSettlement);
}

bool validActivityFilter(const QString& filter)
{
    return filter == QStringLiteral("ALL")
        || filter == QStringLiteral("ACTIVE")
        || filter == QStringLiteral("IDLE");
}

bool readRequiredText(const QJsonObject& json, const QString& key,
                      int maximumLength, QString* target,
                      QString* errorMessage)
{
    const QJsonValue value = json.value(key);
    const QString text = value.toString().trimmed();
    if (!value.isString() || text.isEmpty() || text.size() > maximumLength) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("字段 %1 必须为 1～%2 个字符")
                                .arg(key).arg(maximumLength);
        }
        return false;
    }
    *target = text;
    return true;
}

QDateTime parseIsoTime(const QString& value)
{
    QDateTime parsed = QDateTime::fromString(value, Qt::ISODateWithMs);
    if (!parsed.isValid()) {
        parsed = QDateTime::fromString(value, Qt::ISODate);
    }
    return parsed;
}

bool readTime(const QJsonObject& json, const QString& key,
              qint64* epochMs, QString* errorMessage)
{
    const QJsonValue value = json.value(key);
    if (value.isDouble()) {
        qint64 parsed = 0;
        if (readInteger(json, key, &parsed, true, errorMessage)) {
            *epochMs = parsed;
            return true;
        }
        return false;
    }
    if (value.isString()) {
        const QDateTime parsed = parseIsoTime(value.toString());
        if (parsed.isValid()) {
            *epochMs = parsed.toMSecsSinceEpoch();
            return true;
        }
    }
    if (errorMessage) {
        *errorMessage = QStringLiteral("字段 %1 必须是 epoch 毫秒或 ISO 8601 时间").arg(key);
    }
    return false;
}

} // namespace

QString AdminUserActiveOrder::statusLabel() const
{
    if (status == QString::fromUtf8(protocol::orderStatus::kReserved)) {
        return QStringLiteral("已预约");
    }
    if (status == QString::fromUtf8(protocol::orderStatus::kCharging)) {
        return QStringLiteral("充电中");
    }
    // 与用户端统一：WAIT_SETTLEMENT 表示充电已停止、账单已生成，等待支付。
    return QStringLiteral("待支付");
}

QString AdminUserActiveOrder::deviceLabel() const
{
    return QStringLiteral("%1 · %2").arg(stationName, chargerCode);
}

bool AdminUserActiveOrder::fromJson(const QJsonObject& json,
                                    AdminUserActiveOrder* order,
                                    QString* errorMessage)
{
    if (!order) return false;
    AdminUserActiveOrder parsed;
    if (!readInteger(json, QStringLiteral("orderId"), &parsed.orderId,
                     true, errorMessage)
        || !readInteger(json, QStringLiteral("stationId"), &parsed.stationId,
                        true, errorMessage)
        || !readRequiredText(json, QStringLiteral("stationName"), 60,
                             &parsed.stationName, errorMessage)
        || !readInteger(json, QStringLiteral("chargerId"), &parsed.chargerId,
                        true, errorMessage)
        || !readRequiredText(json, QStringLiteral("chargerCode"), 80,
                             &parsed.chargerCode, errorMessage)
        || !readRequiredText(json, QStringLiteral("status"), 32,
                             &parsed.status, errorMessage)
        || !activeOrderStatus(parsed.status)) {
        if (errorMessage && errorMessage->isEmpty()) {
            *errorMessage = QStringLiteral("活跃订单 status 取值无效");
        }
        return false;
    }
    *order = parsed;
    return true;
}

QString AdminUser::statusLabel() const
{
    return status == protocol::UserStatusFrozen
        ? QStringLiteral("已冻结") : QStringLiteral("正常");
}

QString AdminUser::formattedBalance() const
{
    return QStringLiteral("¥ %1").arg(balance, 0, 'f', 2);
}

QString AdminUser::formattedCreatedAt() const
{
    return QDateTime::fromMSecsSinceEpoch(createdAtEpochMs)
        .toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
}

QString AdminUser::activityLabel() const
{
    return activeOrder ? activeOrder->statusLabel() : QStringLiteral("未使用");
}

QString AdminUser::currentDeviceLabel() const
{
    return activeOrder ? activeOrder->deviceLabel() : QStringLiteral("—");
}

bool AdminUser::fromJson(const QJsonObject& json,
                         AdminUser* user,
                         QString* errorMessage)
{
    if (!user) {
        return false;
    }
    AdminUser parsed;
    if (!readInteger(json, QStringLiteral("userId"), &parsed.userId,
                     true, errorMessage)) {
        return false;
    }
    const QJsonValue phoneValue = json.value(QStringLiteral("phone"));
    parsed.phone = phoneValue.toString();
    static const QRegularExpression phonePattern(QStringLiteral("^1\\d{10}$"));
    if (!phoneValue.isString() || !phonePattern.match(parsed.phone).hasMatch()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("字段 phone 必须是 11 位手机号");
        }
        return false;
    }
    const QJsonValue nicknameValue = json.value(QStringLiteral("nickname"));
    parsed.nickname = nicknameValue.toString().trimmed();
    if (!nicknameValue.isString() || parsed.nickname.isEmpty()
        || parsed.nickname.size() > 20) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("字段 nickname 必须为 1～20 个字符");
        }
        return false;
    }
    const QJsonValue balanceValue = json.value(QStringLiteral("balance"));
    parsed.balance = balanceValue.toDouble(-1.0);
    if (!balanceValue.isDouble() || !std::isfinite(parsed.balance)
        || parsed.balance < 0.0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("字段 balance 必须是非负金额");
        }
        return false;
    }
    if (!readTime(json, QStringLiteral("createdAt"),
                  &parsed.createdAtEpochMs, errorMessage)) {
        return false;
    }
    qint64 status = -1;
    if (!readInteger(json, QStringLiteral("status"), &status,
                     false, errorMessage) || !validStatus(status)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("字段 status 只能为 0 或 1");
        }
        return false;
    }
    parsed.status = static_cast<int>(status);

    const QJsonValue activityValue = json.value(QStringLiteral("activityStatus"));
    parsed.activityStatus = activityValue.toString().trimmed();
    const QJsonValue activeOrderValue = json.value(QStringLiteral("activeOrder"));
    if (!activityValue.isString()) {
        if (errorMessage) *errorMessage = QStringLiteral("字段 activityStatus 必须是字符串");
        return false;
    }
    if (parsed.activityStatus == QStringLiteral("IDLE")) {
        if (!activeOrderValue.isNull()) {
            if (errorMessage) *errorMessage = QStringLiteral("未使用用户的 activeOrder 必须为 null");
            return false;
        }
    } else if (activeOrderStatus(parsed.activityStatus)) {
        if (!activeOrderValue.isObject()) {
            if (errorMessage) *errorMessage = QStringLiteral("使用中用户缺少 activeOrder 对象");
            return false;
        }
        AdminUserActiveOrder order;
        if (!AdminUserActiveOrder::fromJson(activeOrderValue.toObject(),
                                            &order, errorMessage)) {
            return false;
        }
        if (order.status != parsed.activityStatus) {
            if (errorMessage) *errorMessage = QStringLiteral("使用状态与活跃订单状态不一致");
            return false;
        }
        parsed.activeOrder = order;
    } else {
        if (errorMessage) *errorMessage = QStringLiteral("activityStatus 取值无效");
        return false;
    }
    *user = parsed;
    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}

bool UserListQuery::validate(QString* errorMessage) const
{
    const QString keyword = phoneKeyword.trimmed();
    static const QRegularExpression digits(QStringLiteral("^\\d{0,11}$"));
    if (page < 1) {
        if (errorMessage) *errorMessage = QStringLiteral("页码必须从 1 开始");
        return false;
    }
    if (pageSize < 1 || pageSize > 100) {
        if (errorMessage) *errorMessage = QStringLiteral("每页数量必须在 1～100 之间");
        return false;
    }
    if (!digits.match(keyword).hasMatch()) {
        if (errorMessage) *errorMessage = QStringLiteral("手机号关键字只能包含 0～11 位数字");
        return false;
    }
    if (status != -1 && !validStatus(status)) {
        if (errorMessage) *errorMessage = QStringLiteral("用户状态筛选值无效");
        return false;
    }
    if (!validActivityFilter(activityFilter)) {
        if (errorMessage) *errorMessage = QStringLiteral("用户使用状态筛选值无效");
        return false;
    }
    if (errorMessage) errorMessage->clear();
    return true;
}

QJsonObject UserListQuery::toJson() const
{
    QJsonObject json {
        {QStringLiteral("page"), page},
        {QStringLiteral("pageSize"), pageSize},
        {QStringLiteral("phoneKeyword"), phoneKeyword.trimmed()},
        {QStringLiteral("activityFilter"), activityFilter},
    };
    if (status >= 0) {
        json.insert(QStringLiteral("status"), status);
    }
    return json;
}

bool UserListPage::fromJson(const QJsonObject& json,
                            const UserListQuery& query,
                            UserListPage* result,
                            QString* errorMessage)
{
    if (!result) return false;
    qint64 total = -1;
    qint64 page = -1;
    qint64 pageSize = -1;
    if (!readInteger(json, QStringLiteral("total"), &total, false, errorMessage)
        || !readInteger(json, QStringLiteral("page"), &page, true, errorMessage)
        || !readInteger(json, QStringLiteral("pageSize"), &pageSize, true, errorMessage)
        || page != query.page || pageSize != query.pageSize) {
        if (errorMessage && errorMessage->isEmpty()) {
            *errorMessage = QStringLiteral("分页元数据与请求不匹配");
        }
        return false;
    }
    const QJsonValue usersValue = json.value(QStringLiteral("users"));
    if (!usersValue.isArray()) {
        if (errorMessage) *errorMessage = QStringLiteral("响应缺少 users 数组");
        return false;
    }
    const QJsonArray array = usersValue.toArray();
    if (array.size() > query.pageSize || total < array.size()) {
        if (errorMessage) *errorMessage = QStringLiteral("用户数量与分页元数据不一致");
        return false;
    }
    UserListPage parsed;
    parsed.total = total;
    parsed.page = static_cast<int>(page);
    parsed.pageSize = static_cast<int>(pageSize);
    QSet<qint64> ids;
    for (int index = 0; index < array.size(); ++index) {
        if (!array.at(index).isObject()) {
            if (errorMessage) *errorMessage = QStringLiteral("第 %1 条用户数据不是对象").arg(index + 1);
            return false;
        }
        AdminUser user;
        QString itemError;
        if (!AdminUser::fromJson(array.at(index).toObject(), &user, &itemError)) {
            if (errorMessage) *errorMessage = QStringLiteral("第 %1 条用户数据异常：%2").arg(index + 1).arg(itemError);
            return false;
        }
        if (ids.contains(user.userId)) {
            if (errorMessage) *errorMessage = QStringLiteral("用户 ID %1 重复").arg(user.userId);
            return false;
        }
        if (!query.phoneKeyword.trimmed().isEmpty()
            && !user.phone.contains(query.phoneKeyword.trimmed())) {
            if (errorMessage) *errorMessage = QStringLiteral("服务端返回了不匹配手机号关键字的用户");
            return false;
        }
        if (query.status >= 0 && user.status != query.status) {
            if (errorMessage) *errorMessage = QStringLiteral("服务端返回了不匹配状态筛选的用户");
            return false;
        }
        const bool active = user.activeOrder.has_value();
        if ((query.activityFilter == QStringLiteral("ACTIVE") && !active)
            || (query.activityFilter == QStringLiteral("IDLE") && active)) {
            if (errorMessage) *errorMessage = QStringLiteral("服务端返回了不匹配使用状态筛选的用户");
            return false;
        }
        ids.insert(user.userId);
        parsed.users.append(user);
    }
    *result = parsed;
    if (errorMessage) errorMessage->clear();
    return true;
}

bool UserStatusUpdateRequest::validate(QString* errorMessage) const
{
    if (userId <= 0) {
        if (errorMessage) *errorMessage = QStringLiteral("请选择有效用户");
        return false;
    }
    if (!validStatus(expectedStatus) || !validStatus(targetStatus)
        || expectedStatus == targetStatus) {
        if (errorMessage) *errorMessage = QStringLiteral("用户状态变更参数无效");
        return false;
    }
    const QString normalizedReason = reason.trimmed();
    if (normalizedReason.size() < 2 || normalizedReason.size() > 200) {
        if (errorMessage) *errorMessage = QStringLiteral("操作原因必须为 2～200 个字符");
        return false;
    }
    if (errorMessage) errorMessage->clear();
    return true;
}

QJsonObject UserStatusUpdateRequest::toJson() const
{
    return {
        {QStringLiteral("userId"), userId},
        {QStringLiteral("expectedStatus"), expectedStatus},
        {QStringLiteral("targetStatus"), targetStatus},
        {QStringLiteral("reason"), reason.trimmed()},
    };
}

bool UserStatusUpdateResult::fromJson(const QJsonObject& json,
                                      UserStatusUpdateResult* result,
                                      QString* errorMessage)
{
    if (!result) return false;
    UserStatusUpdateResult parsed;
    qint64 previousStatus = -1;
    qint64 status = -1;
    if (!readInteger(json, QStringLiteral("userId"), &parsed.userId,
                     true, errorMessage)
        || !readInteger(json, QStringLiteral("previousStatus"), &previousStatus,
                        false, errorMessage)
        || !readInteger(json, QStringLiteral("status"), &status,
                        false, errorMessage)
        || !validStatus(previousStatus) || !validStatus(status)
        || previousStatus == status) {
        if (errorMessage) *errorMessage = QStringLiteral("用户状态变更响应字段无效");
        return false;
    }
    if (!readTime(json, QStringLiteral("changedAt"),
                  &parsed.changedAtEpochMs, errorMessage)) {
        return false;
    }
    parsed.previousStatus = static_cast<int>(previousStatus);
    parsed.status = static_cast<int>(status);
    *result = parsed;
    if (errorMessage) errorMessage->clear();
    return true;
}
