#include "session/Session.h"

void Session::setUser(qint64 userId, const QString& phone, const QString& nickname,
                      const QString& avatarUrl, double balance, const QString& status)
{
    userId_ = userId;
    phone_ = phone;
    nickname_ = nickname;
    avatarUrl_ = avatarUrl;
    balance_ = balance;
    status_ = status;
    emit userChanged();
}

void Session::setLocation(double latitude, double longitude, const QString& label)
{
    latitude_ = latitude;
    longitude_ = longitude;
    locationLabel_ = label;
    hasLocation_ = true;
    emit userChanged();
}

void Session::setNickname(const QString& nickname)
{
    if (nickname_ == nickname) {
        return;
    }
    nickname_ = nickname;
    emit userChanged();
}

void Session::setAvatarUrl(const QString& avatarUrl)
{
    if (avatarUrl_ == avatarUrl) {
        return;
    }
    avatarUrl_ = avatarUrl;
    emit userChanged();
}

void Session::setBalance(double balance)
{
    if (qFuzzyCompare(balance_, balance)) {
        return;
    }
    balance_ = balance;
    emit userChanged();
}

void Session::logout()
{
    userId_ = 0;
    phone_.clear();
    nickname_.clear();
    avatarUrl_.clear();
    balance_ = 0.0;
    status_.clear();
    hasLocation_ = false;
    latitude_ = 0.0;
    longitude_ = 0.0;
    locationLabel_.clear();
    emit userChanged();
}