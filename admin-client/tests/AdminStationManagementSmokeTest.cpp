#include "api/AdminApiClient.h"
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

    QObject::connect(&api, &AdminApiClient::loginSucceeded, &application, [&] {
        const bool listStarted = api.requestStations(
            [&api](std::optional<QList<Station>> stations, const QString&) {
                if (!stations || stations->size() != 4) {
                    finish(false, "STATION MANAGEMENT SMOKE TEST FAILED (initial list)");
                    return;
                }

                qint64 emptyStationId = 0;
                for (const Station& station : *stations) {
                    if (station.totalCount == 0 && station.onlineRate == 0.0) {
                        emptyStationId = station.stationId;
                        break;
                    }
                }
                StationFilter filter;
                filter.keyword = QStringLiteral("亦庄");
                if (emptyStationId <= 0
                    || !filter.matches(stations->last())) {
                    finish(false, "STATION MANAGEMENT SMOKE TEST FAILED (empty/filter)");
                    return;
                }

                const bool detailStarted = api.requestStationDetail(
                    emptyStationId,
                    [&api](std::optional<StationDetail> detail, const QString&) {
                        if (!detail || !detail->chargers.isEmpty()) {
                            finish(false,
                                   "STATION MANAGEMENT SMOKE TEST FAILED (empty detail)");
                            return;
                        }

                        QJsonObject invalidData {
                            {QStringLiteral("name"), QStringLiteral("无效站点")},
                            {QStringLiteral("address"), QStringLiteral("测试地址")},
                            {QStringLiteral("latitude"), 100.0},
                            {QStringLiteral("longitude"), 116.4},
                            {QStringLiteral("chargerCount"), 2},
                        };
                        const QString invalidRequestId = api.sendAuthenticated(
                            QString::fromUtf8(protocol::action::kAdminStationCreate),
                            invalidData,
                            [&api](const protocol::Response& response) {
                                if (response.isOk()) {
                                    finish(false,
                                           "STATION MANAGEMENT SMOKE TEST FAILED (invalid accepted)");
                                    return;
                                }

                                const bool unchangedStarted = api.requestStations(
                                    [&api](std::optional<QList<Station>> unchanged,
                                           const QString&) {
                                        if (!unchanged || unchanged->size() != 4) {
                                            finish(false,
                                                   "STATION MANAGEMENT SMOKE TEST FAILED (failed create polluted list)");
                                            return;
                                        }

                                        StationCreateRequest request;
                                        request.name = QStringLiteral("通州智慧能源站");
                                        request.address = QStringLiteral("北京市通州区运河东大街");
                                        request.latitude = 39.902500;
                                        request.longitude = 116.656300;
                                        request.pricePerKwh = 1.40;
                                        request.chargerCount = 2;
                                        const bool createStarted = api.createStation(
                                            request,
                                            [&api](std::optional<StationCreateResult> result,
                                                   const QString&) {
                                                if (!result
                                                    || result->createdChargerCount != 2) {
                                                    finish(false,
                                                           "STATION MANAGEMENT SMOKE TEST FAILED (create)");
                                                    return;
                                                }
                                                const qint64 createdId = result->stationId;
                                                const bool reloadStarted = api.requestStations(
                                                    [&api, createdId](
                                                        std::optional<QList<Station>> reloaded,
                                                        const QString&) {
                                                        if (!reloaded || reloaded->size() != 5) {
                                                            finish(false,
                                                                   "STATION MANAGEMENT SMOKE TEST FAILED (reload)");
                                                            return;
                                                        }
                                                        bool found = false;
                                                        for (const Station& station : *reloaded) {
                                                            if (station.stationId == createdId
                                                                && station.totalCount == 2
                                                                && station.onlineRate == 100.0) {
                                                                found = true;
                                                            }
                                                        }
                                                        if (!found) {
                                                            finish(false,
                                                                   "STATION MANAGEMENT SMOKE TEST FAILED (created station missing)");
                                                            return;
                                                        }
                                                        const bool createdDetailStarted =
                                                            api.requestStationDetail(
                                                                createdId,
                                                                [](std::optional<StationDetail> createdDetail,
                                                                   const QString&) {
                                                                    bool ok = createdDetail
                                                                        && createdDetail->chargers.size() == 2
                                                                        && createdDetail->totalCount == 2
                                                                        && createdDetail->availableCount == 2
                                                                        && createdDetail->pricePerKwh == 1.40;
                                                                    if (ok) {
                                                                        for (const StationCharger& charger
                                                                             : createdDetail->chargers) {
                                                                            ok = ok
                                                                                && charger.status
                                                                                    == protocol::ChargerStatusIdle;
                                                                        }
                                                                    }
                                                                    finish(ok,
                                                                        ok
                                                                            ? "STATION MANAGEMENT SMOKE TEST PASSED"
                                                                            : "STATION MANAGEMENT SMOKE TEST FAILED (created detail)");
                                                                });
                                                        if (!createdDetailStarted) {
                                                            finish(false,
                                                                   "STATION MANAGEMENT SMOKE TEST FAILED (created detail request)");
                                                        }
                                                    });
                                                if (!reloadStarted) {
                                                    finish(false,
                                                           "STATION MANAGEMENT SMOKE TEST FAILED (reload request)");
                                                }
                                            });
                                        const bool duplicateCreateStarted =
                                            api.createStation(request, {});
                                        if (!createStarted || duplicateCreateStarted) {
                                            finish(false,
                                                   "STATION MANAGEMENT SMOKE TEST FAILED (create guard)");
                                        }
                                    });
                                if (!unchangedStarted) {
                                    finish(false,
                                           "STATION MANAGEMENT SMOKE TEST FAILED (unchanged request)");
                                }
                            });
                        if (invalidRequestId.isEmpty()) {
                            finish(false,
                                   "STATION MANAGEMENT SMOKE TEST FAILED (invalid request)");
                        }
                    });
                if (!detailStarted) {
                    finish(false, "STATION MANAGEMENT SMOKE TEST FAILED (detail request)");
                }
            });
        const bool duplicateListStarted = api.requestStations({});
        if (!listStarted || duplicateListStarted) {
            finish(false, "STATION MANAGEMENT SMOKE TEST FAILED (list guard)");
        }
    });

    QObject::connect(&api, &AdminApiClient::loginFailed, &application,
                     [](int, const QString&) {
        finish(false, "STATION MANAGEMENT SMOKE TEST FAILED (login)");
    });
    QTimer::singleShot(0, &application, [&api] {
        api.login(QStringLiteral("admin"), QStringLiteral("123456"));
    });
    QTimer::singleShot(20000, &application, [] {
        finish(false, "STATION MANAGEMENT SMOKE TEST FAILED (timeout)");
    });
    return application.exec();
}
