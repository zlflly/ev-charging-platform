#include "api/AdminApiClient.h"
#include "model/Charger.h"
#include "net/NetworkClient.h"
#include "protocol/Protocol.h"
#include "session/AdminSession.h"

#include <QCoreApplication>
#include <QTimer>

#include <cstdio>

namespace {

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

    QObject::connect(&api, &AdminApiClient::loginSucceeded, &application, [&] {
        const bool listStarted = api.requestChargers(
            [&api](std::optional<QList<Charger>> chargers, const QString&) {
                if (!chargers || chargers->size() != 12) {
                    finish(false, "CHARGER MANAGEMENT SMOKE TEST FAILED (list)");
                    return;
                }

                qint64 idleId = 0;
                qint64 chargingId = 0;
                for (const Charger& charger : *chargers) {
                    if (charger.status == protocol::ChargerStatusIdle && idleId == 0) {
                        idleId = charger.chargerId;
                    }
                    if (charger.status == protocol::ChargerStatusCharging
                        && chargingId == 0) {
                        chargingId = charger.chargerId;
                    }
                }
                if (idleId <= 0 || chargingId <= 0) {
                    finish(false, "CHARGER MANAGEMENT SMOKE TEST FAILED (status fixtures)");
                    return;
                }

                const Charger sample = chargers->first();
                ChargerFilter filter;
                filter.keyword = sample.code.mid(1).toLower();
                filter.stationId = sample.stationId;
                filter.type = sample.type;
                filter.status = sample.status;
                if (!filter.matches(sample)) {
                    finish(false,
                           "CHARGER MANAGEMENT SMOKE TEST FAILED (combined filter)");
                    return;
                }
                filter.stationId = sample.stationId + 100000;
                if (filter.matches(sample)) {
                    finish(false,
                           "CHARGER MANAGEMENT SMOKE TEST FAILED (station filter)");
                    return;
                }

                const bool busyRestartStarted = api.restartCharger(
                    chargingId,
                    [&api, idleId](bool ok, const QString&) {
                        if (ok) {
                            finish(false,
                                   "CHARGER MANAGEMENT SMOKE TEST FAILED (busy restart)");
                            return;
                        }

                        const bool idleRestartStarted = api.restartCharger(
                            idleId,
                            [](bool restartOk, const QString&) {
                                finish(restartOk,
                                       restartOk
                                           ? "CHARGER MANAGEMENT SMOKE TEST PASSED"
                                           : "CHARGER MANAGEMENT SMOKE TEST FAILED (idle restart)");
                            });
                        const bool duplicateRestartStarted =
                            api.restartCharger(idleId, {});
                        if (!idleRestartStarted || duplicateRestartStarted) {
                            finish(false,
                                   "CHARGER MANAGEMENT SMOKE TEST FAILED (restart guard)");
                            return;
                        }
                    });
                if (!busyRestartStarted) {
                    finish(false,
                           "CHARGER MANAGEMENT SMOKE TEST FAILED (restart request)");
                    return;
                }
            });
        const bool duplicateListStarted = api.requestChargers({});
        if (!listStarted || duplicateListStarted) {
            finish(false, "CHARGER MANAGEMENT SMOKE TEST FAILED (list guard)");
            return;
        }
    });
    QObject::connect(&api, &AdminApiClient::loginFailed, &application,
                     [](int, const QString&) {
        finish(false, "CHARGER MANAGEMENT SMOKE TEST FAILED (login)");
    });

    QTimer::singleShot(0, &application, [&api] {
        api.login(QStringLiteral("admin"), QStringLiteral("123456"));
    });
    QTimer::singleShot(15000, &application, [] {
        finish(false, "CHARGER MANAGEMENT SMOKE TEST FAILED (timeout)");
    });
    return application.exec();
}
