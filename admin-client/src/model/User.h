#pragma once

#include <QJsonObject>
#include <QList>
#include <QString>
#include <QtGlobal>

#include <optional>

struct AdminUserActiveOrder final
{
    qint64 orderId = 0;
    qint64 stationId = 0;
    QString stationName;
    qint64 chargerId = 0;
    QString chargerCode;
    QString status;

    QString statusLabel() const;
    QString deviceLabel() const;

    static bool fromJson(const QJsonObject& json,
                         AdminUserActiveOrder* order,
                         QString* errorMessage);
};

struct AdminUser final
{
    qint64 userId = 0;
    QString phone;
    QString nickname;
    double balance = 0.0;
    qint64 createdAtEpochMs = 0;
    int status = 0;
    QString activityStatus;
    std::optional<AdminUserActiveOrder> activeOrder;

    QString statusLabel() const;
    QString formattedBalance() const;
    QString formattedCreatedAt() const;
    QString activityLabel() const;
    QString currentDeviceLabel() const;

    static bool fromJson(const QJsonObject& json,
                         AdminUser* user,
                         QString* errorMessage);
};

struct UserListQuery final
{
    int page = 1;
    int pageSize = 20;
    QString phoneKeyword;
    // -1 表示全部，0 表示正常，1 表示冻结。
    int status = -1;
    // ALL / ACTIVE / IDLE；由服务端筛选，不能只过滤当前页。
    QString activityFilter = QStringLiteral("ALL");

    bool validate(QString* errorMessage) const;
    QJsonObject toJson() const;
};

struct UserListPage final
{
    QList<AdminUser> users;
    qint64 total = 0;
    int page = 1;
    int pageSize = 20;

    static bool fromJson(const QJsonObject& json,
                         const UserListQuery& query,
                         UserListPage* result,
                         QString* errorMessage);
};

struct UserStatusUpdateRequest final
{
    qint64 userId = 0;
    int expectedStatus = 0;
    int targetStatus = 1;
    QString reason;

    bool validate(QString* errorMessage) const;
    QJsonObject toJson() const;
};

struct UserStatusUpdateResult final
{
    qint64 userId = 0;
    int previousStatus = 0;
    int status = 0;
    qint64 changedAtEpochMs = 0;

    static bool fromJson(const QJsonObject& json,
                         UserStatusUpdateResult* result,
                         QString* errorMessage);
};
