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
#include <QRegularExpression>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QTimeZone>
#include <QtEndian>

#include <cstdio>
#include <cmath>
#include <cstring>
#include <limits>

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
            {QStringLiteral("pricePerKwh"), fast ? 1.20 : 0.80},
        });
    }
    return chargers;
}

QJsonArray mockStations()
{
    return {
        QJsonObject {
            {QStringLiteral("stationId"), 1},
            {QStringLiteral("name"), QStringLiteral("良乡大学城站")},
            {QStringLiteral("address"), QStringLiteral("北京市房山区良乡大学城北路")},
            {QStringLiteral("latitude"), 39.731320},
            {QStringLiteral("longitude"), 116.171590},
            {QStringLiteral("pricePerKwh"), 1.50},
            {QStringLiteral("version"), 1},
        },
        QJsonObject {
            {QStringLiteral("stationId"), 2},
            {QStringLiteral("name"), QStringLiteral("中关村科技园站")},
            {QStringLiteral("address"), QStringLiteral("北京市海淀区中关村大街")},
            {QStringLiteral("latitude"), 39.983420},
            {QStringLiteral("longitude"), 116.316510},
            {QStringLiteral("pricePerKwh"), 1.80},
            {QStringLiteral("version"), 1},
        },
        QJsonObject {
            {QStringLiteral("stationId"), 3},
            {QStringLiteral("name"), QStringLiteral("西山运营站")},
            {QStringLiteral("address"), QStringLiteral("北京市海淀区西山产业园")},
            {QStringLiteral("latitude"), 39.994610},
            {QStringLiteral("longitude"), 116.196780},
            {QStringLiteral("pricePerKwh"), 1.60},
            {QStringLiteral("version"), 1},
        },
        QJsonObject {
            {QStringLiteral("stationId"), 4},
            {QStringLiteral("name"), QStringLiteral("亦庄待投运站")},
            {QStringLiteral("address"), QStringLiteral("北京市大兴区亦庄新城")},
            {QStringLiteral("latitude"), 39.794310},
            {QStringLiteral("longitude"), 116.506840},
            {QStringLiteral("pricePerKwh"), 1.30},
            {QStringLiteral("version"), 1},
        },
    };
}

QJsonArray mockUsers()
{
    const auto idle = [] {
        return QJsonValue(QJsonValue::Null);
    };
    const auto active = [](int orderId, int stationId, const QString& stationName,
                           int chargerId, const QString& chargerCode,
                           const QString& status) {
        return QJsonObject {
            {QStringLiteral("orderId"), orderId},
            {QStringLiteral("stationId"), stationId},
            {QStringLiteral("stationName"), stationName},
            {QStringLiteral("chargerId"), chargerId},
            {QStringLiteral("chargerCode"), chargerCode},
            {QStringLiteral("status"), status},
        };
    };
    return {
        QJsonObject {{QStringLiteral("userId"), 1}, {QStringLiteral("phone"), QStringLiteral("13800138001")}, {QStringLiteral("nickname"), QStringLiteral("用户8001")}, {QStringLiteral("balance"), 50.00}, {QStringLiteral("createdAt"), 1788240600000.0}, {QStringLiteral("status"), protocol::UserStatusNormal}, {QStringLiteral("activityStatus"), QStringLiteral("IDLE")}, {QStringLiteral("activeOrder"), idle()}},
        QJsonObject {{QStringLiteral("userId"), 2}, {QStringLiteral("phone"), QStringLiteral("13800138002")}, {QStringLiteral("nickname"), QStringLiteral("用户8002")}, {QStringLiteral("balance"), 100.50}, {QStringLiteral("createdAt"), 1788331200000.0}, {QStringLiteral("status"), protocol::UserStatusNormal}, {QStringLiteral("activityStatus"), QStringLiteral("IDLE")}, {QStringLiteral("activeOrder"), idle()}},
        QJsonObject {{QStringLiteral("userId"), 3}, {QStringLiteral("phone"), QStringLiteral("13800138003")}, {QStringLiteral("nickname"), QStringLiteral("充电中用户")}, {QStringLiteral("balance"), 150.00}, {QStringLiteral("createdAt"), 1788421800000.0}, {QStringLiteral("status"), protocol::UserStatusNormal}, {QStringLiteral("activityStatus"), QStringLiteral("CHARGING")}, {QStringLiteral("activeOrder"), active(9003, 2, QStringLiteral("中关村科技园站"), 1007, QStringLiteral("CP-007"), QStringLiteral("CHARGING"))}},
        QJsonObject {{QStringLiteral("userId"), 4}, {QStringLiteral("phone"), QStringLiteral("13912345678")}, {QStringLiteral("nickname"), QStringLiteral("风控样例")}, {QStringLiteral("balance"), 0.00}, {QStringLiteral("createdAt"), 1788426000000.0}, {QStringLiteral("status"), protocol::UserStatusFrozen}, {QStringLiteral("activityStatus"), QStringLiteral("IDLE")}, {QStringLiteral("activeOrder"), idle()}},
        QJsonObject {{QStringLiteral("userId"), 5}, {QStringLiteral("phone"), QStringLiteral("18688886666")}, {QStringLiteral("nickname"), QStringLiteral("待结算用户")}, {QStringLiteral("balance"), 28.80}, {QStringLiteral("createdAt"), 1788470700000.0}, {QStringLiteral("status"), protocol::UserStatusNormal}, {QStringLiteral("activityStatus"), QStringLiteral("WAIT_SETTLEMENT")}, {QStringLiteral("activeOrder"), active(9005, 1, QStringLiteral("良乡大学城站"), 1002, QStringLiteral("CP-002"), QStringLiteral("WAIT_SETTLEMENT"))}},
        QJsonObject {{QStringLiteral("userId"), 6}, {QStringLiteral("phone"), QStringLiteral("15100001234")}, {QStringLiteral("nickname"), QStringLiteral("测试用户")}, {QStringLiteral("balance"), 9.90}, {QStringLiteral("createdAt"), 1788480900000.0}, {QStringLiteral("status"), protocol::UserStatusNormal}, {QStringLiteral("activityStatus"), QStringLiteral("IDLE")}, {QStringLiteral("activeOrder"), idle()}},
    };
}

QJsonArray mockOrders()
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const auto order = [](int orderId, int userId, const QString& phone,
                          const QString& nickname, const QString& status,
                          const QString& paymentStatus, const QString& amountKind,
                          int stationId, const QString& stationName, int chargerId,
                          const QString& chargerCode, int type, double powerKw,
                          double price, double energy, double amount, qint64 createdAt,
                          qint64 startTime, qint64 stopTime, qint64 settleTime,
                          int durationSeconds) {
        return QJsonObject {
            {QStringLiteral("orderId"), orderId},
            {QStringLiteral("userId"), userId},
            {QStringLiteral("phone"), phone},
            {QStringLiteral("nickname"), nickname},
            {QStringLiteral("status"), status},
            {QStringLiteral("paymentStatus"), paymentStatus},
            {QStringLiteral("amountKind"), amountKind},
            {QStringLiteral("stationId"), stationId},
            {QStringLiteral("stationName"), stationName},
            {QStringLiteral("chargerId"), chargerId},
            {QStringLiteral("chargerCode"), chargerCode},
            {QStringLiteral("type"), type},
            {QStringLiteral("powerKw"), powerKw},
            {QStringLiteral("pricePerKwh"), price},
            {QStringLiteral("energyKwh"), energy},
            {QStringLiteral("amount"), amount},
            {QStringLiteral("createdAt"), static_cast<double>(createdAt)},
            {QStringLiteral("startTime"), static_cast<double>(startTime)},
            {QStringLiteral("stopTime"), static_cast<double>(stopTime)},
            {QStringLiteral("settleTime"), static_cast<double>(settleTime)},
            {QStringLiteral("durationSeconds"), durationSeconds},
        };
    };
    return {
        order(9010, 6, QStringLiteral("15100001234"), QStringLiteral("测试用户"),
              QStringLiteral("RESERVED"), QStringLiteral("NOT_DUE"), QStringLiteral("NONE"),
              3, QStringLiteral("西山运营站"), 1009, QStringLiteral("CP-009"),
              protocol::ChargerTypeFast, 120.0, 1.60, 0.0, 0.0,
              now - 600000, 0, 0, 0, 0),
        order(9009, 3, QStringLiteral("13800138003"), QStringLiteral("充电中用户"),
              QStringLiteral("CHARGING"), QStringLiteral("NOT_DUE"), QStringLiteral("ESTIMATED"),
              2, QStringLiteral("中关村科技园站"), 1007, QStringLiteral("CP-007"),
              protocol::ChargerTypeFast, 120.0, 1.80, 176.0, 316.80,
              now - 7200000, now - 6600000, 0, 0, 6600),
        order(9008, 5, QStringLiteral("18688886666"), QStringLiteral("待结算用户"),
              QStringLiteral("WAIT_SETTLEMENT"), QStringLiteral("UNPAID"), QStringLiteral("FINAL"),
              1, QStringLiteral("良乡大学城站"), 1002, QStringLiteral("CP-002"),
              protocol::ChargerTypeSlow, 7.0, 1.50, 11.20, 16.80,
              now - 10800000, now - 10500000, now - 3300000, 0, 7200),
        order(9007, 2, QStringLiteral("13800138002"), QStringLiteral("用户8002"),
              QStringLiteral("FINISHED"), QStringLiteral("PAID"), QStringLiteral("FINAL"),
              1, QStringLiteral("良乡大学城站"), 1001, QStringLiteral("CP-001"),
              protocol::ChargerTypeFast, 120.0, 1.50, 42.40, 63.60,
              now - 86400000, now - 85800000, now - 84000000, now - 83700000, 1800),
        order(9006, 1, QStringLiteral("13800138001"), QStringLiteral("用户8001"),
              QStringLiteral("FINISHED"), QStringLiteral("PAID"), QStringLiteral("FINAL"),
              2, QStringLiteral("中关村科技园站"), 1005, QStringLiteral("CP-005"),
              protocol::ChargerTypeFast, 120.0, 1.80, 53.50, 96.30,
              now - 172800000, now - 172200000, now - 170400000, now - 170100000, 1800),
        order(9005, 6, QStringLiteral("15100001234"), QStringLiteral("测试用户"),
              QStringLiteral("FINISHED"), QStringLiteral("PAID"), QStringLiteral("FINAL"),
              3, QStringLiteral("西山运营站"), 1010, QStringLiteral("CP-010"),
              protocol::ChargerTypeSlow, 7.0, 1.60, 8.00, 12.80,
              now - 259200000, now - 258900000, now - 255300000, now - 255000000, 3600),
    };
}

double mockRevenueForDate(const QDate& date)
{
    if (date.dayOfYear() % 6 == 0) return 0.0;
    return 40.5 + static_cast<double>((date.dayOfYear() * 37) % 190);
}

QJsonArray mockRevenuePoints(int days)
{
    const QDate endDate = QDateTime::currentDateTimeUtc().toTimeZone(
        QTimeZone(QByteArrayLiteral("Asia/Shanghai"))).date();
    QJsonArray points;
    for (int offset = days - 1; offset >= 0; --offset) {
        const QDate date = endDate.addDays(-offset);
        points.append(QJsonObject {
            {QStringLiteral("date"), date.toString(QStringLiteral("yyyy-MM-dd"))},
            {QStringLiteral("revenue"), mockRevenueForDate(date)},
        });
    }
    return points;
}

class MockServer final : public QObject
{
public:
    explicit MockServer(QObject* parent = nullptr)
        : QObject(parent)
        , stations_(mockStations())
        , chargers_(mockChargers())
        , users_(mockUsers())
        , orders_(mockOrders())
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
        return server_.listen(QHostAddress::LocalHost, 8888);
    }

private:
    QJsonObject findStation(qint64 stationId) const
    {
        for (const QJsonValue value : stations_) {
            const QJsonObject station = value.toObject();
            if (static_cast<qint64>(station.value(
                    QStringLiteral("stationId")).toDouble()) == stationId) {
                return station;
            }
        }
        return {};
    }

    QJsonArray stationListPayload() const
    {
        QJsonArray payload;
        for (const QJsonValue stationValue : stations_) {
            QJsonObject station = stationValue.toObject();
            const qint64 stationId = static_cast<qint64>(
                station.value(QStringLiteral("stationId")).toDouble());
            int totalCount = 0;
            int offlineCount = 0;
            for (const QJsonValue chargerValue : chargers_) {
                const QJsonObject charger = chargerValue.toObject();
                if (static_cast<qint64>(charger.value(
                        QStringLiteral("stationId")).toDouble()) != stationId) {
                    continue;
                }
                ++totalCount;
                if (charger.value(QStringLiteral("status")).toInt()
                    == protocol::ChargerStatusOffline) {
                    ++offlineCount;
                }
            }
            const double onlineRate = totalCount == 0
                ? 0.0
                : 100.0 * static_cast<double>(totalCount - offlineCount)
                    / static_cast<double>(totalCount);
            station.insert(QStringLiteral("totalCount"), totalCount);
            station.insert(QStringLiteral("onlineRate"), onlineRate);
            payload.append(station);
        }
        return payload;
    }

    QJsonObject stationDetailPayload(qint64 stationId) const
    {
        QJsonObject station = findStation(stationId);
        if (station.isEmpty()) {
            return {};
        }
        QJsonArray detailChargers;
        int availableCount = 0;
        for (const QJsonValue chargerValue : chargers_) {
            const QJsonObject charger = chargerValue.toObject();
            if (static_cast<qint64>(charger.value(
                    QStringLiteral("stationId")).toDouble()) != stationId) {
                continue;
            }
            detailChargers.append(QJsonObject {
                {QStringLiteral("chargerId"),
                 charger.value(QStringLiteral("chargerId"))},
                {QStringLiteral("code"),
                 charger.value(QStringLiteral("code"))},
                {QStringLiteral("type"), charger.value(QStringLiteral("type"))},
                {QStringLiteral("powerKw"),
                 charger.value(QStringLiteral("powerKw"))},
                {QStringLiteral("status"), charger.value(QStringLiteral("status"))},
            });
            if (charger.value(QStringLiteral("status")).toInt()
                == protocol::ChargerStatusIdle) {
                ++availableCount;
            }
        }
        // station.detail 严格模拟成员 A 已实现的公共接口：电价在站点级，
        // 单桩使用 code/powerKw，不携带管理员扩展字段 version。
        station.remove(QStringLiteral("version"));
        station.insert(QStringLiteral("availableCount"), availableCount);
        station.insert(QStringLiteral("totalCount"), detailChargers.size());
        station.insert(QStringLiteral("chargers"), detailChargers);
        return station;
    }

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
                       protocol::action::kAdminOrderList)) {
            if (adminIds_.value(socket) <= 0) {
                response.insert(QStringLiteral("code"), protocol::CodeNotLoggedIn);
                response.insert(QStringLiteral("message"), QStringLiteral("administrator login required"));
            } else {
                const QJsonValue pageValue = requestData.value(QStringLiteral("page"));
                const QJsonValue sizeValue = requestData.value(QStringLiteral("pageSize"));
                const QJsonValue keywordValue = requestData.value(QStringLiteral("keyword"));
                const QJsonValue statusValue = requestData.value(QStringLiteral("status"));
                const QJsonValue paymentValue = requestData.value(QStringLiteral("paymentStatus"));
                const double pageNumber = pageValue.toDouble(-1.0);
                const double sizeNumber = sizeValue.toDouble(-1.0);
                const QString keyword = keywordValue.toString().trimmed();
                const QString status = statusValue.toString();
                const QString payment = paymentValue.toString();
                const auto validStatus = [](const QString& value) {
                    return value == QStringLiteral("ALL") || value == QStringLiteral("RESERVED")
                        || value == QStringLiteral("CHARGING")
                        || value == QStringLiteral("WAIT_SETTLEMENT")
                        || value == QStringLiteral("FINISHED");
                };
                const auto validPayment = [](const QString& value) {
                    return value == QStringLiteral("ALL") || value == QStringLiteral("NOT_DUE")
                        || value == QStringLiteral("UNPAID") || value == QStringLiteral("PAID");
                };
                const bool valid = requestData.size() == 5 && pageValue.isDouble()
                    && std::floor(pageNumber) == pageNumber && pageNumber >= 1.0
                    && sizeValue.isDouble() && std::floor(sizeNumber) == sizeNumber
                    && sizeNumber >= 1.0 && sizeNumber <= 100.0
                    && keywordValue.isString() && keyword.size() <= 50
                    && statusValue.isString() && validStatus(status)
                    && paymentValue.isString() && validPayment(payment);
                if (!valid) {
                    response.insert(QStringLiteral("code"), protocol::CodeBadRequest);
                    response.insert(QStringLiteral("message"), QStringLiteral("订单查询参数不符合协议"));
                } else {
                    const qint64 generatedAt = QDateTime::currentMSecsSinceEpoch();
                    QJsonArray filtered;
                    qint64 reserved = 0, charging = 0, waiting = 0, finished = 0;
                    double paidAmount = 0.0;
                    for (const QJsonValue value : orders_) {
                        QJsonObject current = value.toObject();
                        const QString currentStatus = current.value(QStringLiteral("status")).toString();
                        if (currentStatus == QStringLiteral("RESERVED")) ++reserved;
                        else if (currentStatus == QStringLiteral("CHARGING")) {
                            ++charging;
                            const qint64 startTime = static_cast<qint64>(
                                current.value(QStringLiteral("startTime")).toDouble());
                            const qint64 seconds = qMax<qint64>(0, (generatedAt - startTime) / 1000);
                            const double energy = seconds / 3600.0
                                * current.value(QStringLiteral("powerKw")).toDouble() * 0.8;
                            current.insert(QStringLiteral("durationSeconds"), seconds);
                            current.insert(QStringLiteral("energyKwh"), energy);
                            current.insert(QStringLiteral("amount"), energy
                                * current.value(QStringLiteral("pricePerKwh")).toDouble());
                        } else if (currentStatus == QStringLiteral("WAIT_SETTLEMENT")) ++waiting;
                        else {
                            ++finished;
                            paidAmount += current.value(QStringLiteral("amount")).toDouble();
                        }
                        const QString searchable = QStringLiteral("%1 %2 %3 %4 %5")
                            .arg(current.value(QStringLiteral("orderId")).toVariant().toString(),
                                 current.value(QStringLiteral("phone")).toString(),
                                 current.value(QStringLiteral("nickname")).toString(),
                                 current.value(QStringLiteral("stationName")).toString(),
                                 current.value(QStringLiteral("chargerCode")).toString());
                        if ((!keyword.isEmpty() && !searchable.contains(keyword, Qt::CaseInsensitive))
                            || (status != QStringLiteral("ALL") && currentStatus != status)
                            || (payment != QStringLiteral("ALL")
                                && current.value(QStringLiteral("paymentStatus")).toString() != payment)) {
                            continue;
                        }
                        filtered.append(current);
                    }
                    const int page = static_cast<int>(pageNumber);
                    const int pageSize = static_cast<int>(sizeNumber);
                    const int start = (page - 1) * pageSize;
                    QJsonArray paged;
                    for (int index = start; index < filtered.size()
                         && index < start + pageSize; ++index) {
                        paged.append(filtered.at(index));
                    }
                    response.insert(QStringLiteral("data"), QJsonObject {
                        {QStringLiteral("orders"), paged},
                        {QStringLiteral("total"), filtered.size()},
                        {QStringLiteral("page"), page},
                        {QStringLiteral("pageSize"), pageSize},
                        {QStringLiteral("generatedAt"), static_cast<double>(generatedAt)},
                        {QStringLiteral("platformSummary"), QJsonObject {
                            {QStringLiteral("totalOrders"), orders_.size()},
                            {QStringLiteral("reservedCount"), reserved},
                            {QStringLiteral("chargingCount"), charging},
                            {QStringLiteral("waitSettlementCount"), waiting},
                            {QStringLiteral("finishedCount"), finished},
                            {QStringLiteral("paidAmount"), paidAmount},
                        }},
                    });
                }
            }
        } else if (action == QString::fromUtf8(
                       protocol::action::kAdminUserList)) {
            if (adminIds_.value(socket) <= 0) {
                response.insert(QStringLiteral("code"), protocol::CodeNotLoggedIn);
                response.insert(QStringLiteral("message"), QStringLiteral("administrator login required"));
            } else {
                const QJsonValue pageValue = requestData.value(QStringLiteral("page"));
                const QJsonValue sizeValue = requestData.value(QStringLiteral("pageSize"));
                const QJsonValue keywordValue = requestData.value(QStringLiteral("phoneKeyword"));
                const double pageNumber = pageValue.toDouble(-1.0);
                const double sizeNumber = sizeValue.toDouble(-1.0);
                const QString keyword = keywordValue.toString();
                const QString activityFilter = requestData.value(
                    QStringLiteral("activityFilter")).toString();
                const bool hasStatus = requestData.contains(QStringLiteral("status"));
                const QJsonValue statusValue = requestData.value(QStringLiteral("status"));
                const double statusNumber = statusValue.toDouble(-1.0);
                static const QRegularExpression digits(QStringLiteral("^\\d{0,11}$"));
                const bool valid = pageValue.isDouble() && std::floor(pageNumber) == pageNumber
                    && pageNumber >= 1.0 && sizeValue.isDouble()
                    && std::floor(sizeNumber) == sizeNumber && sizeNumber >= 1.0
                    && sizeNumber <= 100.0 && keywordValue.isString()
                    && digits.match(keyword).hasMatch()
                    && (activityFilter == QStringLiteral("ALL")
                        || activityFilter == QStringLiteral("ACTIVE")
                        || activityFilter == QStringLiteral("IDLE"))
                    && (!hasStatus || (statusValue.isDouble()
                        && (statusNumber == protocol::UserStatusNormal
                            || statusNumber == protocol::UserStatusFrozen)));
                if (!valid) {
                    response.insert(QStringLiteral("code"), protocol::CodeBadRequest);
                    response.insert(QStringLiteral("message"), QStringLiteral("用户查询参数不符合协议"));
                } else {
                    QJsonArray filtered;
                    for (const QJsonValue value : users_) {
                        const QJsonObject user = value.toObject();
                        if (!keyword.isEmpty()
                            && !user.value(QStringLiteral("phone")).toString().contains(keyword)) continue;
                        if (hasStatus
                            && user.value(QStringLiteral("status")).toInt(-1) != static_cast<int>(statusNumber)) continue;
                        const bool isActive = user.value(
                            QStringLiteral("activityStatus")).toString()
                            != QStringLiteral("IDLE");
                        if ((activityFilter == QStringLiteral("ACTIVE") && !isActive)
                            || (activityFilter == QStringLiteral("IDLE") && isActive)) continue;
                        filtered.append(user);
                    }
                    const int page = static_cast<int>(pageNumber);
                    const int pageSize = static_cast<int>(sizeNumber);
                    const int start = (page - 1) * pageSize;
                    QJsonArray paged;
                    for (int index = start; index < filtered.size() && index < start + pageSize; ++index) {
                        paged.append(filtered.at(index));
                    }
                    response.insert(QStringLiteral("data"), QJsonObject {
                        {QStringLiteral("users"), paged},
                        {QStringLiteral("total"), filtered.size()},
                        {QStringLiteral("page"), page},
                        {QStringLiteral("pageSize"), pageSize},
                    });
                }
            }
        } else if (action == QString::fromUtf8(
                       protocol::action::kAdminUserFreeze)) {
            if (adminIds_.value(socket) <= 0) {
                response.insert(QStringLiteral("code"), protocol::CodeNotLoggedIn);
                response.insert(QStringLiteral("message"), QStringLiteral("administrator login required"));
            } else {
                const QJsonValue idValue = requestData.value(QStringLiteral("userId"));
                const QJsonValue expectedValue = requestData.value(QStringLiteral("expectedStatus"));
                const QJsonValue targetValue = requestData.value(QStringLiteral("targetStatus"));
                const double idNumber = idValue.toDouble(-1.0);
                const double expectedNumber = expectedValue.toDouble(-1.0);
                const double targetNumber = targetValue.toDouble(-1.0);
                const QString reason = requestData.value(QStringLiteral("reason")).toString().trimmed();
                const auto statusValid = [](double status) {
                    return status == protocol::UserStatusNormal
                        || status == protocol::UserStatusFrozen;
                };
                const bool valid = idValue.isDouble() && std::floor(idNumber) == idNumber
                    && idNumber > 0.0 && expectedValue.isDouble() && statusValid(expectedNumber)
                    && targetValue.isDouble() && statusValid(targetNumber)
                    && expectedNumber != targetNumber && reason.size() >= 2 && reason.size() <= 200;
                int userIndex = -1;
                if (valid) {
                    for (int index = 0; index < users_.size(); ++index) {
                        if (users_.at(index).toObject().value(QStringLiteral("userId")).toDouble() == idNumber) {
                            userIndex = index;
                            break;
                        }
                    }
                }
                if (!valid) {
                    response.insert(QStringLiteral("code"), protocol::CodeBadRequest);
                    response.insert(QStringLiteral("message"), QStringLiteral("用户状态参数不符合协议"));
                } else if (userIndex < 0) {
                    response.insert(QStringLiteral("code"), protocol::CodeBadRequest);
                    response.insert(QStringLiteral("message"), QStringLiteral("用户不存在"));
                } else {
                    QJsonObject user = users_.at(userIndex).toObject();
                    const int current = user.value(QStringLiteral("status")).toInt(-1);
                    if (current != static_cast<int>(expectedNumber)) {
                        response.insert(QStringLiteral("code"), protocol::CodeUserStateConflict);
                        response.insert(QStringLiteral("message"), QStringLiteral("用户状态已变化，请刷新后重试"));
                    } else if (targetNumber == protocol::UserStatusFrozen
                               && user.value(QStringLiteral("activeOrder")).isObject()) {
                        // 覆盖 RESERVED / CHARGING / WAIT_SETTLEMENT；生产规则仍由服务端决定。
                        response.insert(QStringLiteral("code"), protocol::CodeOrderConflict);
                        response.insert(QStringLiteral("message"), QStringLiteral("用户存在未完成订单，服务端拒绝冻结"));
                    } else {
                        const int target = static_cast<int>(targetNumber);
                        user.insert(QStringLiteral("status"), target);
                        users_.replace(userIndex, user);
                        response.insert(QStringLiteral("message"), target == protocol::UserStatusFrozen
                            ? QStringLiteral("用户已冻结") : QStringLiteral("用户已解冻"));
                        response.insert(QStringLiteral("data"), QJsonObject {
                            {QStringLiteral("userId"), idNumber},
                            {QStringLiteral("previousStatus"), current},
                            {QStringLiteral("status"), target},
                            {QStringLiteral("changedAt"), static_cast<double>(QDateTime::currentMSecsSinceEpoch())},
                        });
                    }
                }
            }
        } else if (action == QString::fromUtf8(
                       protocol::action::kAdminRevenueSummary)) {
            if (adminIds_.value(socket) <= 0) {
                response.insert(QStringLiteral("code"), protocol::CodeNotLoggedIn);
                response.insert(QStringLiteral("message"),
                                QStringLiteral("administrator login required"));
            } else if (!requestData.isEmpty()) {
                response.insert(QStringLiteral("code"), protocol::CodeBadRequest);
                response.insert(QStringLiteral("message"),
                                QStringLiteral("revenue summary data must be empty"));
            } else {
                const QDate today = QDateTime::currentDateTimeUtc().toTimeZone(
                    QTimeZone(QByteArrayLiteral("Asia/Shanghai"))).date();
                response.insert(QStringLiteral("data"), QJsonObject {
                    {QStringLiteral("todayRevenue"), mockRevenueForDate(today)},
                    {QStringLiteral("monthRevenue"), 3680.75},
                    {QStringLiteral("totalRevenue"), 15280.25},
                    {QStringLiteral("currency"), QStringLiteral("CNY")},
                    {QStringLiteral("timezone"), QStringLiteral("Asia/Shanghai")},
                    {QStringLiteral("generatedAt"),
                     QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
                });
            }
        } else if (action == QString::fromUtf8(
                       protocol::action::kAdminRevenueTrend)) {
            if (adminIds_.value(socket) <= 0) {
                response.insert(QStringLiteral("code"), protocol::CodeNotLoggedIn);
                response.insert(QStringLiteral("message"),
                                QStringLiteral("administrator login required"));
            } else {
                const QJsonValue daysValue = requestData.value(QStringLiteral("days"));
                const double daysNumber = daysValue.toDouble(-1.0);
                const bool valid = requestData.size() == 1 && daysValue.isDouble()
                    && std::floor(daysNumber) == daysNumber
                    && (daysNumber == 7.0 || daysNumber == 30.0);
                if (!valid) {
                    response.insert(QStringLiteral("code"), protocol::CodeBadRequest);
                    response.insert(QStringLiteral("message"),
                                    QStringLiteral("days must be 7 or 30"));
                } else {
                    const int days = static_cast<int>(daysNumber);
                    response.insert(QStringLiteral("data"), QJsonObject {
                        {QStringLiteral("days"), days},
                        {QStringLiteral("timezone"), QStringLiteral("Asia/Shanghai")},
                        {QStringLiteral("generatedAt"),
                         QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
                        {QStringLiteral("points"), mockRevenuePoints(days)},
                    });
                }
            }
        } else if (action == QString::fromUtf8(
                       protocol::action::kAdminChargerOverview)) {
            if (adminIds_.value(socket) <= 0) {
                response.insert(QStringLiteral("code"), protocol::CodeNotLoggedIn);
                response.insert(QStringLiteral("message"),
                                QStringLiteral("administrator login required"));
            } else {
                // 仅用于客户端联调；真实数量和占比必须由成员 1 的服务端聚合。
                int counts[4] = {0, 0, 0, 0};
                for (const QJsonValue value : chargers_) {
                    const int status = value.toObject().value(
                        QStringLiteral("status")).toInt(-1);
                    if (status >= protocol::ChargerStatusIdle
                        && status <= protocol::ChargerStatusOffline) {
                        ++counts[status];
                    }
                }
                const int total = chargers_.size();
                const auto percent = [total](int count) {
                    return total == 0 ? 0.0
                                      : 100.0 * static_cast<double>(count)
                                          / static_cast<double>(total);
                };
                response.insert(QStringLiteral("data"), QJsonObject {
                    {QStringLiteral("total"), total},
                    {QStringLiteral("idle"), counts[protocol::ChargerStatusIdle]},
                    {QStringLiteral("charging"), counts[protocol::ChargerStatusCharging]},
                    {QStringLiteral("fault"), counts[protocol::ChargerStatusFault]},
                    {QStringLiteral("offline"), counts[protocol::ChargerStatusOffline]},
                    {QStringLiteral("idlePercent"),
                     percent(counts[protocol::ChargerStatusIdle])},
                    {QStringLiteral("chargingPercent"),
                     percent(counts[protocol::ChargerStatusCharging])},
                    {QStringLiteral("faultPercent"),
                     percent(counts[protocol::ChargerStatusFault])},
                    {QStringLiteral("offlinePercent"),
                     percent(counts[protocol::ChargerStatusOffline])},
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
                    {QStringLiteral("chargers"), chargers_},
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
                    for (const QJsonValue value : chargers_) {
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
        } else if (action == QString::fromUtf8(
                       protocol::action::kAdminChargerStatusUpdate)) {
            if (adminIds_.value(socket) <= 0) {
                response.insert(QStringLiteral("code"), protocol::CodeNotLoggedIn);
                response.insert(QStringLiteral("message"),
                                QStringLiteral("administrator login required"));
            } else {
                const QJsonValue chargerIdValue = requestData.value(
                    QStringLiteral("chargerId"));
                const QJsonValue expectedValue = requestData.value(
                    QStringLiteral("expectedStatus"));
                const QJsonValue targetValue = requestData.value(
                    QStringLiteral("targetStatus"));
                const QString reason = requestData.value(
                    QStringLiteral("reason")).toString().trimmed();
                const double chargerIdNumber = chargerIdValue.toDouble(-1.0);
                const double expectedNumber = expectedValue.toDouble(-1.0);
                const double targetNumber = targetValue.toDouble(-1.0);
                const auto validNumber = [](const QJsonValue& value,
                                            double number) {
                    return value.isDouble() && std::isfinite(number)
                        && std::floor(number) == number;
                };
                const bool validTarget = targetNumber == protocol::ChargerStatusIdle
                    || targetNumber == protocol::ChargerStatusFault
                    || targetNumber == protocol::ChargerStatusOffline;
                const bool valid = validNumber(chargerIdValue, chargerIdNumber)
                    && chargerIdNumber > 0.0
                    && chargerIdNumber <= 9007199254740991.0
                    && validNumber(expectedValue, expectedNumber)
                    && expectedNumber >= protocol::ChargerStatusIdle
                    && expectedNumber <= protocol::ChargerStatusOffline
                    && validNumber(targetValue, targetNumber) && validTarget
                    && expectedNumber != targetNumber
                    && reason.size() >= 2 && reason.size() <= 200;
                int selectedIndex = -1;
                if (valid) {
                    for (int index = 0; index < chargers_.size(); ++index) {
                        const QJsonObject charger = chargers_.at(index).toObject();
                        if (charger.value(QStringLiteral("chargerId")).toDouble()
                            == chargerIdNumber) {
                            selectedIndex = index;
                            break;
                        }
                    }
                }
                if (!valid) {
                    response.insert(QStringLiteral("code"), protocol::CodeBadRequest);
                    response.insert(QStringLiteral("message"),
                                    QStringLiteral("设备状态变更参数不符合协议"));
                } else if (selectedIndex < 0) {
                    response.insert(QStringLiteral("code"), protocol::CodeBadRequest);
                    response.insert(QStringLiteral("message"),
                                    QStringLiteral("充电桩不存在"));
                } else {
                    QJsonObject charger = chargers_.at(selectedIndex).toObject();
                    const int currentStatus = charger.value(
                        QStringLiteral("status")).toInt(-1);
                    if (currentStatus == protocol::ChargerStatusCharging) {
                        response.insert(QStringLiteral("code"),
                                        protocol::CodeChargerOperationRejected);
                        response.insert(QStringLiteral("message"),
                                        QStringLiteral("充电桩正在服务订单，禁止强制修改状态"));
                    } else if (currentStatus != static_cast<int>(expectedNumber)) {
                        response.insert(QStringLiteral("code"),
                                        protocol::CodeChargerStateConflict);
                        response.insert(QStringLiteral("message"),
                                        QStringLiteral("充电桩状态已变化，请刷新后重试"));
                    } else {
                        const int targetStatus = static_cast<int>(targetNumber);
                        charger.insert(QStringLiteral("status"), targetStatus);
                        chargers_.replace(selectedIndex, charger);
                        response.insert(QStringLiteral("message"),
                                        QStringLiteral("设备状态已更新"));
                        response.insert(QStringLiteral("data"), QJsonObject {
                            {QStringLiteral("chargerId"), chargerIdNumber},
                            {QStringLiteral("previousStatus"), currentStatus},
                            {QStringLiteral("status"), targetStatus},
                            {QStringLiteral("changedAt"),
                             QDateTime::currentMSecsSinceEpoch()},
                        });
                    }
                }
            }
        } else if (action == QString::fromUtf8(
                       protocol::action::kAdminStationList)) {
            if (adminIds_.value(socket) <= 0) {
                response.insert(QStringLiteral("code"), protocol::CodeNotLoggedIn);
                response.insert(QStringLiteral("message"),
                                QStringLiteral("administrator login required"));
            } else {
                response.insert(QStringLiteral("data"), QJsonObject {
                    {QStringLiteral("stations"), stationListPayload()},
                });
            }
        } else if (action == QString::fromUtf8(
                       protocol::action::kStationDetail)) {
            const QJsonValue stationIdValue =
                requestData.value(QStringLiteral("stationId"));
            const double stationIdNumber = stationIdValue.toDouble();
            const bool validStationId = stationIdValue.isDouble()
                && std::isfinite(stationIdNumber) && stationIdNumber >= 1.0
                && stationIdNumber <= 9007199254740991.0
                && std::floor(stationIdNumber) == stationIdNumber;
            const qint64 stationId = validStationId
                ? static_cast<qint64>(stationIdNumber) : 0;
            const QJsonObject detail = validStationId
                ? stationDetailPayload(stationId) : QJsonObject{};
            if (!validStationId) {
                response.insert(QStringLiteral("code"), protocol::CodeBadRequest);
                response.insert(QStringLiteral("message"),
                                QStringLiteral("stationId must be a positive integer"));
            } else if (detail.isEmpty()) {
                response.insert(QStringLiteral("code"), protocol::CodeBadRequest);
                response.insert(QStringLiteral("message"),
                                QStringLiteral("充电站不存在"));
            } else {
                response.insert(QStringLiteral("data"), detail);
            }
        } else if (action == QString::fromUtf8(
                       protocol::action::kAdminStationCreate)) {
            if (adminIds_.value(socket) <= 0) {
                response.insert(QStringLiteral("code"), protocol::CodeNotLoggedIn);
                response.insert(QStringLiteral("message"),
                                QStringLiteral("administrator login required"));
            } else {
                const QString name = requestData.value(
                    QStringLiteral("name")).toString().trimmed();
                const QString address = requestData.value(
                    QStringLiteral("address")).toString().trimmed();
                const QJsonValue latitudeValue = requestData.value(
                    QStringLiteral("latitude"));
                const QJsonValue longitudeValue = requestData.value(
                    QStringLiteral("longitude"));
                const QJsonValue countValue = requestData.value(
                    QStringLiteral("chargerCount"));
                const QJsonValue priceValue = requestData.value(
                    QStringLiteral("pricePerKwh"));
                const double latitude = latitudeValue.toDouble(
                    std::numeric_limits<double>::quiet_NaN());
                const double longitude = longitudeValue.toDouble(
                    std::numeric_limits<double>::quiet_NaN());
                const double countNumber = countValue.toDouble(-1.0);
                const double pricePerKwh = priceValue.toDouble(-1.0);
                const bool valid = !name.isEmpty() && name.size() <= 60
                    && !address.isEmpty() && address.size() <= 200
                    && latitudeValue.isDouble() && std::isfinite(latitude)
                    && latitude >= -90.0 && latitude <= 90.0
                    && longitudeValue.isDouble() && std::isfinite(longitude)
                    && longitude >= -180.0 && longitude <= 180.0
                    && countValue.isDouble() && std::isfinite(countNumber)
                    && countNumber >= 0.0 && countNumber <= 100.0
                    && std::floor(countNumber) == countNumber
                    && priceValue.isDouble() && std::isfinite(pricePerKwh)
                    && pricePerKwh > 0.0;
                bool duplicate = false;
                if (valid) {
                    for (const QJsonValue value : stations_) {
                        const QJsonObject station = value.toObject();
                        if (station.value(QStringLiteral("name")).toString()
                                    .compare(name, Qt::CaseInsensitive) == 0
                            && station.value(QStringLiteral("address")).toString()
                                   .compare(address, Qt::CaseInsensitive) == 0) {
                            duplicate = true;
                            break;
                        }
                    }
                }
                if (!valid) {
                    response.insert(QStringLiteral("code"), protocol::CodeBadRequest);
                    response.insert(QStringLiteral("message"),
                                    QStringLiteral("站点新增参数不符合协议"));
                } else if (duplicate) {
                    response.insert(QStringLiteral("code"), protocol::CodeBadRequest);
                    response.insert(QStringLiteral("message"),
                                    QStringLiteral("已存在同名同址站点"));
                } else {
                    qint64 stationId = 1;
                    qint64 chargerId = 1001;
                    for (const QJsonValue value : stations_) {
                        stationId = qMax(stationId,
                            static_cast<qint64>(value.toObject().value(
                                QStringLiteral("stationId")).toDouble()) + 1);
                    }
                    for (const QJsonValue value : chargers_) {
                        chargerId = qMax(chargerId,
                            static_cast<qint64>(value.toObject().value(
                                QStringLiteral("chargerId")).toDouble()) + 1);
                    }
                    stations_.append(QJsonObject {
                        {QStringLiteral("stationId"), stationId},
                        {QStringLiteral("name"), name},
                        {QStringLiteral("address"), address},
                        {QStringLiteral("latitude"), latitude},
                        {QStringLiteral("longitude"), longitude},
                        {QStringLiteral("pricePerKwh"), pricePerKwh},
                        {QStringLiteral("version"), 1},
                    });
                    const int chargerCount = static_cast<int>(countNumber);
                    for (int index = 0; index < chargerCount; ++index) {
                        const bool fast = index % 2 == 0;
                        chargers_.append(QJsonObject {
                            {QStringLiteral("chargerId"), chargerId + index},
                            {QStringLiteral("code"),
                             QStringLiteral("ST-%1-CP-%2")
                                 .arg(stationId)
                                 .arg(index + 1, 3, 10, QChar(u'0'))},
                            {QStringLiteral("stationId"), stationId},
                            {QStringLiteral("stationName"), name},
                            {QStringLiteral("type"), fast
                                 ? protocol::ChargerTypeFast
                                 : protocol::ChargerTypeSlow},
                            {QStringLiteral("powerKw"), fast ? 120.0 : 7.0},
                            {QStringLiteral("status"), protocol::ChargerStatusIdle},
                            {QStringLiteral("totalChargeCount"), 0},
                            {QStringLiteral("totalChargeDurationSeconds"), 0},
                        });
                    }
                    response.insert(QStringLiteral("message"),
                                    QStringLiteral("充电站创建成功"));
                    response.insert(QStringLiteral("data"), QJsonObject {
                        {QStringLiteral("stationId"), stationId},
                        {QStringLiteral("createdChargerCount"), chargerCount},
                    });
                }
            }
        } else if (action == QString::fromUtf8(
                       protocol::action::kAdminStationUpdate)) {
            if (adminIds_.value(socket) <= 0) {
                response.insert(QStringLiteral("code"), protocol::CodeNotLoggedIn);
                response.insert(QStringLiteral("message"),
                                QStringLiteral("administrator login required"));
            } else {
                const QJsonValue stationIdValue = requestData.value(
                    QStringLiteral("stationId"));
                const QJsonValue versionValue = requestData.value(
                    QStringLiteral("expectedVersion"));
                const QJsonValue latitudeValue = requestData.value(
                    QStringLiteral("latitude"));
                const QJsonValue longitudeValue = requestData.value(
                    QStringLiteral("longitude"));
                const QJsonValue priceValue = requestData.value(
                    QStringLiteral("pricePerKwh"));
                const QString name = requestData.value(
                    QStringLiteral("name")).toString().trimmed();
                const QString address = requestData.value(
                    QStringLiteral("address")).toString().trimmed();
                const double stationIdNumber = stationIdValue.toDouble(-1.0);
                const double versionNumber = versionValue.toDouble(-1.0);
                const double latitude = latitudeValue.toDouble(
                    std::numeric_limits<double>::quiet_NaN());
                const double longitude = longitudeValue.toDouble(
                    std::numeric_limits<double>::quiet_NaN());
                const double pricePerKwh = priceValue.toDouble(-1.0);
                const bool valid = stationIdValue.isDouble()
                    && std::isfinite(stationIdNumber) && stationIdNumber > 0.0
                    && stationIdNumber <= 9007199254740991.0
                    && std::floor(stationIdNumber) == stationIdNumber
                    && versionValue.isDouble() && std::isfinite(versionNumber)
                    && versionNumber > 0.0 && std::floor(versionNumber) == versionNumber
                    && !name.isEmpty() && name.size() <= 60
                    && !address.isEmpty() && address.size() <= 200
                    && latitudeValue.isDouble() && std::isfinite(latitude)
                    && latitude >= -90.0 && latitude <= 90.0
                    && longitudeValue.isDouble() && std::isfinite(longitude)
                    && longitude >= -180.0 && longitude <= 180.0
                    && priceValue.isDouble() && std::isfinite(pricePerKwh)
                    && pricePerKwh > 0.0;
                int stationIndex = -1;
                bool duplicate = false;
                if (valid) {
                    for (int index = 0; index < stations_.size(); ++index) {
                        const QJsonObject station = stations_.at(index).toObject();
                        const double existingId = station.value(
                            QStringLiteral("stationId")).toDouble();
                        if (existingId == stationIdNumber) {
                            stationIndex = index;
                        } else if (station.value(QStringLiteral("name")).toString()
                                       .compare(name, Qt::CaseInsensitive) == 0
                                   && station.value(QStringLiteral("address")).toString()
                                          .compare(address, Qt::CaseInsensitive) == 0) {
                            duplicate = true;
                        }
                    }
                }
                if (!valid) {
                    response.insert(QStringLiteral("code"), protocol::CodeBadRequest);
                    response.insert(QStringLiteral("message"),
                                    QStringLiteral("站点编辑参数不符合协议"));
                } else if (stationIndex < 0) {
                    response.insert(QStringLiteral("code"), protocol::CodeBadRequest);
                    response.insert(QStringLiteral("message"),
                                    QStringLiteral("充电站不存在"));
                } else if (duplicate) {
                    response.insert(QStringLiteral("code"), protocol::CodeBadRequest);
                    response.insert(QStringLiteral("message"),
                                    QStringLiteral("已存在同名同址站点"));
                } else {
                    QJsonObject station = stations_.at(stationIndex).toObject();
                    const qint64 currentVersion = static_cast<qint64>(
                        station.value(QStringLiteral("version")).toDouble());
                    if (currentVersion != static_cast<qint64>(versionNumber)) {
                        response.insert(QStringLiteral("code"),
                                        protocol::CodeStationVersionConflict);
                        response.insert(QStringLiteral("message"),
                                        QStringLiteral("站点资料已被其他管理员修改，请刷新后重试"));
                    } else {
                        const qint64 nextVersion = currentVersion + 1;
                        station.insert(QStringLiteral("name"), name);
                        station.insert(QStringLiteral("address"), address);
                        station.insert(QStringLiteral("latitude"), latitude);
                        station.insert(QStringLiteral("longitude"), longitude);
                        station.insert(QStringLiteral("pricePerKwh"), pricePerKwh);
                        station.insert(QStringLiteral("version"), nextVersion);
                        stations_.replace(stationIndex, station);
                        for (int index = 0; index < chargers_.size(); ++index) {
                            QJsonObject charger = chargers_.at(index).toObject();
                            if (charger.value(QStringLiteral("stationId")).toDouble()
                                == stationIdNumber) {
                                charger.insert(QStringLiteral("stationName"), name);
                                chargers_.replace(index, charger);
                            }
                        }
                        response.insert(QStringLiteral("message"),
                                        QStringLiteral("充电站资料已更新"));
                        response.insert(QStringLiteral("data"), QJsonObject {
                            {QStringLiteral("stationId"), stationIdNumber},
                            {QStringLiteral("version"), nextVersion},
                            {QStringLiteral("updatedAt"),
                             QDateTime::currentMSecsSinceEpoch()},
                        });
                    }
                }
            }
        } else {
            response.insert(QStringLiteral("code"), 1001);
            response.insert(QStringLiteral("message"), QStringLiteral("unsupported action"));
        }

        // 反向延迟 PING，强制客户端在乱序响应下按 requestId 匹配。
        const QString echo = requestData.value(QStringLiteral("echo")).toString();
        const int suffix = echo.right(1).toInt();
        int delayMs = echo.isEmpty() ? 0 : (5 - suffix) * 25;
        // 7 日请求故意慢于 30 日请求，用于验证页面 generation 防止迟到覆盖。
        if (action == QString::fromUtf8(protocol::action::kAdminRevenueTrend)) {
            delayMs = requestData.value(QStringLiteral("days")).toInt() == 7 ? 180 : 20;
        }
        const QPointer<QTcpSocket> guardedSocket(socket);
        QTimer::singleShot(delayMs, this, [guardedSocket, response] {
            if (guardedSocket) {
                guardedSocket->write(makeFrame(response));
            }
        });
    }

    QTcpServer server_;
    QJsonArray stations_;
    QJsonArray chargers_;
    QJsonArray users_;
    QJsonArray orders_;
    QHash<QTcpSocket*, QByteArray> buffers_;
    QHash<QTcpSocket*, qint64> adminIds_;
};

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    MockServer server;
    if (!server.listen()) {
        std::fprintf(stderr, "failed to listen on 127.0.0.1:8888\n");
        return 1;
    }
    std::printf("admin mock server listening on 127.0.0.1:8888\n");
    std::fflush(stdout);
    return application.exec();
}
