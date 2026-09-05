#include "SessionManager.h"
#include <QDebug>

namespace net {

SessionManager& SessionManager::instance() {
    static SessionManager instance;
    return instance;
}

void SessionManager::bindUser(QTcpSocket* socket, int userId) {
    if (!socket) {
        qWarning() << "[SessionManager] Cannot bind user: null socket";
        return;
    }

    m_userSessions[socket] = userId;
    qInfo() << "[SessionManager] User" << userId << "bound to socket" << socket;
}

void SessionManager::bindAdmin(QTcpSocket* socket, int adminId) {
    if (!socket) {
        qWarning() << "[SessionManager] Cannot bind admin: null socket";
        return;
    }

    m_adminSessions[socket] = adminId;
    qInfo() << "[SessionManager] Admin" << adminId << "bound to socket" << socket;
}

int SessionManager::getUserId(QTcpSocket* socket) const {
    return m_userSessions.value(socket, 0);
}

int SessionManager::getAdminId(QTcpSocket* socket) const {
    return m_adminSessions.value(socket, 0);
}

bool SessionManager::isUserLoggedIn(QTcpSocket* socket) const {
    return m_userSessions.contains(socket) && m_userSessions.value(socket) > 0;
}

bool SessionManager::isAdminLoggedIn(QTcpSocket* socket) const {
    return m_adminSessions.contains(socket) && m_adminSessions.value(socket) > 0;
}

void SessionManager::clearSession(QTcpSocket* socket) {
    if (!socket) {
        return;
    }

    bool hadUser = m_userSessions.remove(socket) > 0;
    bool hadAdmin = m_adminSessions.remove(socket) > 0;

    if (hadUser || hadAdmin) {
        qInfo() << "[SessionManager] Session cleared for socket" << socket;
    }
}

} // namespace net
