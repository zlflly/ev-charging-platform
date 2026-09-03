#include "net/NetworkClient.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QtEndian>
#include <cstring>

using protocol::Response;

namespace {

// 重连退避：从 1s 起倍增，封顶 15s
constexpr int kReconnectInitialMs = 1000;
constexpr int kReconnectMaxMs = 15000;

} // namespace

NetworkClient::NetworkClient(QObject* parent)
    : QObject(parent)
{
    socket_ = new QTcpSocket(this);
    connect(socket_, &QTcpSocket::connected,
            this, &NetworkClient::onSocketConnected);
    connect(socket_, &QTcpSocket::disconnected,
            this, &NetworkClient::onSocketDisconnected);
    connect(socket_, &QAbstractSocket::errorOccurred,
            this, &NetworkClient::onSocketErrorOccurred);
    connect(socket_, &QTcpSocket::readyRead,
            this, &NetworkClient::onReadyRead);

    reconnectTimer_ = new QTimer(this);
    reconnectTimer_->setSingleShot(true);
    connect(reconnectTimer_, &QTimer::timeout, this, [this] {
        if (intentionalDisconnect_ ||
            socket_->state() == QAbstractSocket::ConnectedState) {
            return;
        }
        socket_->abort();
        socket_->connectToHost(lastHost_, lastPort_);
    });
}

NetworkClient::~NetworkClient()
{
    if (socket_) {
        socket_->blockSignals(true);
        socket_->abort();
    }
}

void NetworkClient::connectToServer(const QString& host, quint16 port)
{
    lastHost_ = host;
    lastPort_ = port;
    intentionalDisconnect_ = false;
    reconnectTimer_->stop();
    if (socket_->state() == QAbstractSocket::ConnectedState) {
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
        Response response;
        response.code = protocol::CodeNotConnected;
        response.message = QStringLiteral("未连接到服务器");
        callback(response);
        return {};
    }

    const QString requestId = nextRequestId();
    const QJsonDocument document(protocol::buildEnvelope(action, requestId, data));
    const QByteArray payload = document.toJson(QJsonDocument::Compact);

    QByteArray frame;
    frame.reserve(protocol::kFrameLengthPrefixBytes + payload.size());
    quint32 payloadLength = qToBigEndian<quint32>(static_cast<quint32>(payload.size()));
    frame.append(reinterpret_cast<const char*>(&payloadLength),
                 protocol::kFrameLengthPrefixBytes);
    frame.append(payload);

    pendingRequests_.insert(requestId, std::move(callback));

    auto* timeoutTimer = new QTimer(this);
    timeoutTimer->setSingleShot(true);
    connect(timeoutTimer, &QTimer::timeout, this, [this, requestId, timeoutMs] {
        if (!pendingRequests_.contains(requestId)) {
            return;
        }
        Response response;
        response.requestId = requestId;
        response.code = protocol::CodeRequestTimeout;
        response.message = QStringLiteral("请求超时（%1 ms）").arg(timeoutMs);
        if (auto callback = pendingRequests_.take(requestId)) {
            callback(response);
        }
        if (auto* timer = pendingTimeouts_.take(requestId)) {
            timer->deleteLater();
        }
    });
    pendingTimeouts_.insert(requestId, timeoutTimer);
    timeoutTimer->start(timeoutMs);

    socket_->write(frame);
    return requestId;
}

void NetworkClient::onSocketConnected()
{
    receiveBuffer_.clear();
    everConnected_ = true;
    reconnectBackoffMs_ = 0;
    emit connected();
}

void NetworkClient::onSocketDisconnected()
{
    failPendingRequests(protocol::CodeConnectionLost,
                        QStringLiteral("与服务器连接已断开"));
    emit disconnected();
    scheduleReconnect();
}

void NetworkClient::scheduleReconnect()
{
    if (intentionalDisconnect_ || !everConnected_ ||
        lastHost_.isEmpty()) {
        return;
    }
    reconnectBackoffMs_ = reconnectBackoffMs_ <= 0
        ? kReconnectInitialMs
        : qMin(reconnectBackoffMs_ * 2, kReconnectMaxMs);
    reconnectTimer_->start(reconnectBackoffMs_);
}

void NetworkClient::onSocketErrorOccurred(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    if (socket_->state() != QAbstractSocket::ConnectedState &&
        pendingRequests_.isEmpty()) {
        emit transportError(protocol::CodeNotConnected, socket_->errorString());
    }
}

void NetworkClient::onReadyRead()
{
    receiveBuffer_.append(socket_->readAll());
    processReceiveBuffer();
}

void NetworkClient::failPendingRequests(int transportErrorCode, const QString& message)
{
    auto requests = std::move(pendingRequests_);
    pendingRequests_.clear();
    for (auto* timer : pendingTimeouts_) {
        timer->stop();
        timer->deleteLater();
    }
    pendingTimeouts_.clear();

    for (auto it = requests.constBegin(); it != requests.constEnd(); ++it) {
        Response response;
        response.requestId = it.key();
        response.code = transportErrorCode;
        response.message = message;
        if (it.value()) {
            it.value()(response);
        }
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
                                QStringLiteral("响应帧长度异常: %1").arg(payloadLength));
            socket_->abort();
            return;
        }

        const qint64 frameSize = protocol::kFrameLengthPrefixBytes +
                                 static_cast<qint64>(payloadLength);
        if (receiveBuffer_.size() < frameSize) {
            return;
        }

        const QByteArray payload = receiveBuffer_.mid(
            protocol::kFrameLengthPrefixBytes, static_cast<qint64>(payloadLength));
        receiveBuffer_.remove(0, static_cast<int>(frameSize));

        QJsonParseError parseError{};
        const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            emit transportError(protocol::CodeBadPayload,
                                QStringLiteral("响应 JSON 解析失败: %1")
                                    .arg(parseError.errorString()));
            continue;
        }

        const Response response = Response::fromJson(document.object());

        if (auto* timer = pendingTimeouts_.take(response.requestId)) {
            timer->stop();
            timer->deleteLater();
        }
        if (auto callback = pendingRequests_.take(response.requestId)) {
            callback(response);
        }
    }
}

QString NetworkClient::nextRequestId()
{
    ++requestIdCounter_;
    return QStringLiteral("req-%1").arg(requestIdCounter_);
}