#pragma once

#include <QObject>
#include <QString>

// ============================================================================
// Session：登录态唯一来源（单例）。
// 沿用 WSL 参考项目的接口；登录成功后由 LoginPage 写入。
// ============================================================================
class Session : public QObject
{
    Q_OBJECT

public:
    static Session& instance()
    {
        static Session session;
        return session;
    }

    bool isLoggedIn() const { return userId_ > 0; }

    qint64 userId() const { return userId_; }
    QString phone() const { return phone_; }
    QString nickname() const { return nickname_; }
    QString avatarUrl() const { return avatarUrl_; }
    double balance() const { return balance_; }
    QString status() const { return status_; }

    bool hasLocation() const { return hasLocation_; }
    double latitude() const { return latitude_; }
    double longitude() const { return longitude_; }
    QString locationLabel() const { return locationLabel_; }

    void setUser(qint64 userId, const QString& phone, const QString& nickname,
                 const QString& avatarUrl, double balance, const QString& status);
    void setLocation(double latitude, double longitude, const QString& label);
    void setNickname(const QString& nickname);
    void setAvatarUrl(const QString& avatarUrl);
    void setBalance(double balance);
    void logout();

signals:
    void userChanged();

private:
    Session() = default;

    qint64 userId_ = 0;
    QString phone_;
    QString nickname_;
    QString avatarUrl_;
    double balance_ = 0.0;
    QString status_;

    bool hasLocation_ = false;
    double latitude_ = 0.0;
    double longitude_ = 0.0;
    QString locationLabel_;
};