#pragma once

// ============================================================================
// TcpServer：监听端口、接受连接、管理所有 TcpConnection 的生命周期
// ============================================================================

#include <QHostAddress>
#include <QObject>
#include <QSet>
#include <QTcpServer>

#include "config/ServerConfig.h"
#include "net/TcpConnection.h"
#include "net/SessionManager.h"

namespace net {

class TcpServer : public QObject {
    Q_OBJECT

public:
    explicit TcpServer(const config::ServerConfig& serverConfig, QObject* parent = nullptr)
        : QObject(parent), m_config(serverConfig)
    {
        m_server = new QTcpServer(this);
        connect(m_server, &QTcpServer::newConnection, this, &TcpServer::onNewConnection);
    }

    bool start()
    {
        const QHostAddress address(m_config.host);
        if (!m_server->listen(address, m_config.port)) {
            qCritical() << "[TcpServer] listen on" << m_config.host << m_config.port
                        << "failed:" << m_server->errorString();
            return false;
        }
        qInfo() << "[TcpServer] listening on"
                << m_server->serverAddress().toString() << m_server->serverPort();
        return true;
    }

    void stop()
    {
        m_server->close();
        const auto connections = m_connections;
        for (TcpConnection* connection : connections) {
            connection->close();
        }
    }

    int activeConnectionCount() const { return m_connections.size(); }

signals:
    void requestReceived(const protocol::Request& request, net::TcpConnection* connection);

private slots:
    void onNewConnection()
    {
        while (QTcpSocket* socket = m_server->nextPendingConnection()) {
            if (m_connections.size() >= m_config.maxConnections) {
                qWarning() << "[TcpServer] connection limit" << m_config.maxConnections
                           << "reached, rejecting" << socket->peerAddress().toString();
                socket->disconnectFromHost();
                socket->deleteLater();
                continue;
            }

            auto* connection = new TcpConnection(socket, this);
            m_connections.insert(connection);

            connect(connection, &TcpConnection::requestReceived,
                    this, &TcpServer::requestReceived);
            connect(connection, &TcpConnection::connectionClosed,
                    this, &TcpServer::onConnectionClosed);
        }
    }

    void onConnectionClosed(net::TcpConnection* connection)
    {
        // 清除会话
        SessionManager::instance().clearSession(connection->socket());

        m_connections.remove(connection);
        qInfo() << "[TcpServer] active connections:" << m_connections.size();
    }

private:
    config::ServerConfig m_config;
    QTcpServer* m_server = nullptr;
    QSet<TcpConnection*> m_connections;
};

} // namespace net
