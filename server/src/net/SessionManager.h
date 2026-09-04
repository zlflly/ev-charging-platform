#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <QObject>
#include <QMap>
#include <QTcpSocket>

namespace net {

/**
 * 会话管理器
 * 负责维护 TCP 连接 → userId/adminId 的绑定关系
 */
class SessionManager : public QObject {
    Q_OBJECT

public:
    static SessionManager& instance();

    // 绑定用户到连接
    void bindUser(QTcpSocket* socket, int userId);

    // 绑定管理员到连接
    void bindAdmin(QTcpSocket* socket, int adminId);

    // 获取连接绑定的用户ID（未绑定返回0）
    int getUserId(QTcpSocket* socket) const;

    // 获取连接绑定的管理员ID（未绑定返回0）
    int getAdminId(QTcpSocket* socket) const;

    // 检查连接是否已登录用户
    bool isUserLoggedIn(QTcpSocket* socket) const;

    // 检查连接是否已登录管理员
    bool isAdminLoggedIn(QTcpSocket* socket) const;

    // 清除连接会话（断开时调用）
    void clearSession(QTcpSocket* socket);

private:
    SessionManager() = default;
    ~SessionManager() = default;
    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;

    QMap<QTcpSocket*, int> m_userSessions;   // socket → userId
    QMap<QTcpSocket*, int> m_adminSessions;  // socket → adminId
};

} // namespace net

#endif // SESSION_MANAGER_H
