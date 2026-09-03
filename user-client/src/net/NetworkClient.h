#pragma once

#include "protocol/Protocol.h"

#include <QHash>
#include <functional>
#include <QObject>
#include <QTcpSocket>
#include <QTimer>

class QAbstractSocket;

// ============================================================================
// NetworkClient：统一 TCP 网络层（沿用 WSL 参考项目的接口）
//
// 业务页面只调用 sendRequest(action, data, callback)，不接触 socket 缓冲。
// 断线自动重连：曾成功连接过且非用户主动断开时，按 1s/2s/4s/.../15s 退避。
// ============================================================================
class NetworkClient : public QObject
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
    void transportError(int transportErrorCode, const QString& message);

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketErrorOccurred(QAbstractSocket::SocketError socketError);
    void onReadyRead();

private:
    void failPendingRequests(int transportErrorCode, const QString& message);
    void processReceiveBuffer();
    QString nextRequestId();
    void scheduleReconnect();

    QTcpSocket* socket_ = nullptr;
    QByteArray receiveBuffer_;
    QHash<QString, ResponseCallback> pendingRequests_;
    QHash<QString, QTimer*> pendingTimeouts_;
    quint64 requestIdCounter_ = 0;

    QTimer* reconnectTimer_ = nullptr;
    QString lastHost_;
    quint16 lastPort_ = 0;
    int reconnectBackoffMs_ = 0;
    bool everConnected_ = false;
    bool intentionalDisconnect_ = false;
};