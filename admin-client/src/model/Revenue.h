#pragma once

#include <QDate>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QtGlobal>

struct RevenueSummary final
{
    double todayRevenue = 0.0;
    double monthRevenue = 0.0;
    double totalRevenue = 0.0;
    QString currency;
    QString timezone;
    qint64 generatedAtEpochMs = 0;

    QString formattedTodayRevenue() const;
    QString formattedMonthRevenue() const;
    QString formattedTotalRevenue() const;
    QString formattedGeneratedAt() const;

    static bool fromJson(const QJsonObject& json,
                         RevenueSummary* summary,
                         QString* errorMessage);
};

struct RevenueTrendPoint final
{
    QDate date;
    double revenue = 0.0;
};

struct RevenueTrend final
{
    int days = 0;
    QString timezone;
    qint64 generatedAtEpochMs = 0;
    QList<RevenueTrendPoint> points;

    QString formattedGeneratedAt() const;

    static bool fromJson(const QJsonObject& json,
                         int expectedDays,
                         RevenueTrend* trend,
                         QString* errorMessage);
};
