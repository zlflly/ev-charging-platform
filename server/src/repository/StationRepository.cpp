#include "StationRepository.h"
#include "Database.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

namespace repository {

std::optional<Station> StationRepository::findById(int stationId) {
    QSqlQuery query(Database::instance().db());
    query.prepare("SELECT stationId, name, address, latitude, longitude, pricePerKwh, status, version, createdAt "
                  "FROM stations WHERE stationId = ?");
    query.addBindValue(stationId);

    if (!query.exec()) {
        qWarning() << "Failed to query station by id:" << query.lastError().text();
        return std::nullopt;
    }

    if (!query.next()) {
        return std::nullopt;
    }

    Station station;
    station.stationId = query.value(0).toInt();
    station.name = query.value(1).toString();
    station.address = query.value(2).toString();
    station.latitude = query.value(3).toDouble();
    station.longitude = query.value(4).toDouble();
    station.pricePerKwh = query.value(5).toDouble();
    station.status = query.value(6).toInt();
    station.version = query.value(7).toInt();
    station.createdAt = query.value(8).toLongLong();

    return station;
}

QVector<Station> StationRepository::findAll() {
    QVector<Station> stations;
    QSqlQuery query(Database::instance().db());

    if (!query.exec("SELECT stationId, name, address, latitude, longitude, pricePerKwh, status, version, createdAt "
                    "FROM stations ORDER BY stationId")) {
        qWarning() << "Failed to query all stations:" << query.lastError().text();
        return stations;
    }

    while (query.next()) {
        Station station;
        station.stationId = query.value(0).toInt();
        station.name = query.value(1).toString();
        station.address = query.value(2).toString();
        station.latitude = query.value(3).toDouble();
        station.longitude = query.value(4).toDouble();
        station.pricePerKwh = query.value(5).toDouble();
        station.status = query.value(6).toInt();
        station.version = query.value(7).toInt();
        station.createdAt = query.value(8).toLongLong();
        stations.append(station);
    }

    return stations;
}

QVector<Station> StationRepository::findActive() {
    QVector<Station> stations;
    QSqlQuery query(Database::instance().db());

    if (!query.exec("SELECT stationId, name, address, latitude, longitude, pricePerKwh, status, version, createdAt "
                    "FROM stations WHERE status = 0 ORDER BY stationId")) {
        qWarning() << "Failed to query active stations:" << query.lastError().text();
        return stations;
    }

    while (query.next()) {
        Station station;
        station.stationId = query.value(0).toInt();
        station.name = query.value(1).toString();
        station.address = query.value(2).toString();
        station.latitude = query.value(3).toDouble();
        station.longitude = query.value(4).toDouble();
        station.pricePerKwh = query.value(5).toDouble();
        station.status = query.value(6).toInt();
        station.version = query.value(7).toInt();
        station.createdAt = query.value(8).toLongLong();
        stations.append(station);
    }

    return stations;
}

int StationRepository::create(const Station& station) {
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    QSqlQuery query(Database::instance().db());
    query.prepare("INSERT INTO stations (name, address, latitude, longitude, pricePerKwh, status, version, createdAt) "
                  "VALUES (?, ?, ?, ?, ?, ?, 1, ?)");
    query.addBindValue(station.name);
    query.addBindValue(station.address);
    query.addBindValue(station.latitude);
    query.addBindValue(station.longitude);
    query.addBindValue(station.pricePerKwh);
    query.addBindValue(0);  // active
    query.addBindValue(now);

    if (!query.exec()) {
        qWarning() << "Failed to create station:" << query.lastError().text();
        return 0;
    }

    return query.lastInsertId().toInt();
}

bool StationRepository::update(const Station& station) {
    QSqlQuery query(Database::instance().db());
    query.prepare("UPDATE stations SET name = ?, address = ?, latitude = ?, longitude = ?, "
                  "pricePerKwh = ?, version = version + 1 "
                  "WHERE stationId = ? AND version = ?");
    query.addBindValue(station.name);
    query.addBindValue(station.address);
    query.addBindValue(station.latitude);
    query.addBindValue(station.longitude);
    query.addBindValue(station.pricePerKwh);
    query.addBindValue(station.stationId);
    query.addBindValue(station.version);

    if (!query.exec()) {
        qWarning() << "Failed to update station:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool StationRepository::setStatus(int stationId, int status) {
    QSqlQuery query(Database::instance().db());
    query.prepare("UPDATE stations SET status = ? WHERE stationId = ?");
    query.addBindValue(status);
    query.addBindValue(stationId);

    if (!query.exec()) {
        qWarning() << "Failed to set station status:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

} // namespace repository
