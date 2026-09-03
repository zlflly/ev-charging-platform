#include "net/NetworkClient.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QtEndian>

#include <cstring>
#include <utility>

namespace {
constexpr int kReconnectInitialMs = 1000;
constexpr int kReconnectMaxMs = 15000;
}

NetworkClient::NetworkClient(QObject* parent)
    : QObject(parent)
    , socket_(new QTcpSocket(this))
    , reconnectTimer_(new QTimer(this))
{
    connect(socket_, &QTcpSocket::connected, this, &NetworkClient::onConnected);
    connect(socket_, &QTcpSocket::disconnected, this, &NetworkClient::onDisconnected);
    connect(socket_, &QTcpSocket::readyRead, this, &NetworkClient::onReadyRead);
    connect(socket_, &QTcpSocket::errorOccurred, this, &NetworkClient::onSocketError);

    reconnectTimer_->setSingleShot(true);
    connect(reconnectTimer_, &QTimer::timeout, this, [this] {
        if (intentionalDisconnect_ || lastHost_.isEmpty() || isConnected()) {
            return;
        }
        socket_->abort();
        socket_->connectToHost(lastHost_, lastPort_);
    });
}

NetworkClient::~NetworkClient()
{
    socket_->blockSignals(true);
    socket_->abort();
}

void NetworkClient::connectToServer(const QString& host, quint16 port)
{
    lastHost_ = host;
    lastPort_ = port;
    intentionalDisconnect_ = false;
    reconnectTimer_->stop();

    if (isConnected()) {
        emit connected();
        return;
    }

    socket_->abort();
    socket_->connectToHost(host, port);
}

void NetworkClient::disconnectFromServer()
{
    intentionalDisconnect_ = true;
    reconnectTimer_->stop();
    socket_->disconnectFromHost();
}

bool NetworkClient::isConnected() const
{
    return socket_->state() == QAbstractSocket::ConnectedState;
}

QString NetworkClient::sendRequest(const QString& action,
                                   const QJsonObject& data,
                                   ResponseCallback callback,
                                   int timeoutMs)
{
    if (!isConnected()) {
        if (callback) {
            protocol::Response response;
            response.code = protocol::CodeNotConnected;
            response.message = QStringLiteral("未连接到服务器");
            callback(response);
        }
        return {};
    }

    const QString requestId = nextRequestId();
    const QByteArray payload = QJsonDocument(
        protocol::buildEnvelope(action, requestId, data))
                                   .toJson(QJsonDocument::Compact);

    const quint32 bigEndianLength = qToBigEndian<quint32>(
        static_cast<quint32>(payload.size()));
    QByteArray frame;
    frame.reserve(protocol::kFrameLengthPrefixBytes + payload.size());
    frame.append(reinterpret_cast<const char*>(&bigEndianLength),
                 protocol::kFrameLengthPrefixBytes);
    frame.append(payload);

    pendingRequests_.insert(requestId, std::move(callback));

    auto* timeout = new QTimer(this);
    timeout->setSingleShot(true);
    connect(timeout, &QTimer::timeout, this, [this, requestId, timeoutMs] {
        if (!pendingRequests_.contains(requestId)) {
            return;
        }
        protocol::Response response;
        response.requestId = requestId;
        response.code = protocol::CodeRequestTimeout;
        response.message = QStringLiteral("请求超时（%1 ms）").arg(timeoutMs);
        const auto callback = pendingRequests_.take(requestId);
        pendingTimeouts_.remove(requestId);
        if (callback) {
            callback(response);
        }
    });
    connect(timeout, &QTimer::timeout, timeout, &QObject::deleteLater);
    pendingTimeouts_.insert(requestId, timeout);
    timeout->start(timeoutMs);

    socket_->write(frame);
    return requestId;
}

void NetworkClient::onConnected()
{
    receiveBuffer_.clear();
    everConnected_ = true;
    reconnectBackoffMs_ = 0;
    emit connected();
}

void NetworkClient::onDisconnected()
{
    failPendingRequests(protocol::CodeConnectionLost,
                        QStringLiteral("与服务器的连接已断开"));
    emit disconnected();
    scheduleReconnect();
}

void NetworkClient::onReadyRead()
{
    receiveBuffer_.append(socket_->readAll());
    processReceiveBuffer();
}

void NetworkClient::onSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)
    if (!isConnected() && pendingRequests_.isEmpty()) {
        emit transportError(protocol::CodeNotConnected, socket_->errorString());
    }
}

void NetworkClient::processReceiveBuffer()
{
    while (true) {
        if (receiveBuffer_.size() < protocol::kFrameLengthPrefixBytes) {
            return;
        }

        quint32 payloadLength = 0;
        std::memcpy(&payloadLength, receiveBuffer_.constData(),
                    protocol::kFrameLengthPrefixBytes);
        payloadLength = qFromBigEndian<quint32>(payloadLength);

        if (payloadLength > protocol::kMaxPayloadBytes) {
            emit transportError(protocol::CodeBadPayload,
                                QStringLiteral("响应帧长度异常：%1").arg(payloadLength));
            socket_->abort();
            return;
        }

        const qint64 frameSize = protocol::kFrameLengthPrefixBytes
            + static_cast<qint64>(payloadLength);
        if (receiveBuffer_.size() < frameSize) {
            return;
        }

        const QByteArray payload = receiveBuffer_.mid(
            protocol::kFrameLengthPrefixBytes, payloadLength);
        receiveBuffer_.remove(0, static_cast<int>(frameSize));

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            emit transportError(protocol::CodeBadPayload,
                                QStringLiteral("响应 JSON 解析失败：%1")
                                    .arg(parseError.errorString()));
            continue;
        }

        const protocol::Response response = protocol::Response::fromJson(document.object());
        if (response.requestId.isEmpty()) {
            emit transportError(protocol::CodeBadPayload,
                                QStringLiteral("响应缺少 requestId"));
            continue;
        }

        if (auto* timeout = pendingTimeouts_.take(response.requestId)) {
            timeout->stop();
            timeout->deleteLater();
        }
        const auto callback = pendingRequests_.take(response.requestId);
        if (callback) {
            callback(response);
        }
        // 超时后才到达的响应没有 callback，会被安全丢弃。
    }
}

void NetworkClient::failPendingRequests(int code, const QString& message)
{
    auto requests = std::move(pendingRequests_);
    pendingRequests_.clear();

    for (auto* timeout : pendingTimeouts_) {
        timeout->stop();
        timeout->deleteLater();
    }
    pendingTimeouts_.clear();

    for (auto iterator = requests.constBegin(); iterator != requests.constEnd(); ++iterator) {
        if (!iterator.value()) {
            continue;
        }
        protocol::Response response;
        response.requestId = iterator.key();
        response.code = code;
        response.message = message;
        iterator.value()(response);
    }
}

void NetworkClient::scheduleReconnect()
{
    if (intentionalDisconnect_ || !everConnected_ || lastHost_.isEmpty()) {
        return;
    }
    reconnectBackoffMs_ = reconnectBackoffMs_ <= 0
        ? kReconnectInitialMs
        : qMin(reconnectBackoffMs_ * 2, kReconnectMaxMs);
    reconnectTimer_->start(reconnectBackoffMs_);
}

QString NetworkClient::nextRequestId()
{
    return QStringLiteral("admin-%1").arg(++requestCounter_);
}
