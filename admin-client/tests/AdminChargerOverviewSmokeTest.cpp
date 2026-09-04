#include "api/AdminApiClient.h"
#include "model/ChargerStatusOverview.h"
#include "net/NetworkClient.h"
#include "session/AdminSession.h"

#include <QCoreApplication>
#include <QJsonObject>
#include <QTimer>

#include <cstdio>

namespace {

bool verifyZeroAndInvalidPayloads()
{
    ChargerStatusOverview overview;
    QString error;
    const QJsonObject zeroPayload {
        {QStringLiteral("total"), 0},
        {QStringLiteral("idle"), 0},
        {QStringLiteral("charging"), 0},
        {QStringLiteral("fault"), 0},
        {QStringLiteral("offline"), 0},
        {QStringLiteral("idlePercent"), 0.0},
        {QStringLiteral("chargingPercent"), 0.0},
        {QStringLiteral("faultPercent"), 0.0},
        {QStringLiteral("offlinePercent"), 0.0},
    };
    if (!ChargerStatusOverview::fromJson(zeroPayload, &overview, &error)) {
        return false;
    }

    QJsonObject invalidPayload = zeroPayload;
    invalidPayload.insert(QStringLiteral("total"), 1);
    return !ChargerStatusOverview::fromJson(invalidPayload, &overview, &error);
}

void finish(bool ok, const char* message)
{
    std::printf("%s\n", message);
    QCoreApplication::exit(ok ? 0 : 1);
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    NetworkClient network;
    AdminSession session;
    AdminApiClient api(&network, &session);

    if (!verifyZeroAndInvalidPayloads()) {
        std::printf("CHARGER OVERVIEW SMOKE TEST FAILED (payload validation)\n");
        return 1;
    }

    QObject::connect(&api, &AdminApiClient::loginSucceeded, &application, [&] {
        const bool firstStarted = api.requestChargerOverview(
            [](std::optional<ChargerStatusOverview> overview,
               const QString&) {
                const bool ok = overview.has_value()
                    && overview->total == overview->idle + overview->charging
                        + overview->fault + overview->offline;
                finish(ok, ok ? "CHARGER OVERVIEW SMOKE TEST PASSED"
                              : "CHARGER OVERVIEW SMOKE TEST FAILED (response)");
            });
        const bool duplicateStarted = api.requestChargerOverview({});
        if (!firstStarted || duplicateStarted) {
            finish(false, "CHARGER OVERVIEW SMOKE TEST FAILED (duplicate request guard)");
        }
    });
    QObject::connect(&api, &AdminApiClient::loginFailed, &application,
                     [](int, const QString&) {
        finish(false, "CHARGER OVERVIEW SMOKE TEST FAILED (login)");
    });

    QTimer::singleShot(0, &application, [&api] {
        api.login(QStringLiteral("admin"), QStringLiteral("123456"));
    });
    QTimer::singleShot(15000, &application, [] {
        finish(false, "CHARGER OVERVIEW SMOKE TEST FAILED (timeout)");
    });
    return application.exec();
}
