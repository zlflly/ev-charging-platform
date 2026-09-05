#include "api/AdminApiClient.h"
#include "model/Charger.h"
#include "model/Revenue.h"
#include "model/Station.h"
#include "model/User.h"
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
    int completedSources = 0;
    const auto sourcePassed = [&completedSources] {
        if (++completedSources == 6) {
            finish(true, "OPERATIONS OVERVIEW SMOKE TEST PASSED");
        }
    };

    QObject::connect(&api, &AdminApiClient::loginSucceeded, &application, [&] {
        const bool summaryStarted = api.requestRevenueSummary(
            [sourcePassed](std::optional<RevenueSummary> summary, const QString&) {
                if (!summary) {
                    finish(false, "OPERATIONS OVERVIEW SMOKE TEST FAILED (revenue summary)");
                    return;
                }
                sourcePassed();
            });
        const bool trendStarted = api.requestRevenueTrend(
            7, [sourcePassed](std::optional<RevenueTrend> trend, const QString&) {
                if (!trend || trend->points.size() != 7) {
                    finish(false, "OPERATIONS OVERVIEW SMOKE TEST FAILED (revenue trend)");
                    return;
                }
                sourcePassed();
            });
        const bool overviewStarted = api.requestChargerOverview(
            [sourcePassed](std::optional<ChargerStatusOverview> overview, const QString&) {
                if (!overview || overview->total < 0) {
                    finish(false, "OPERATIONS OVERVIEW SMOKE TEST FAILED (charger overview)");
                    return;
                }
                sourcePassed();
            });
        const bool stationsStarted = api.requestStations(
            [sourcePassed](std::optional<QList<Station>> stations, const QString&) {
                if (!stations || stations->isEmpty()) {
                    finish(false, "OPERATIONS OVERVIEW SMOKE TEST FAILED (stations)");
                    return;
                }
                sourcePassed();
            });
        const bool chargersStarted = api.requestChargers(
            [sourcePassed](std::optional<QList<Charger>> chargers, const QString&) {
                if (!chargers) {
                    finish(false, "OPERATIONS OVERVIEW SMOKE TEST FAILED (chargers)");
                    return;
                }
                bool hasFault = false;
                for (const Charger& charger : *chargers) {
                    hasFault = hasFault || charger.status == protocol::ChargerStatusFault;
                }
                if (!hasFault) {
                    finish(false, "OPERATIONS OVERVIEW SMOKE TEST FAILED (fault focus)");
                    return;
                }
                sourcePassed();
            });

        UserListQuery allUsers;
        allUsers.page = 1;
        allUsers.pageSize = 1;
        allUsers.activityFilter = QStringLiteral("ALL");
        const bool usersStarted = api.requestUsers(
            allUsers, [&api, sourcePassed](std::optional<UserListPage> all,
                                           const QString&) {
                if (!all || all->total < all->users.size()) {
                    finish(false, "OPERATIONS OVERVIEW SMOKE TEST FAILED (user total)");
                    return;
                }
                UserListQuery activeUsers;
                activeUsers.page = 1;
                activeUsers.pageSize = 3;
                activeUsers.activityFilter = QStringLiteral("ACTIVE");
                const bool activeStarted = api.requestUsers(
                    activeUsers, [sourcePassed](std::optional<UserListPage> active,
                                                const QString&) {
                        if (!active || active->total < active->users.size()) {
                            finish(false, "OPERATIONS OVERVIEW SMOKE TEST FAILED (active users)");
                            return;
                        }
                        for (const AdminUser& user : active->users) {
                            if (!user.activeOrder || user.currentDeviceLabel().isEmpty()) {
                                finish(false, "OPERATIONS OVERVIEW SMOKE TEST FAILED (active detail)");
                                return;
                            }
                        }
                        sourcePassed();
                    });
                if (!activeStarted) {
                    finish(false, "OPERATIONS OVERVIEW SMOKE TEST FAILED (active request guard)");
                }
            });

        if (!summaryStarted || !trendStarted || !overviewStarted
            || !stationsStarted || !chargersStarted || !usersStarted) {
            finish(false, "OPERATIONS OVERVIEW SMOKE TEST FAILED (request start)");
        }
    });
    QObject::connect(&api, &AdminApiClient::loginFailed, &application,
                     [](int, const QString&) {
        finish(false, "OPERATIONS OVERVIEW SMOKE TEST FAILED (login)");
    });
    QTimer::singleShot(0, &application, [&api] {
        api.login(QStringLiteral("admin"), QStringLiteral("123456"));
    });
    QTimer::singleShot(15000, &application, [] {
        finish(false, "OPERATIONS OVERVIEW SMOKE TEST FAILED (timeout)");
    });
    return application.exec();
}
