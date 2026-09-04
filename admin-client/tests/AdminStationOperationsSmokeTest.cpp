#include "api/AdminApiClient.h"
#include "model/Charger.h"
#include "model/Station.h"
#include "net/NetworkClient.h"
#include "protocol/Protocol.h"
#include "session/AdminSession.h"

#include <QCoreApplication>
#include <QJsonObject>
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

    ChargerStatusUpdateRequest orderControlled;
    orderControlled.chargerId = 1007;
    orderControlled.expectedStatus = protocol::ChargerStatusCharging;
    orderControlled.targetStatus = protocol::ChargerStatusOffline;
    orderControlled.reason = QStringLiteral("非法强制下线");
    QString validationError;
    if (orderControlled.validate(&validationError)) {
        std::printf("STATION OPERATIONS SMOKE TEST FAILED (local order guard)\n");
        return 1;
    }

    QObject::connect(&api, &AdminApiClient::loginSucceeded, &application, [&] {
        const bool listStarted = api.requestStations(
            [&api](std::optional<QList<Station>> stations, const QString&) {
                if (!stations || stations->isEmpty()) {
                    finish(false, "STATION OPERATIONS SMOKE TEST FAILED (list)");
                    return;
                }
                const Station original = stations->first();
                if (original.version != 1) {
                    finish(false, "STATION OPERATIONS SMOKE TEST FAILED (version)");
                    return;
                }

                StationUpdateRequest update;
                update.stationId = original.stationId;
                update.expectedVersion = original.version;
                update.name = QStringLiteral("良乡大学城智慧站");
                update.address = original.address;
                update.latitude = original.latitude;
                update.longitude = original.longitude;
                const bool updateStarted = api.updateStation(
                    update,
                    [&api, update](std::optional<StationUpdateResult> result,
                                   const QString&) {
                        if (!result || result->version != 2) {
                            finish(false, "STATION OPERATIONS SMOKE TEST FAILED (update)");
                            return;
                        }

                        StationUpdateRequest stale = update;
                        stale.name = QStringLiteral("不应覆盖的新名称");
                        const bool staleStarted = api.updateStation(
                            stale,
                            [&api, stationId = update.stationId](
                                std::optional<StationUpdateResult> staleResult,
                                const QString&) {
                                if (staleResult) {
                                    finish(false, "STATION OPERATIONS SMOKE TEST FAILED (stale accepted)");
                                    return;
                                }
                                const bool reloadStarted = api.requestStations(
                                    [&api, stationId](
                                        std::optional<QList<Station>> reloaded,
                                        const QString&) {
                                        if (!reloaded) {
                                            finish(false, "STATION OPERATIONS SMOKE TEST FAILED (reload)");
                                            return;
                                        }
                                        bool updated = false;
                                        for (const Station& station : *reloaded) {
                                            if (station.stationId == stationId) {
                                                updated = station.version == 2
                                                    && station.name
                                                        == QStringLiteral("良乡大学城智慧站");
                                            }
                                        }
                                        if (!updated) {
                                            finish(false, "STATION OPERATIONS SMOKE TEST FAILED (update missing)");
                                            return;
                                        }
                                        const bool detailStarted = api.requestStationDetail(
                                            stationId,
                                            [&api, stationId](
                                                std::optional<StationDetail> detail,
                                                const QString&) {
                                                if (!detail || detail->version != 2
                                                    || detail->chargers.isEmpty()) {
                                                    finish(false, "STATION OPERATIONS SMOKE TEST FAILED (detail)");
                                                    return;
                                                }
                                                const StationCharger charger =
                                                    detail->chargers.first();
                                                ChargerStatusUpdateRequest statusUpdate;
                                                statusUpdate.chargerId = charger.chargerId;
                                                statusUpdate.expectedStatus = charger.status;
                                                statusUpdate.targetStatus =
                                                    protocol::ChargerStatusOffline;
                                                statusUpdate.reason =
                                                    QStringLiteral("计划检修下线");
                                                const bool statusStarted =
                                                    api.updateChargerStatus(
                                                        statusUpdate,
                                                        [&api, stationId, statusUpdate](
                                                            std::optional<ChargerStatusUpdateResult> statusResult,
                                                            const QString&) {
                                                            if (!statusResult
                                                                || statusResult->status
                                                                    != protocol::ChargerStatusOffline) {
                                                                finish(false, "STATION OPERATIONS SMOKE TEST FAILED (status update)");
                                                                return;
                                                            }
                                                            ChargerStatusUpdateRequest staleStatus =
                                                                statusUpdate;
                                                            staleStatus.targetStatus =
                                                                protocol::ChargerStatusFault;
                                                            const bool staleStatusStarted =
                                                                api.updateChargerStatus(
                                                                    staleStatus,
                                                                    [&api, stationId, statusUpdate](
                                                                        std::optional<ChargerStatusUpdateResult> staleResult,
                                                                        const QString&) {
                                                                        if (staleResult) {
                                                                            finish(false, "STATION OPERATIONS SMOKE TEST FAILED (stale status accepted)");
                                                                            return;
                                                                        }
                                                                        const bool listAgainStarted =
                                                                            api.requestStations(
                                                                                [&api, stationId, statusUpdate](
                                                                                    std::optional<QList<Station>> stationsAgain,
                                                                                    const QString&) {
                                                                                    bool rateUpdated = false;
                                                                                    if (stationsAgain) {
                                                                                        for (const Station& station : *stationsAgain) {
                                                                                            if (station.stationId == stationId
                                                                                                && station.onlineRate == 75.0) {
                                                                                                rateUpdated = true;
                                                                                            }
                                                                                        }
                                                                                    }
                                                                                    if (!rateUpdated) {
                                                                                        finish(false, "STATION OPERATIONS SMOKE TEST FAILED (online rate)");
                                                                                        return;
                                                                                    }
                                                                                    QJsonObject forced {
                                                                                        {QStringLiteral("chargerId"), 1007},
                                                                                        {QStringLiteral("expectedStatus"), protocol::ChargerStatusCharging},
                                                                                        {QStringLiteral("targetStatus"), protocol::ChargerStatusOffline},
                                                                                        {QStringLiteral("reason"), QStringLiteral("非法强制下线")},
                                                                                    };
                                                                                    const QString requestId = api.sendAuthenticated(
                                                                                        QString::fromUtf8(protocol::action::kAdminChargerStatusUpdate),
                                                                                        forced,
                                                                                        [](const protocol::Response& response) {
                                                                                            const bool rejected = response.code
                                                                                                == protocol::CodeChargerOperationRejected;
                                                                                            finish(rejected,
                                                                                                rejected
                                                                                                    ? "STATION OPERATIONS SMOKE TEST PASSED"
                                                                                                    : "STATION OPERATIONS SMOKE TEST FAILED (active order accepted)");
                                                                                        });
                                                                                    if (requestId.isEmpty()) {
                                                                                        finish(false, "STATION OPERATIONS SMOKE TEST FAILED (forced request)");
                                                                                    }
                                                                                });
                                                                        if (!listAgainStarted) {
                                                                            finish(false, "STATION OPERATIONS SMOKE TEST FAILED (rate request)");
                                                                        }
                                                                    });
                                                            if (!staleStatusStarted) {
                                                                finish(false, "STATION OPERATIONS SMOKE TEST FAILED (stale status request)");
                                                            }
                                                        });
                                                if (!statusStarted) {
                                                    finish(false, "STATION OPERATIONS SMOKE TEST FAILED (status request)");
                                                }
                                            });
                                        if (!detailStarted) {
                                            finish(false, "STATION OPERATIONS SMOKE TEST FAILED (detail request)");
                                        }
                                    });
                                if (!reloadStarted) {
                                    finish(false, "STATION OPERATIONS SMOKE TEST FAILED (reload request)");
                                }
                            });
                        if (!staleStarted) {
                            finish(false, "STATION OPERATIONS SMOKE TEST FAILED (stale request)");
                        }
                    });
                const bool duplicateStarted = api.updateStation(update, {});
                if (!updateStarted || duplicateStarted) {
                    finish(false, "STATION OPERATIONS SMOKE TEST FAILED (update guard)");
                }
            });
        if (!listStarted) {
            finish(false, "STATION OPERATIONS SMOKE TEST FAILED (list request)");
        }
    });
    QObject::connect(&api, &AdminApiClient::loginFailed, &application,
                     [](int, const QString&) {
        finish(false, "STATION OPERATIONS SMOKE TEST FAILED (login)");
    });
    QTimer::singleShot(0, &application, [&api] {
        api.login(QStringLiteral("admin"), QStringLiteral("123456"));
    });
    QTimer::singleShot(20000, &application, [] {
        finish(false, "STATION OPERATIONS SMOKE TEST FAILED (timeout)");
    });
    return application.exec();
}
