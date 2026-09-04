#pragma once

// ============================================================================
// TcpConnection：单个客户端连接的封装
//
// 职责：
// - 管理 QTcpSocket 的生命周期
// - 使用 FrameCodec 处理帧的收发（粘包/半包由 FrameCodec 兜底）
// - 解析 JSON 请求外壳，分发给 RequestDispatcher
// - 持有本连接绑定的 userId / adminId（协议无 token，身份靠连接绑定）
// ============================================================================

#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QTcpSocket>

#include "net/FrameCodec.h"
#include "protocol/Protocol.h"

namespace net {

class TcpConnection : public QObject {
    Q_OBJECT

public:
    // socket 由本对象接管（reparent），调用方不要再自行 delete。
    explicit TcpConnection(QTcpSocket* socket, QObject* parent = nullptr)
        : QObject(parent), m_socket(socket)
    {
        m_socket->setParent(this);
        m_peerAddress = QStringLiteral("%1:%2")
                            .arg(m_socket->peerAddress().toString())
                            .arg(m_socket->peerPort());

        connect(m_socket, &QTcpSocket::readyRead, this, &TcpConnection::onReadyRead);
        connect(m_socket, &QTcpSocket::disconnected, this, &TcpConnection::onDisconnected);
        connect(m_socket, &QTcpSocket::errorOccurred, this, &TcpConnection::onError);

        qInfo() << "[Connection]" << m_peerAddress << "connected";
    }

    // 对端地址，仅用于日志。socket 断开后 peerAddress() 会失效，故构造时缓存。
    QString peerAddress() const { return m_peerAddress; }

    // 身份绑定：登录成功后由业务层调用
    void bindUser(int userId) { m_userId = userId; }
    void bindAdmin(int adminId) { m_adminId = adminId; }

    int userId() const { return m_userId; }
    int adminId() const { return m_adminId; }
    bool isUserLoggedIn() const { return m_userId > 0; }
    bool isAdminLoggedIn() const { return m_adminId > 0; }

    void sendResponse(const QJsonObject& response)
    {
        const QByteArray payload = QJsonDocument(response).toJson(QJsonDocument::Compact);

        QByteArray frame;
        if (FrameCodec::encode(payload, frame) != EncodeOk) {
            qWarning() << "[Connection]" << m_peerAddress
                       << "response dropped: payload exceeds" << protocol::kMaxPayloadBytes;
            return;
        }
        m_socket->write(frame);
    }

    void close() { m_socket->disconnectFromHost(); }

signals:
    void requestReceived(const protocol::Request& request, net::TcpConnection* connection);
    void connectionClosed(net::TcpConnection* connection);

private slots:
    void onReadyRead()
    {
        m_receiveBuffer.append(m_socket->readAll());

        // 一次 readyRead 可能带来任意个完整帧（粘包），循环取干净为止。
        while (true) {
            QByteArray payload;
            int consumedBytes = 0;
            const DecodeResult result =
                FrameCodec::decode(m_receiveBuffer, payload, consumedBytes);

            if (result == DecodeNeedMoreData) {
                break;
            }
            if (result != DecodeOk) {
                qWarning() << "[Connection]" << m_peerAddress
                           << "framing error" << result << "- closing connection";
                close();
                return;
            }

            m_receiveBuffer.remove(0, consumedBytes);
            handlePayload(payload);
        }
    }

    void onDisconnected()
    {
        qInfo() << "[Connection]" << m_peerAddress << "disconnected";
        emit connectionClosed(this);
        deleteLater();
    }

    void onError(QAbstractSocket::SocketError socketError)
    {
        // RemoteHostClosedError 是正常断开，disconnected 会紧随其后。
        if (socketError == QAbstractSocket::RemoteHostClosedError) {
            return;
        }
        qWarning() << "[Connection]" << m_peerAddress << "socket error:" << m_socket->errorString();
    }

private:
    void handlePayload(const QByteArray& payload)
    {
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
        if (!document.isObject()) {
            qWarning() << "[Connection]" << m_peerAddress
                       << "invalid JSON:" << parseError.errorString();
            // requestId 未知，只能回空串；客户端按外壳解析失败处理。
            sendResponse(protocol::buildErrorResponse(
                QString(), protocol::CodeBadRequest,
                QStringLiteral("invalid JSON payload")));
            return;
        }

        const protocol::Request request = protocol::Request::fromJson(document.object());
        if (!request.isValid()) {
            sendResponse(protocol::buildErrorResponse(
                request.requestId, protocol::CodeBadRequest,
                QStringLiteral("missing action or requestId")));
            return;
        }

        emit requestReceived(request, this);
    }

    QTcpSocket* m_socket = nullptr;
    QString m_peerAddress;
    QByteArray m_receiveBuffer;

    int m_userId = 0;   // 0 = 未登录
    int m_adminId = 0;  // 0 = 未登录
};

} // namespace net
