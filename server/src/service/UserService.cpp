#include "UserService.h"
#include "repository/UserRepository.h"
#include "net/SessionManager.h"
#include "protocol/ProtocolHelper.h"
#include <QRegularExpression>
#include <QDebug>

namespace service {

UserService::UserService(QObject* parent)
    : QObject(parent)
{
}

QJsonObject UserService::handleLogin(const QJsonObject& data, QTcpSocket* socket) {
    // 提取手机号
    QString phone = data.value("phone").toString().trimmed();

    // 校验手机号格式
    if (!validatePhone(phone)) {
        return protocol::makeErrorResponse(protocol::CodeBadRequest, "手机号格式错误（应为1开头的11位数字）");
    }

    // 查询用户是否存在
    auto userOpt = repository::UserRepository::findByPhone(phone);

    if (!userOpt.has_value()) {
        // 不存在则自动注册
        qInfo() << "[UserService] Auto-registering new user:" << phone;
        userOpt = repository::UserRepository::create(phone);

        if (!userOpt.has_value()) {
            return protocol::makeErrorResponse(protocol::CodeServerError, "用户注册失败");
        }
    }

    const auto& user = userOpt.value();

    // 检查用户状态
    if (user.status == 1) {
        return protocol::makeErrorResponse(protocol::CodeUserFrozen, "账号已被冻结，请联系管理员");
    }

    // 绑定连接到用户
    net::SessionManager::instance().bindUser(socket, user.userId);

    qInfo() << "[UserService] User logged in: userId=" << user.userId << "phone=" << phone;

    // 返回用户信息
    QJsonObject result;
    result["userId"] = user.userId;
    result["phone"] = user.phone;
    result["nickname"] = user.nickname;
    result["avatar"] = user.avatar;  // 可能为空
    result["balance"] = user.balance;
    result["status"] = user.status;

    return protocol::makeSuccessResponse(result);
}

QJsonObject UserService::handleProfileUpdate(const QJsonObject& data, QTcpSocket* socket) {
    // 检查登录状态
    if (!net::SessionManager::instance().isUserLoggedIn(socket)) {
        return protocol::makeErrorResponse(protocol::CodeNotLoggedIn, "请先登录");
    }

    int userId = net::SessionManager::instance().getUserId(socket);

    // 提取字段
    QString nickname = data.value("nickname").toString().trimmed();
    QString avatarData = data.value("avatarData").toString().trimmed();

    bool updated = false;

    // 更新昵称
    if (!nickname.isEmpty()) {
        if (nickname.length() > 20) {
            return protocol::makeErrorResponse(protocol::CodeBadRequest, "昵称长度不能超过20个字符");
        }

        if (!repository::UserRepository::updateNickname(userId, nickname)) {
            return protocol::makeErrorResponse(protocol::CodeServerError, "昵称更新失败");
        }
        updated = true;
        qInfo() << "[UserService] Nickname updated: userId=" << userId;
    }

    // 更新头像
    if (!avatarData.isEmpty()) {
        // 校验 data-URL 格式（简单检查）
        if (!avatarData.startsWith("data:image/")) {
            return protocol::makeErrorResponse(protocol::CodeBadRequest, "头像格式错误（应为data-URL）");
        }

        // 限制大小（256KB，base64后约170KB原始数据）
        if (avatarData.length() > 256 * 1024) {
            return protocol::makeErrorResponse(protocol::CodeBadRequest, "头像文件过大（最大256KB）");
        }

        if (!repository::UserRepository::updateAvatar(userId, avatarData)) {
            return protocol::makeErrorResponse(protocol::CodeServerError, "头像更新失败");
        }
        updated = true;
        qInfo() << "[UserService] Avatar updated: userId=" << userId;
    }

    if (!updated) {
        return protocol::makeErrorResponse(protocol::CodeBadRequest, "未提供有效的更新字段");
    }

    return protocol::makeSuccessResponse(QJsonObject());
}

QJsonObject UserService::handleRecharge(const QJsonObject& data, QTcpSocket* socket) {
    // 检查登录状态
    if (!net::SessionManager::instance().isUserLoggedIn(socket)) {
        return protocol::makeErrorResponse(protocol::CodeNotLoggedIn, "请先登录");
    }

    int userId = net::SessionManager::instance().getUserId(socket);

    // 提取充值金额
    double amount = data.value("amount").toDouble(0.0);

    // 校验金额
    if (!validateAmount(amount)) {
        return protocol::makeErrorResponse(protocol::CodeBadRequest, "充值金额必须大于0且最多2位小数");
    }

    // 更新余额
    if (!repository::UserRepository::updateBalance(userId, amount)) {
        return protocol::makeErrorResponse(protocol::CodeServerError, "充值失败");
    }

    // 查询新余额
    auto userOpt = repository::UserRepository::findById(userId);
    if (!userOpt.has_value()) {
        return protocol::makeErrorResponse(protocol::CodeServerError, "查询余额失败");
    }

    double newBalance = userOpt.value().balance;
    qInfo() << "[UserService] Recharge success: userId=" << userId << "amount=" << amount << "newBalance=" << newBalance;

    QJsonObject result;
    result["balance"] = newBalance;

    return protocol::makeSuccessResponse(result);
}

bool UserService::validatePhone(const QString& phone) const {
    // 中国手机号：1开头的11位数字
    QRegularExpression regex("^1\\d{10}$");
    return regex.match(phone).hasMatch();
}

bool UserService::validateAmount(double amount) const {
    // 金额必须大于0，且最多2位小数
    if (amount <= 0 || amount > 100000) {
        return false;
    }

    // 检查小数位数
    QString amountStr = QString::number(amount, 'f', 2);
    double rounded = amountStr.toDouble();
    return qAbs(amount - rounded) < 0.001;
}

} // namespace service
