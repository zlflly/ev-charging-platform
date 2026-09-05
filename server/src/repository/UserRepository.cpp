#include "UserRepository.h"
#include "Database.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

namespace repository {

std::optional<User> UserRepository::findByPhone(const QString& phone) {
    QSqlQuery query(Database::instance().db());
    query.prepare("SELECT userId, phone, nickname, avatar, balance, status, createdAt, updatedAt "
                  "FROM users WHERE phone = ?");
    query.addBindValue(phone);

    if (!query.exec()) {
        qWarning() << "Failed to query user by phone:" << query.lastError().text();
        return std::nullopt;
    }

    if (!query.next()) {
        return std::nullopt;
    }

    User user;
    user.userId = query.value(0).toInt();
    user.phone = query.value(1).toString();
    user.nickname = query.value(2).toString();
    user.avatar = query.value(3).toString();
    user.balance = query.value(4).toDouble();
    user.status = query.value(5).toInt();
    user.createdAt = query.value(6).toLongLong();
    user.updatedAt = query.value(7).toLongLong();

    return user;
}

std::optional<User> UserRepository::findById(int userId) {
    QSqlQuery query(Database::instance().db());
    query.prepare("SELECT userId, phone, nickname, avatar, balance, status, createdAt, updatedAt "
                  "FROM users WHERE userId = ?");
    query.addBindValue(userId);

    if (!query.exec()) {
        qWarning() << "Failed to query user by id:" << query.lastError().text();
        return std::nullopt;
    }

    if (!query.next()) {
        return std::nullopt;
    }

    User user;
    user.userId = query.value(0).toInt();
    user.phone = query.value(1).toString();
    user.nickname = query.value(2).toString();
    user.avatar = query.value(3).toString();
    user.balance = query.value(4).toDouble();
    user.status = query.value(5).toInt();
    user.createdAt = query.value(6).toLongLong();
    user.updatedAt = query.value(7).toLongLong();

    return user;
}

std::optional<User> UserRepository::create(const QString& phone) {
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    QString defaultNickname = QString("用户%1").arg(phone.right(4));

    QSqlQuery query(Database::instance().db());
    query.prepare("INSERT INTO users (phone, nickname, balance, status, createdAt, updatedAt) "
                  "VALUES (?, ?, ?, ?, ?, ?)");
    query.addBindValue(phone);
    query.addBindValue(defaultNickname);
    query.addBindValue(50.0);  // 初始余额
    query.addBindValue(0);     // 正常状态
    query.addBindValue(now);
    query.addBindValue(now);

    if (!query.exec()) {
        qWarning() << "Failed to create user:" << query.lastError().text();
        return std::nullopt;
    }

    int userId = query.lastInsertId().toInt();
    return findById(userId);
}

bool UserRepository::updateNickname(int userId, const QString& nickname) {
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    QSqlQuery query(Database::instance().db());
    query.prepare("UPDATE users SET nickname = ?, updatedAt = ? WHERE userId = ?");
    query.addBindValue(nickname);
    query.addBindValue(now);
    query.addBindValue(userId);

    if (!query.exec()) {
        qWarning() << "Failed to update nickname:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool UserRepository::updateAvatar(int userId, const QString& avatarData) {
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    QSqlQuery query(Database::instance().db());
    query.prepare("UPDATE users SET avatar = ?, updatedAt = ? WHERE userId = ?");
    query.addBindValue(avatarData);
    query.addBindValue(now);
    query.addBindValue(userId);

    if (!query.exec()) {
        qWarning() << "Failed to update avatar:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool UserRepository::updateBalance(int userId, double delta) {
    QSqlDatabase& db = Database::instance().db();
    db.transaction();

    QSqlQuery query(db);
    query.prepare("UPDATE users SET balance = balance + ?, updatedAt = ? WHERE userId = ?");
    query.addBindValue(delta);
    query.addBindValue(QDateTime::currentMSecsSinceEpoch());
    query.addBindValue(userId);

    if (!query.exec() || query.numRowsAffected() == 0) {
        qWarning() << "Failed to update balance:" << query.lastError().text();
        db.rollback();
        return false;
    }

    // 检查余额不能为负
    query.prepare("SELECT balance FROM users WHERE userId = ?");
    query.addBindValue(userId);
    if (!query.exec() || !query.next()) {
        qWarning() << "Failed to verify balance:" << query.lastError().text();
        db.rollback();
        return false;
    }

    double newBalance = query.value(0).toDouble();
    if (newBalance < 0) {
        qWarning() << "Balance cannot be negative";
        db.rollback();
        return false;
    }

    return db.commit();
}

bool UserRepository::setBalance(int userId, double newBalance) {
    if (newBalance < 0) {
        qWarning() << "Balance cannot be negative";
        return false;
    }

    qint64 now = QDateTime::currentMSecsSinceEpoch();

    QSqlQuery query(Database::instance().db());
    query.prepare("UPDATE users SET balance = ?, updatedAt = ? WHERE userId = ?");
    query.addBindValue(newBalance);
    query.addBindValue(now);
    query.addBindValue(userId);

    if (!query.exec()) {
        qWarning() << "Failed to set balance:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool UserRepository::updateStatus(int userId, int status) {
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    QSqlQuery query(Database::instance().db());
    query.prepare("UPDATE users SET status = ?, updatedAt = ? WHERE userId = ?");
    query.addBindValue(status);
    query.addBindValue(now);
    query.addBindValue(userId);

    if (!query.exec()) {
        qWarning() << "Failed to update user status:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

QVector<User> UserRepository::findAll() {
    QVector<User> users;
    QSqlQuery query(Database::instance().db());

    if (!query.exec("SELECT userId, phone, nickname, avatar, balance, status, createdAt, updatedAt "
                    "FROM users ORDER BY userId")) {
        qWarning() << "Failed to query all users:" << query.lastError().text();
        return users;
    }

    while (query.next()) {
        User user;
        user.userId = query.value(0).toInt();
        user.phone = query.value(1).toString();
        user.nickname = query.value(2).toString();
        user.avatar = query.value(3).toString();
        user.balance = query.value(4).toDouble();
        user.status = query.value(5).toInt();
        user.createdAt = query.value(6).toLongLong();
        user.updatedAt = query.value(7).toLongLong();
        users.append(user);
    }

    return users;
}

QVector<User> UserRepository::searchByPhone(const QString& phonePattern) {
    QVector<User> users;
    QSqlQuery query(Database::instance().db());
    query.prepare("SELECT userId, phone, nickname, avatar, balance, status, createdAt, updatedAt "
                  "FROM users WHERE phone LIKE ? ORDER BY userId");
    query.addBindValue("%" + phonePattern + "%");

    if (!query.exec()) {
        qWarning() << "Failed to search users by phone:" << query.lastError().text();
        return users;
    }

    while (query.next()) {
        User user;
        user.userId = query.value(0).toInt();
        user.phone = query.value(1).toString();
        user.nickname = query.value(2).toString();
        user.avatar = query.value(3).toString();
        user.balance = query.value(4).toDouble();
        user.status = query.value(5).toInt();
        user.createdAt = query.value(6).toLongLong();
        user.updatedAt = query.value(7).toLongLong();
        users.append(user);
    }

    return users;
}

} // namespace repository
