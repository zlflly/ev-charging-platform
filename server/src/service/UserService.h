#ifndef USER_SERVICE_H
#define USER_SERVICE_H

#include <QObject>
#include <QJsonObject>
#include <QTcpSocket>

namespace service {

/**
 * 用户业务服务层
 * 处理 user.* 相关的 action
 */
class UserService : public QObject {
    Q_OBJECT

public:
    explicit UserService(QObject* parent = nullptr);

    // user.login: 手机号登录/自动注册
    QJsonObject handleLogin(const QJsonObject& data, QTcpSocket* socket);

    // user.profile.update: 更新昵称/头像
    QJsonObject handleProfileUpdate(const QJsonObject& data, QTcpSocket* socket);

    // user.recharge: 充值
    QJsonObject handleRecharge(const QJsonObject& data, QTcpSocket* socket);

private:
    // 校验手机号格式
    bool validatePhone(const QString& phone) const;

    // 校验充值金额
    bool validateAmount(double amount) const;
};

} // namespace service

#endif // USER_SERVICE_H
