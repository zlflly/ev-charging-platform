#include "AdminRepository.h"
#include "Database.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

namespace repository {

std::optional<Admin> AdminRepository::authenticate(const QString& account, const QString& password) {
    QSqlQuery query(Database::instance().db());
    query.prepare("SELECT adminId, account, displayName, createdAt "
                  "FROM admins WHERE account = ? AND password = ?");
    query.addBindValue(account);
    query.addBindValue(password);

    if (!query.exec()) {
        qWarning() << "Failed to authenticate admin:" << query.lastError().text();
        return std::nullopt;
    }

    if (!query.next()) {
        return std::nullopt;
    }

    Admin admin;
    admin.adminId = query.value(0).toInt();
    admin.account = query.value(1).toString();
    admin.displayName = query.value(2).toString();
    admin.createdAt = query.value(3).toLongLong();

    return admin;
}

std::optional<Admin> AdminRepository::findById(int adminId) {
    QSqlQuery query(Database::instance().db());
    query.prepare("SELECT adminId, account, displayName, createdAt "
                  "FROM admins WHERE adminId = ?");
    query.addBindValue(adminId);

    if (!query.exec()) {
        qWarning() << "Failed to query admin by id:" << query.lastError().text();
        return std::nullopt;
    }

    if (!query.next()) {
        return std::nullopt;
    }

    Admin admin;
    admin.adminId = query.value(0).toInt();
    admin.account = query.value(1).toString();
    admin.displayName = query.value(2).toString();
    admin.createdAt = query.value(3).toLongLong();

    return admin;
}

} // namespace repository
