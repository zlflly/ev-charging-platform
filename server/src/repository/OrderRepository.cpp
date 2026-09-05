#include "OrderRepository.h"
#include "Database.h"
#include <QDateTime>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

namespace repository {

std::optional<Order> OrderRepository::findById(int orderId) {
    QSqlQuery query(Database::instance().db());
    query.prepare("SELECT orderId, userId, chargerId, stationId, status, startTime, stopTime, "
                  "settleTime, duration, energyKwh, amount, createdAt "
                  "FROM orders WHERE orderId = ?");
    query.addBindValue(orderId);

    if (!query.exec()) {
        qWarning() << "Failed to query order by id:" << query.lastError().text();
        return std::nullopt;
    }

    if (!query.next()) {
        return std::nullopt;
    }

    Order order;
    order.orderId = query.value(0).toInt();
    order.userId = query.value(1).toInt();
    order.chargerId = query.value(2).toInt();
    order.stationId = query.value(3).toInt();
    order.status = query.value(4).toString();
    order.startTime = query.value(5).toLongLong();
    order.stopTime = query.value(6).toLongLong();
    order.settleTime = query.value(7).toLongLong();
    order.duration = query.value(8).toInt();
    order.energyKwh = query.value(9).toDouble();
    order.amount = query.value(10).toDouble();
    order.createdAt = query.value(11).toLongLong();

    return order;
}

std::optional<Order> OrderRepository::findActiveByUser(int userId) {
    QSqlQuery query(Database::instance().db());
    query.prepare("SELECT orderId, userId, chargerId, stationId, status, startTime, stopTime, "
                  "settleTime, duration, energyKwh, amount, createdAt "
                  "FROM orders WHERE userId = ? AND status IN ('RESERVED', 'CHARGING', 'WAIT_SETTLEMENT') "
                  "ORDER BY createdAt DESC LIMIT 1");
    query.addBindValue(userId);

    if (!query.exec()) {
        qWarning() << "Failed to query active order:" << query.lastError().text();
        return std::nullopt;
    }

    if (!query.next()) {
        return std::nullopt;
    }

    Order order;
    order.orderId = query.value(0).toInt();
    order.userId = query.value(1).toInt();
    order.chargerId = query.value(2).toInt();
    order.stationId = query.value(3).toInt();
    order.status = query.value(4).toString();
    order.startTime = query.value(5).toLongLong();
    order.stopTime = query.value(6).toLongLong();
    order.settleTime = query.value(7).toLongLong();
    order.duration = query.value(8).toInt();
    order.energyKwh = query.value(9).toDouble();
    order.amount = query.value(10).toDouble();
    order.createdAt = query.value(11).toLongLong();

    return order;
}

QVector<Order> OrderRepository::findByUser(int userId) {
    QVector<Order> orders;
    QSqlQuery query(Database::instance().db());
    query.prepare("SELECT orderId, userId, chargerId, stationId, status, startTime, stopTime, "
                  "settleTime, duration, energyKwh, amount, createdAt "
                  "FROM orders WHERE userId = ? ORDER BY createdAt DESC");
    query.addBindValue(userId);

    if (!query.exec()) {
        qWarning() << "Failed to query orders by user:" << query.lastError().text();
        return orders;
    }

    while (query.next()) {
        Order order;
        order.orderId = query.value(0).toInt();
        order.userId = query.value(1).toInt();
        order.chargerId = query.value(2).toInt();
        order.stationId = query.value(3).toInt();
        order.status = query.value(4).toString();
        order.startTime = query.value(5).toLongLong();
        order.stopTime = query.value(6).toLongLong();
        order.settleTime = query.value(7).toLongLong();
        order.duration = query.value(8).toInt();
        order.energyKwh = query.value(9).toDouble();
        order.amount = query.value(10).toDouble();
        order.createdAt = query.value(11).toLongLong();
        orders.append(order);
    }

    return orders;
}

QVector<Order> OrderRepository::findFinished() {
    QVector<Order> orders;
    QSqlQuery query(Database::instance().db());

    if (!query.exec("SELECT orderId, userId, chargerId, stationId, status, startTime, stopTime, "
                    "settleTime, duration, energyKwh, amount, createdAt "
                    "FROM orders WHERE status = 'FINISHED' ORDER BY settleTime")) {
        qWarning() << "Failed to query finished orders:" << query.lastError().text();
        return orders;
    }

    while (query.next()) {
        Order order;
        order.orderId = query.value(0).toInt();
        order.userId = query.value(1).toInt();
        order.chargerId = query.value(2).toInt();
        order.stationId = query.value(3).toInt();
        order.status = query.value(4).toString();
        order.startTime = query.value(5).toLongLong();
        order.stopTime = query.value(6).toLongLong();
        order.settleTime = query.value(7).toLongLong();
        order.duration = query.value(8).toInt();
        order.energyKwh = query.value(9).toDouble();
        order.amount = query.value(10).toDouble();
        order.createdAt = query.value(11).toLongLong();
        orders.append(order);
    }

    return orders;
}

QVector<Order> OrderRepository::findFinishedInRange(qint64 startTime, qint64 endTime) {
    QVector<Order> orders;
    QSqlQuery query(Database::instance().db());
    query.prepare("SELECT orderId, userId, chargerId, stationId, status, startTime, stopTime, "
                  "settleTime, duration, energyKwh, amount, createdAt "
                  "FROM orders WHERE status = 'FINISHED' AND settleTime >= ? AND settleTime < ? "
                  "ORDER BY settleTime");
    query.addBindValue(startTime);
    query.addBindValue(endTime);

    if (!query.exec()) {
        qWarning() << "Failed to query finished orders in range:" << query.lastError().text();
        return orders;
    }

    while (query.next()) {
        Order order;
        order.orderId = query.value(0).toInt();
        order.userId = query.value(1).toInt();
        order.chargerId = query.value(2).toInt();
        order.stationId = query.value(3).toInt();
        order.status = query.value(4).toString();
        order.startTime = query.value(5).toLongLong();
        order.stopTime = query.value(6).toLongLong();
        order.settleTime = query.value(7).toLongLong();
        order.duration = query.value(8).toInt();
        order.energyKwh = query.value(9).toDouble();
        order.amount = query.value(10).toDouble();
        order.createdAt = query.value(11).toLongLong();
        orders.append(order);
    }

    return orders;
}

std::optional<Order> OrderRepository::createReservation(int userId, int chargerId, int stationId) {
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    QSqlQuery query(Database::instance().db());
    query.prepare("INSERT INTO orders (userId, chargerId, stationId, status, createdAt) "
                  "VALUES (?, ?, ?, 'RESERVED', ?)");
    query.addBindValue(userId);
    query.addBindValue(chargerId);
    query.addBindValue(stationId);
    query.addBindValue(now);

    if (!query.exec()) {
        qWarning() << "Failed to create reservation:" << query.lastError().text();
        return std::nullopt;
    }

    int orderId = query.lastInsertId().toInt();
    return findById(orderId);
}

bool OrderRepository::setStatus(int orderId, const QString& status) {
    QSqlQuery query(Database::instance().db());
    query.prepare("UPDATE orders SET status = ? WHERE orderId = ?");
    query.addBindValue(status);
    query.addBindValue(orderId);

    if (!query.exec()) {
        qWarning() << "Failed to set order status:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool OrderRepository::startCharging(int orderId, qint64 startTime) {
    QSqlQuery query(Database::instance().db());
    query.prepare("UPDATE orders SET status = 'CHARGING', startTime = ? WHERE orderId = ?");
    query.addBindValue(startTime);
    query.addBindValue(orderId);

    if (!query.exec()) {
        qWarning() << "Failed to start charging:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool OrderRepository::stopCharging(int orderId, qint64 stopTime, int duration, double energyKwh, double amount) {
    QSqlQuery query(Database::instance().db());
    query.prepare("UPDATE orders SET status = 'WAIT_SETTLEMENT', stopTime = ?, duration = ?, "
                  "energyKwh = ?, amount = ? WHERE orderId = ?");
    query.addBindValue(stopTime);
    query.addBindValue(duration);
    query.addBindValue(energyKwh);
    query.addBindValue(amount);
    query.addBindValue(orderId);

    if (!query.exec()) {
        qWarning() << "Failed to stop charging:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool OrderRepository::settle(int orderId, qint64 settleTime) {
    QSqlQuery query(Database::instance().db());
    query.prepare("UPDATE orders SET status = 'FINISHED', settleTime = ? WHERE orderId = ?");
    query.addBindValue(settleTime);
    query.addBindValue(orderId);

    if (!query.exec()) {
        qWarning() << "Failed to settle order:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

std::optional<Order> OrderRepository::findActiveOrderByCharger(int chargerId) {
    QSqlQuery query(Database::instance().db());
    query.prepare("SELECT orderId, userId, chargerId, stationId, status, startTime, stopTime, "
                  "settleTime, duration, energyKwh, amount, createdAt "
                  "FROM orders WHERE chargerId = ? AND status IN ('RESERVED', 'CHARGING', 'WAIT_SETTLEMENT') "
                  "ORDER BY createdAt DESC LIMIT 1");
    query.addBindValue(chargerId);

    if (!query.exec()) {
        qWarning() << "Failed to query active order by charger:" << query.lastError().text();
        return std::nullopt;
    }

    if (!query.next()) {
        return std::nullopt;
    }

    Order order;
    order.orderId = query.value(0).toInt();
    order.userId = query.value(1).toInt();
    order.chargerId = query.value(2).toInt();
    order.stationId = query.value(3).toInt();
    order.status = query.value(4).toString();
    order.startTime = query.value(5).toLongLong();
    order.stopTime = query.value(6).toLongLong();
    order.settleTime = query.value(7).toLongLong();
    order.duration = query.value(8).toInt();
    order.energyKwh = query.value(9).toDouble();
    order.amount = query.value(10).toDouble();
    order.createdAt = query.value(11).toLongLong();

    return order;
}

OrderRepository::ChargerStats OrderRepository::getChargerStats(int chargerId) {
    ChargerStats stats = {0, 0};
    QSqlQuery query(Database::instance().db());
    query.prepare("SELECT COUNT(*), COALESCE(SUM(duration), 0) "
                  "FROM orders WHERE chargerId = ? AND status = 'FINISHED'");
    query.addBindValue(chargerId);

    if (!query.exec()) {
        qWarning() << "Failed to get charger stats:" << query.lastError().text();
        return stats;
    }

    if (query.next()) {
        stats.totalCount = query.value(0).toInt();
        stats.totalDurationSeconds = query.value(1).toInt();
    }

    return stats;
}

} // namespace repository
