#include "Database.h"
#include <QDateTime>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QFile>

namespace repository {

Database& Database::instance() {
    static Database instance;
    return instance;
}

Database::~Database() {
    close();
}

bool Database::initialize(const QString& dbPath) {
    // 如果已打开，先关闭
    if (m_db.isOpen()) {
        close();
    }

    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        m_lastError = QString("Failed to open database: %1").arg(m_db.lastError().text());
        qCritical() << m_lastError;
        return false;
    }

    qInfo() << "Database opened:" << dbPath;

    // 启用外键约束
    QSqlQuery query(m_db);
    if (!query.exec("PRAGMA foreign_keys = ON")) {
        m_lastError = QString("Failed to enable foreign keys: %1").arg(query.lastError().text());
        qWarning() << m_lastError;
    }

    // 建表
    if (!createTables()) {
        return false;
    }

    // 插入种子数据
    if (!insertSeedData()) {
        return false;
    }

    qInfo() << "Database initialized successfully";
    return true;
}

void Database::close() {
    if (m_db.isOpen()) {
        QString connectionName = m_db.connectionName();
        m_db.close();
        QSqlDatabase::removeDatabase(connectionName);
        qInfo() << "Database closed";
    }
}

bool Database::createTables() {
    QSqlQuery query(m_db);

    // 用户表
    QString createUsers = R"(
        CREATE TABLE IF NOT EXISTS users (
            userId INTEGER PRIMARY KEY AUTOINCREMENT,
            phone TEXT UNIQUE NOT NULL,
            nickname TEXT NOT NULL,
            avatar TEXT,
            balance REAL NOT NULL DEFAULT 50.0,
            status INTEGER NOT NULL DEFAULT 0,
            createdAt INTEGER NOT NULL,
            updatedAt INTEGER NOT NULL
        )
    )";

    // 管理员表
    QString createAdmins = R"(
        CREATE TABLE IF NOT EXISTS admins (
            adminId INTEGER PRIMARY KEY AUTOINCREMENT,
            account TEXT UNIQUE NOT NULL,
            password TEXT NOT NULL,
            displayName TEXT NOT NULL,
            createdAt INTEGER NOT NULL
        )
    )";

    // 充电站表
    QString createStations = R"(
        CREATE TABLE IF NOT EXISTS stations (
            stationId INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            address TEXT NOT NULL,
            latitude REAL NOT NULL,
            longitude REAL NOT NULL,
            pricePerKwh REAL NOT NULL,
            status INTEGER NOT NULL DEFAULT 0,
            version INTEGER NOT NULL DEFAULT 1,
            createdAt INTEGER NOT NULL
        )
    )";

    // 充电桩表
    QString createChargers = R"(
        CREATE TABLE IF NOT EXISTS chargers (
            chargerId INTEGER PRIMARY KEY AUTOINCREMENT,
            stationId INTEGER NOT NULL,
            code TEXT NOT NULL,
            type INTEGER NOT NULL,
            status INTEGER NOT NULL DEFAULT 0,
            powerKw REAL NOT NULL,
            totalChargeCount INTEGER NOT NULL DEFAULT 0,
            totalChargeDurationSeconds INTEGER NOT NULL DEFAULT 0,
            createdAt INTEGER NOT NULL,
            FOREIGN KEY (stationId) REFERENCES stations(stationId)
        )
    )";

    // 订单表
    QString createOrders = R"(
        CREATE TABLE IF NOT EXISTS orders (
            orderId INTEGER PRIMARY KEY AUTOINCREMENT,
            userId INTEGER NOT NULL,
            chargerId INTEGER NOT NULL,
            stationId INTEGER NOT NULL,
            status TEXT NOT NULL,
            startTime INTEGER NOT NULL DEFAULT 0,
            stopTime INTEGER NOT NULL DEFAULT 0,
            settleTime INTEGER NOT NULL DEFAULT 0,
            duration INTEGER NOT NULL DEFAULT 0,
            energyKwh REAL NOT NULL DEFAULT 0.0,
            amount REAL NOT NULL DEFAULT 0.0,
            createdAt INTEGER NOT NULL,
            FOREIGN KEY (userId) REFERENCES users(userId),
            FOREIGN KEY (chargerId) REFERENCES chargers(chargerId),
            FOREIGN KEY (stationId) REFERENCES stations(stationId)
        )
    )";

    // 按顺序执行建表语句
    QStringList schemas = {createUsers, createAdmins, createStations, createChargers, createOrders};

    for (const QString& sql : schemas) {
        if (!query.exec(sql)) {
            m_lastError = QString("Failed to create table: %1").arg(query.lastError().text());
            qCritical() << m_lastError;
            qCritical() << "SQL:" << sql;
            return false;
        }
    }

    qInfo() << "All tables created successfully";
    return true;
}

bool Database::insertSeedData() {
    QSqlQuery query(m_db);

    // 检查是否已有种子数据（幂等性）
    query.exec("SELECT COUNT(*) FROM admins");
    if (query.next() && query.value(0).toInt() > 0) {
        qInfo() << "Seed data already exists, skipping insertion";
        return true;
    }

    m_db.transaction();

    qint64 now = QDateTime::currentMSecsSinceEpoch();

    // 插入管理员
    query.prepare("INSERT INTO admins (account, password, displayName, createdAt) VALUES (?, ?, ?, ?)");
    query.addBindValue("admin");
    query.addBindValue("123456");
    query.addBindValue("系统管理员");
    query.addBindValue(now);
    if (!query.exec()) {
        m_lastError = QString("Failed to insert admin: %1").arg(query.lastError().text());
        qCritical() << m_lastError;
        m_db.rollback();
        return false;
    }

    // 插入测试用户
    QStringList testPhones = {"13800138001", "13800138002", "13800138003"};
    for (int i = 0; i < testPhones.size(); ++i) {
        query.prepare("INSERT INTO users (phone, nickname, balance, status, createdAt, updatedAt) "
                      "VALUES (?, ?, ?, ?, ?, ?)");
        query.addBindValue(testPhones[i]);
        query.addBindValue(QString("用户%1").arg(testPhones[i].right(4)));
        query.addBindValue(50.0 + i * 50.0);  // 50, 100, 150
        query.addBindValue(0);  // 正常状态
        query.addBindValue(now);
        query.addBindValue(now);
        if (!query.exec()) {
            m_lastError = QString("Failed to insert test user: %1").arg(query.lastError().text());
            qCritical() << m_lastError;
            m_db.rollback();
            return false;
        }
    }

    // 插入充电站（北京理工大学附近）
    struct StationSeed {
        QString name;
        QString address;
        double lat;
        double lon;
        double price;
    };

    QVector<StationSeed> stations = {
        {"良乡大学城充电站", "北京市房山区良乡高教园区", 39.7335, 116.1437, 1.5},
        {"中关村智能充电站", "北京市海淀区中关村大街", 39.9869, 116.3152, 1.8},
        {"亦庄开发区充电站", "北京市大兴区亦庄经济开发区", 39.7950, 116.5063, 1.6}
    };

    QVector<int> stationIds;
    for (const auto& s : stations) {
        query.prepare("INSERT INTO stations (name, address, latitude, longitude, pricePerKwh, status, createdAt) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?)");
        query.addBindValue(s.name);
        query.addBindValue(s.address);
        query.addBindValue(s.lat);
        query.addBindValue(s.lon);
        query.addBindValue(s.price);
        query.addBindValue(0);  // active
        query.addBindValue(now);
        if (!query.exec()) {
            m_lastError = QString("Failed to insert station: %1").arg(query.lastError().text());
            qCritical() << m_lastError;
            m_db.rollback();
            return false;
        }
        stationIds.append(query.lastInsertId().toInt());
    }

    // 为每个站点插入充电桩（每站6个桩：4快充+2慢充，各种状态）
    for (int i = 0; i < stationIds.size(); ++i) {
        int stationId = stationIds[i];

        // 4个快充桩
        for (int j = 1; j <= 4; ++j) {
            int status = 0;  // 默认空闲
            if (j == 3) status = 2;  // 第3个快充桩故障
            if (j == 4 && i == 2) status = 3;  // 第3站第4个桩离线

            query.prepare("INSERT INTO chargers (stationId, code, type, status, powerKw, createdAt) "
                          "VALUES (?, ?, ?, ?, ?, ?)");
            query.addBindValue(stationId);
            query.addBindValue(QString("CP-S%1-F%2").arg(i + 1).arg(j));
            query.addBindValue(0);  // 快充
            query.addBindValue(status);
            query.addBindValue(120.0);
            query.addBindValue(now);
            if (!query.exec()) {
                m_lastError = QString("Failed to insert charger: %1").arg(query.lastError().text());
                qCritical() << m_lastError;
                m_db.rollback();
                return false;
            }
        }

        // 2个慢充桩
        for (int j = 1; j <= 2; ++j) {
            query.prepare("INSERT INTO chargers (stationId, code, type, status, powerKw, createdAt) "
                          "VALUES (?, ?, ?, ?, ?, ?)");
            query.addBindValue(stationId);
            query.addBindValue(QString("CP-S%1-S%2").arg(i + 1).arg(j));
            query.addBindValue(1);  // 慢充
            query.addBindValue(0);  // 空闲
            query.addBindValue(7.0);
            query.addBindValue(now);
            if (!query.exec()) {
                m_lastError = QString("Failed to insert charger: %1").arg(query.lastError().text());
                qCritical() << m_lastError;
                m_db.rollback();
                return false;
            }
        }
    }

    if (!m_db.commit()) {
        m_lastError = QString("Failed to commit seed data: %1").arg(m_db.lastError().text());
        qCritical() << m_lastError;
        return false;
    }

    qInfo() << "Seed data inserted successfully";
    qInfo() << "  - 1 admin account (admin/123456)";
    qInfo() << "  - 3 test users";
    qInfo() << "  - 3 charging stations";
    qInfo() << "  - 18 chargers (12 fast + 6 slow)";

    return true;
}

} // namespace repository
