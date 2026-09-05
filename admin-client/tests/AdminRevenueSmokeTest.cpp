#include "api/AdminApiClient.h"
#include "model/Revenue.h"
#include "net/NetworkClient.h"
#include "session/AdminSession.h"

#include <QCoreApplication>
#include <QDate>
#include <QJsonArray>
#include <QJsonObject>
#include <QStringList>
#include <QTimer>

#include <cstdio>

namespace {

void finish(bool ok, const char* message)
{
    std::printf("%s\n", message);
    QCoreApplication::exit(ok ? 0 : 1);
}

QJsonObject validSummary()
{
    return {
        {QStringLiteral("todayRevenue"), 12.50},
        {QStringLiteral("monthRevenue"), 120.75},
        {QStringLiteral("totalRevenue"), 1020.25},
        {QStringLiteral("currency"), QStringLiteral("CNY")},
        {QStringLiteral("timezone"), QStringLiteral("Asia/Shanghai")},
        {QStringLiteral("generatedAt"), QStringLiteral("2026-09-05T12:00:00Z")},
    };
}

QJsonObject validTrend(int days)
{
    QJsonArray points;
    const QDate lastDate(2026, 9, 5);
    for (int offset = days - 1; offset >= 0; --offset) {
        points.append(QJsonObject {
            {QStringLiteral("date"),
             lastDate.addDays(-offset).toString(QStringLiteral("yyyy-MM-dd"))},
            {QStringLiteral("revenue"), offset % 3 == 0 ? 0.0 : 20.5 + offset},
        });
    }
    return {
        {QStringLiteral("days"), days},
        {QStringLiteral("timezone"), QStringLiteral("Asia/Shanghai")},
        {QStringLiteral("generatedAt"), QStringLiteral("2026-09-05T12:00:00Z")},
        {QStringLiteral("points"), points},
    };
}

bool verifyPayloadValidation()
{
    RevenueSummary summary;
    RevenueTrend trend;
    QString error;
    if (!RevenueSummary::fromJson(validSummary(), &summary, &error)
        || summary.formattedTodayRevenue() != QStringLiteral("¥ 12.50")) {
        return false;
    }
    QJsonObject contradictory = validSummary();
    contradictory.insert(QStringLiteral("todayRevenue"), 200.0);
    if (RevenueSummary::fromJson(contradictory, &summary, &error)) return false;

    if (!RevenueTrend::fromJson(validTrend(7), 7, &trend, &error)
        || trend.points.size() != 7) {
        return false;
    }
    QJsonObject missingDay = validTrend(7);
    QJsonArray points = missingDay.value(QStringLiteral("points")).toArray();
    points.removeLast();
    missingDay.insert(QStringLiteral("points"), points);
    if (RevenueTrend::fromJson(missingDay, 7, &trend, &error)) return false;

    QJsonObject nonConsecutive = validTrend(7);
    points = nonConsecutive.value(QStringLiteral("points")).toArray();
    QJsonObject point = points.at(3).toObject();
    point.insert(QStringLiteral("date"), QStringLiteral("2026-09-05"));
    points.replace(3, point);
    nonConsecutive.insert(QStringLiteral("points"), points);
    return !RevenueTrend::fromJson(nonConsecutive, 7, &trend, &error);
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    if (!verifyPayloadValidation()) {
        std::printf("REVENUE SMOKE TEST FAILED (payload validation)\n");
        return 1;
    }

    NetworkClient network;
    AdminSession session;
    AdminApiClient api(&network, &session);
    int trendCallbacks = 0;
    QStringList callbackOrder;

    QObject::connect(&api, &AdminApiClient::loginSucceeded, &application, [&] {
        const bool summaryStarted = api.requestRevenueSummary(
            [&](std::optional<RevenueSummary> summary, const QString&) {
                if (!summary || summary->todayRevenue > summary->monthRevenue
                    || summary->monthRevenue > summary->totalRevenue) {
                    finish(false, "REVENUE SMOKE TEST FAILED (summary response)");
                    return;
                }
                const auto checkTrend = [&](int expectedDays,
                                            std::optional<RevenueTrend> trend) {
                    if (!trend || trend->days != expectedDays
                        || trend->points.size() != expectedDays) {
                        finish(false, "REVENUE SMOKE TEST FAILED (trend response)");
                        return;
                    }
                    callbackOrder.append(QString::number(expectedDays));
                    ++trendCallbacks;
                    if (trendCallbacks == 2) {
                        const bool ok = callbackOrder
                            == QStringList{QStringLiteral("30"), QStringLiteral("7")}
                            && !api.isRevenueTrendInFlight();
                        finish(ok, ok ? "REVENUE SMOKE TEST PASSED"
                                      : "REVENUE SMOKE TEST FAILED (out-of-order matching)");
                    }
                };
                const bool sevenStarted = api.requestRevenueTrend(
                    7, [&, checkTrend](std::optional<RevenueTrend> trend,
                                      const QString&) { checkTrend(7, trend); });
                const bool thirtyStarted = api.requestRevenueTrend(
                    30, [&, checkTrend](std::optional<RevenueTrend> trend,
                                       const QString&) { checkTrend(30, trend); });
                if (!sevenStarted || !thirtyStarted
                    || !api.isRevenueTrendInFlight()) {
                    finish(false, "REVENUE SMOKE TEST FAILED (concurrent trend requests)");
                }
            });
        const bool duplicateSummary = api.requestRevenueSummary({});
        if (!summaryStarted || duplicateSummary) {
            finish(false, "REVENUE SMOKE TEST FAILED (summary request guard)");
        }
    });
    QObject::connect(&api, &AdminApiClient::loginFailed, &application,
                     [](int, const QString&) {
        finish(false, "REVENUE SMOKE TEST FAILED (login)");
    });
    QTimer::singleShot(0, &application, [&api] {
        api.login(QStringLiteral("admin"), QStringLiteral("123456"));
    });
    QTimer::singleShot(15000, &application, [] {
        finish(false, "REVENUE SMOKE TEST FAILED (timeout)");
    });
    return application.exec();
}
