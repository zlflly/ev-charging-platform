#include "ChargerRepository.h"
#include "Database.h"
#include <QDateTime>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

namespace repository {

std::optional<Charger> ChargerRepository::findById(int chargerId) {
    QSqlQuery query(Database::instance().db());
    query.prepare("SELECT chargerId, stationId, code, type, status, powerKw, "
                  "totalChargeCount, totalChargeDurationSeconds, createdAt "
                  "FROM chargers WHERE chargerId = ?");
    query.addBindValue(chargerId);

    if (!query.exec()) {
        qWarning() << "Failed to query charger by id:" << query.lastError().text();
        return std::nullopt;
    }

    if (!query.next()) {
        return std::nullopt;
    }

    Charger charger;
    charger.chargerId = query.value(0).toInt();
    charger.stationId = query.value(1).toInt();
    charger.code = query.value(2).toString();
    charger.type = query.value(3).toInt();
    charger.status = query.value(4).toInt();
    charger.powerKw = query.value(5).toDouble();
    charger.totalChargeCount = query.value(6).toInt();
    charger.totalChargeDurationSeconds = query.value(7).toInt();
    charger.createdAt = query.value(8).toLongLong();

    return charger;
}

QVector<Charger> ChargerRepository::findByStationId(int stationId) {
    QVector<Charger> chargers;
    QSqlQuery query(Database::instance().db());
    query.prepare("SELECT chargerId, stationId, code, type, status, powerKw, "
                  "totalChargeCount, totalChargeDurationSeconds, createdAt "
                  "FROM chargers WHERE stationId = ? ORDER BY code");
    query.addBindValue(stationId);

    if (!query.exec()) {
        qWarning() << "Failed to query chargers by station:" << query.lastError().text();
        return chargers;
    }

    while (query.next()) {
        Charger charger;
        charger.chargerId = query.value(0).toInt();
        charger.stationId = query.value(1).toInt();
        charger.code = query.value(2).toString();
        charger.type = query.value(3).toInt();
        charger.status = query.value(4).toInt();
        charger.powerKw = query.value(5).toDouble();
        charger.totalChargeCount = query.value(6).toInt();
        charger.totalChargeDurationSeconds = query.value(7).toInt();
        charger.createdAt = query.value(8).toLongLong();
        chargers.append(charger);
    }

    return chargers;
}

QVector<Charger> ChargerRepository::findAll() {
    QVector<Charger> chargers;
    QSqlQuery query(Database::instance().db());

    if (!query.exec("SELECT chargerId, stationId, code, type, status, powerKw, "
                    "totalChargeCount, totalChargeDurationSeconds, createdAt "
                    "FROM chargers ORDER BY stationId, code")) {
        qWarning() << "Failed to query all chargers:" << query.lastError().text();
        return chargers;
    }

    while (query.next()) {
        Charger charger;
        charger.chargerId = query.value(0).toInt();
        charger.stationId = query.value(1).toInt();
        charger.code = query.value(2).toString();
        charger.type = query.value(3).toInt();
        charger.status = query.value(4).toInt();
        charger.powerKw = query.value(5).toDouble();
        charger.totalChargeCount = query.value(6).toInt();
        charger.totalChargeDurationSeconds = query.value(7).toInt();
        charger.createdAt = query.value(8).toLongLong();
        chargers.append(charger);
    }

    return chargers;
}

QVector<ChargerWithStation> ChargerRepository::findAllWithStation() {
    QVector<ChargerWithStation> result;
    QSqlQuery query(Database::instance().db());

    QString sql = R"(
        SELECT c.chargerId, c.stationId, c.code, c.type, c.status, c.powerKw,
               c.totalChargeCount, c.totalChargeDurationSeconds, c.createdAt,
               s.name as stationName
        FROM chargers c
        JOIN stations s ON c.stationId = s.stationId
        ORDER BY c.stationId, c.code
    )";

    if (!query.exec(sql)) {
        qWarning() << "Failed to query chargers with station:" << query.lastError().text();
        return result;
    }

    while (query.next()) {
        ChargerWithStation cws;
        cws.charger.chargerId = query.value(0).toInt();
        cws.charger.stationId = query.value(1).toInt();
        cws.charger.code = query.value(2).toString();
        cws.charger.type = query.value(3).toInt();
        cws.charger.status = query.value(4).toInt();
        cws.charger.powerKw = query.value(5).toDouble();
        cws.charger.totalChargeCount = query.value(6).toInt();
        cws.charger.totalChargeDurationSeconds = query.value(7).toInt();
        cws.charger.createdAt = query.value(8).toLongLong();
        cws.stationName = query.value(9).toString();
        result.append(cws);
    }

    return result;
}

bool ChargerRepository::updateStatus(int chargerId, int status) {
    QSqlQuery query(Database::instance().db());
    query.prepare("UPDATE chargers SET status = ? WHERE chargerId = ?");
    query.addBindValue(status);
    query.addBindValue(chargerId);

    if (!query.exec()) {
        qWarning() << "Failed to update charger status:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

int ChargerRepository::create(const Charger& charger) {
    QSqlQuery query(Database::instance().db());
    query.prepare("INSERT INTO chargers (stationId, code, type, status, powerKw, createdAt) "
                  "VALUES (?, ?, ?, ?, ?, ?)");
    query.addBindValue(charger.stationId);
    query.addBindValue(charger.code);
    query.addBindValue(charger.type);
    query.addBindValue(charger.status);
    query.addBindValue(charger.powerKw);
    query.addBindValue(QDateTime::currentMSecsSinceEpoch());

    if (!query.exec()) {
        qWarning() << "Failed to create charger:" << query.lastError().text();
        return 0;
    }

    return query.lastInsertId().toInt();
}

bool ChargerRepository::incrementStats(int chargerId, int durationSeconds) {
    QSqlQuery query(Database::instance().db());
    query.prepare("UPDATE chargers SET totalChargeCount = totalChargeCount + 1, "
                  "totalChargeDurationSeconds = totalChargeDurationSeconds + ? "
                  "WHERE chargerId = ?");
    query.addBindValue(durationSeconds);
    query.addBindValue(chargerId);

    if (!query.exec()) {
        qWarning() << "Failed to increment charger stats:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

ChargerRepository::StatusCount ChargerRepository::countByStatus() {
    StatusCount count = {0, 0, 0, 0, 0};
    QSqlQuery query(Database::instance().db());

    if (!query.exec("SELECT status, COUNT(*) FROM chargers GROUP BY status")) {
        qWarning() << "Failed to count chargers by status:" << query.lastError().text();
        return count;
    }

    while (query.next()) {
        int status = query.value(0).toInt();
        int cnt = query.value(1).toInt();
        count.total += cnt;

        switch (status) {
            case 0: count.idle = cnt; break;
            case 1: count.charging = cnt; break;
            case 2: count.fault = cnt; break;
            case 3: count.offline = cnt; break;
        }
    }

    return count;
}

} // namespace repository
