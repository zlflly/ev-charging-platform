#include "session/AdminSession.h"

AdminSession::AdminSession(QObject* parent)
    : QObject(parent)
{
}

bool AdminSession::isAuthenticated() const
{
    return adminId_ > 0;
}

qint64 AdminSession::adminId() const
{
    return adminId_;
}

QString AdminSession::account() const
{
    return account_;
}

QString AdminSession::displayName() const
{
    return displayName_;
}

void AdminSession::authenticate(qint64 adminId,
                                const QString& account,
                                const QString& displayName)
{
    const bool wasAuthenticated = isAuthenticated();
    adminId_ = adminId;
    account_ = account;
    displayName_ = displayName;
    emit changed();
    if (wasAuthenticated != isAuthenticated()) {
        emit authenticationChanged(isAuthenticated());
    }
}

void AdminSession::clear()
{
    if (!isAuthenticated() && account_.isEmpty() && displayName_.isEmpty()) {
        return;
    }

    const bool wasAuthenticated = isAuthenticated();
    adminId_ = 0;
    account_.clear();
    displayName_.clear();
    emit changed();
    if (wasAuthenticated) {
        emit authenticationChanged(false);
    }
}
