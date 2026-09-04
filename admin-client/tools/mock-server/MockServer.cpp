#include "protocol/Protocol.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QDateTime>
#include <QHash>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QtEndian>

#include <cstdio>
#include <cmath>
#include <cstring>

namespace {

QByteArray makeFrame(const QJsonObject& object)
{
    const QByteArray payload = QJsonDocument(object).toJson(QJsonDocument::Compact);
    const quint32 length = qToBigEndian<quint32>(static_cast<quint32>(payload.size()));
    QByteArray frame;
    frame.append(reinterpret_cast<const char*>(&length), protocol::kFrameLengthPrefixBytes);
    frame.append(payload);
    return frame;
}

QJsonArray mockChargers()
{
    const int statuses[] = {
        protocol::ChargerStatusIdle,
        protocol::ChargerStatusIdle,
        protocol::ChargerStatusIdle,
        protocol::ChargerStatusIdle,
        protocol::ChargerStatusIdle,
        protocol::ChargerStatusIdle,
        protocol::ChargerStatusCharging,
        protocol::ChargerStatusCharging,
        protocol::ChargerStatusCharging,
        protocol::ChargerStatusFault,
        protocol::ChargerStatusFault,
        protocol::ChargerStatusOffline,
    };
    const QString stationNames[] = {
        QStringLiteral("良乡大学城站"),
        QStringLiteral("中关村科技园站"),
        QStringLiteral("西山运营站"),
    };

    QJsonArray chargers;
    for (int index = 0; index < 12; ++index) {
        const int stationIndex = index / 4;
        const bool fast = index % 2 == 0;
        chargers.append(QJsonObject {
            {QStringLiteral("chargerId"), 1001 + index},
            {QStringLiteral("code"),
             QStringLiteral("CP-%1").arg(index + 1, 3, 10, QChar(u'0'))},
            {QStringLiteral("stationId"), stationIndex + 1},
            {QStringLiteral("stationName"), stationNames[stationIndex]},
            {QStringLiteral("type"), fast ? protocol::ChargerTypeFast
                                           : protocol::ChargerTypeSlow},
            {QStringLiteral("powerKw"), fast ? 120.0 : 7.0},
            {QStringLiteral("status"), statuses[index]},
            {QStringLiteral("totalChargeCount"), 36 + index * 7},
            {QStringLiteral("totalChargeDurationSeconds"),
             5400 + index * 1800},
        });
    }
    return chargers;
}

class MockServer final : public QObject
{
public:
    explicit MockServer(QObject* parent = nullptr)
        : QObject(parent)
    {
        connect(&server_, &QTcpServer::newConnection, this, [this] {
            while (auto* socket = server_.nextPendingConnection()) {
                buffers_.insert(socket, QByteArray{});
                adminIds_.insert(socket, 0);
                connect(socket, &QTcpSocket::readyRead, this, [this, socket] {
                    buffers_[socket].append(socket->readAll());
                    process(socket);
                });
                connect(socket, &QTcpSocket::disconnected, this, [this, socket] {
                    buffers_.remove(socket);
                    adminIds_.remove(socket);
                    socket->deleteLater();
                });
            }
        });
    }

    bool listen()
    {
        return server_.listen(QHostAddress::LocalHost, 9000);
    }

private:
    void process(QTcpSocket* socket)
    {
        QByteArray& buffer = buffers_[socket];
        while (buffer.size() >= protocol::kFrameLengthPrefixBytes) {
            quint32 payloadLength = 0;
            std::memcpy(&payloadLength, buffer.constData(), protocol::kFrameLengthPrefixBytes);
            payloadLength = qFromBigEndian<quint32>(payloadLength);
            if (payloadLength > protocol::kMaxPayloadBytes) {
                socket->abort();
                return;
            }
            const qint64 frameSize = protocol::kFrameLengthPrefixBytes
                + static_cast<qint64>(payloadLength);
            if (buffer.size() < frameSize) {
                return;
            }
            const QByteArray payload = buffer.mid(protocol::kFrameLengthPrefixBytes,
                                                  payloadLength);
            buffer.remove(0, static_cast<int>(frameSize));
            handle(socket, QJsonDocument::fromJson(payload).object());
        }
    }

    void handle(QTcpSocket* socket, const QJsonObject& request)
    {
        const QString requestId = request.value(QStringLiteral("requestId")).toString();
        const QString action = request.value(QStringLiteral("action")).toString();
        const QJsonObject requestData = request.value(QStringLiteral("data")).toObject();

        QJsonObject response {
            {QStringLiteral("requestId"), requestId},
            {QStringLiteral("code"), protocol::CodeOk},
            {QStringLiteral("message"), QStringLiteral("ok")},
        };
        if (action == QString::fromUtf8(protocol::action::kPing)) {
            response.insert(QStringLiteral("data"), requestData);
        } else if (action == QString::fromUtf8(protocol::action::kAdminLogin)) {
            const QString account =
                requestData.value(QStringLiteral("account")).toString().trimmed();
            const QString password =
                requestData.value(QStringLiteral("password")).toString();
            if (account.isEmpty() || password.isEmpty()) {
                response.insert(QStringLiteral("code"), protocol::CodeBadRequest);
                response.insert(QStringLiteral("message"),
                                QStringLiteral("account and password are required"));
            } else if (account != QStringLiteral("admin")
                       || password != QStringLiteral("123456")) {
                response.insert(QStringLiteral("code"),
                                protocol::CodeInvalidAdminCredentials);
                response.insert(QStringLiteral("message"),
                                QStringLiteral("invalid administrator credentials"));
            } else {
                adminIds_[socket] = 1;
                response.insert(QStringLiteral("data"), QJsonObject {
                    {QStringLiteral("adminId"), 1},
                    {QStringLiteral("account"), QStringLiteral("admin")},
                    {QStringLiteral("displayName"), QStringLiteral("系统管理员")},
                });
            }
        } else if (action == QString::fromUtf8(
                       protocol::action::kAdminChargerOverview)) {
            if (adminIds_.value(socket) <= 0) {
                response.insert(QStringLiteral("code"), protocol::CodeNotLoggedIn);
                response.insert(QStringLiteral("message"),
                                QStringLiteral("administrator login required"));
            } else {
                // 仅用于客户端联调；真实数量和占比必须由成员 1 的服务端聚合。
                response.insert(QStringLiteral("data"), QJsonObject {
                    {QStringLiteral("total"), 12},
                    {QStringLiteral("idle"), 6},
                    {QStringLiteral("charging"), 3},
                    {QStringLiteral("fault"), 2},
                    {QStringLiteral("offline"), 1},
                    {QStringLiteral("idlePercent"), 50.0},
                    {QStringLiteral("chargingPercent"), 25.0},
                    {QStringLiteral("faultPercent"), 16.7},
                    {QStringLiteral("offlinePercent"), 8.3},
                    {QStringLiteral("updatedAt"),
                     QDateTime::currentMSecsSinceEpoch()},
                });
            }
        } else if (action == QString::fromUtf8(
                       protocol::action::kAdminChargerList)) {
            if (adminIds_.value(socket) <= 0) {
                response.insert(QStringLiteral("code"), protocol::CodeNotLoggedIn);
                response.insert(QStringLiteral("message"),
                                QStringLiteral("administrator login required"));
            } else {
                response.insert(QStringLiteral("data"), QJsonObject {
                    {QStringLiteral("chargers"), mockChargers()},
                });
            }
        } else if (action == QString::fromUtf8(
                       protocol::action::kAdminChargerRestart)) {
            if (adminIds_.value(socket) <= 0) {
                response.insert(QStringLiteral("code"), protocol::CodeNotLoggedIn);
                response.insert(QStringLiteral("message"),
                                QStringLiteral("administrator login required"));
            } else {
                const QJsonValue chargerIdValue =
                    requestData.value(QStringLiteral("chargerId"));
                const double chargerIdNumber = chargerIdValue.toDouble();
                const bool validChargerId = chargerIdValue.isDouble()
                    && std::isfinite(chargerIdNumber)
                    && chargerIdNumber >= 1.0
                    && chargerIdNumber <= 9007199254740991.0
                    && std::floor(chargerIdNumber) == chargerIdNumber;
                const qint64 chargerId = validChargerId
                    ? static_cast<qint64>(chargerIdNumber) : 0;
                QJsonObject selected;
                if (validChargerId) {
                    const QJsonArray chargers = mockChargers();
                    for (const QJsonValue value : chargers) {
                        const QJsonObject charger = value.toObject();
                        if (static_cast<qint64>(charger.value(
                                QStringLiteral("chargerId")).toDouble()) == chargerId) {
                            selected = charger;
                            break;
                        }
                    }
                }

                if (!validChargerId) {
                    response.insert(QStringLiteral("code"), protocol::CodeBadRequest);
                    response.insert(QStringLiteral("message"),
                                    QStringLiteral("chargerId must be a positive integer"));
                } else if (selected.isEmpty()) {
                    response.insert(QStringLiteral("code"), protocol::CodeBadRequest);
                    response.insert(QStringLiteral("message"),
                                    QStringLiteral("充电桩不存在"));
                } else if (selected.value(QStringLiteral("status")).toInt()
                           == protocol::ChargerStatusCharging) {
                    response.insert(QStringLiteral("code"),
                                    protocol::CodeChargerOperationRejected);
                    response.insert(QStringLiteral("message"),
                                    QStringLiteral("充电桩正在服务订单，禁止远程重启"));
                } else {
                    response.insert(QStringLiteral("message"),
                                    QStringLiteral("服务器已接受重启指令"));
                    response.insert(QStringLiteral("data"), QJsonObject {
                        {QStringLiteral("chargerId"), chargerId},
                        {QStringLiteral("restartedAt"),
                         QDateTime::currentMSecsSinceEpoch()},
                    });
                }
            }
        } else {
            response.insert(QStringLiteral("code"), 1001);
            response.insert(QStringLiteral("message"), QStringLiteral("unsupported action"));
        }

        // 反向延迟 PING，强制客户端在乱序响应下按 requestId 匹配。
        const QString echo = requestData.value(QStringLiteral("echo")).toString();
        const int suffix = echo.right(1).toInt();
        const int delayMs = echo.isEmpty() ? 0 : (5 - suffix) * 25;
        const QPointer<QTcpSocket> guardedSocket(socket);
        QTimer::singleShot(delayMs, this, [guardedSocket, response] {
            if (guardedSocket) {
                guardedSocket->write(makeFrame(response));
            }
        });
    }

    QTcpServer server_;
    QHash<QTcpSocket*, QByteArray> buffers_;
    QHash<QTcpSocket*, qint64> adminIds_;
};

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    MockServer server;
    if (!server.listen()) {
        std::fprintf(stderr, "failed to listen on 127.0.0.1:9000\n");
        return 1;
    }
    std::printf("admin mock server listening on 127.0.0.1:9000\n");
    std::fflush(stdout);
    return application.exec();
}
