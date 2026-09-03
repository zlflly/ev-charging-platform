#pragma once

#include "protocol/Protocol.h"

#include <QByteArray>
#include <QHash>
#include <QObject>
#include <QTcpSocket>
#include <QTimer>

#include <functional>

class QJsonObject;

// 整个管理员客户端只维护一个 NetworkClient。
// 页面只发送 action + data，不接触 TCP framing、粘包或 JSON 解析。
class NetworkClient final : public QObject
{
    Q_OBJECT

public:
    using ResponseCallback = std::function<void(const protocol::Response&)>;

    explicit NetworkClient(QObject* parent = nullptr);
    ~NetworkClient() override;

    void connectToServer(const QString& host, quint16 port);
    void disconnectFromServer();
    bool isConnected() const;

    QString sendRequest(const QString& action,
                        const QJsonObject& data,
                        ResponseCallback callback,
                        int timeoutMs = protocol::kDefaultRequestTimeoutMs);

signals:
    void connected();
    void disconnected();
    void transportError(int code, const QString& message);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onSocketError(QAbstractSocket::SocketError error);

private:
    void processReceiveBuffer();
    void failPendingRequests(int code, const QString& message);
    void scheduleReconnect();
    QString nextRequestId();

    QTcpSocket* socket_ = nullptr;
    QTimer* reconnectTimer_ = nullptr;
    QByteArray receiveBuffer_;
    QHash<QString, ResponseCallback> pendingRequests_;
    QHash<QString, QTimer*> pendingTimeouts_;
    quint64 requestCounter_ = 0;

    QString lastHost_;
    quint16 lastPort_ = 0;
    int reconnectBackoffMs_ = 0;
    bool everConnected_ = false;
    bool intentionalDisconnect_ = false;
};
