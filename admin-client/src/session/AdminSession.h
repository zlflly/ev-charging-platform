#pragma once

#include <QObject>
#include <QString>
#include <QtGlobal>

// 管理员登录态的唯一来源。密码和连接细节不属于会话数据，绝不在此保存。
class AdminSession final : public QObject
{
    Q_OBJECT

public:
    explicit AdminSession(QObject* parent = nullptr);

    bool isAuthenticated() const;
    qint64 adminId() const;
    QString account() const;
    QString displayName() const;

    void authenticate(qint64 adminId,
                      const QString& account,
                      const QString& displayName);
    void clear();

signals:
    void changed();
    void authenticationChanged(bool authenticated);

private:
    qint64 adminId_ = 0;
    QString account_;
    QString displayName_;
};
