#include "api/AdminApiClient.h"
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
}

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    NetworkClient network;
    AdminSession session;
    AdminApiClient api(&network, &session);

    UserListQuery invalidQuery;
    invalidQuery.phoneKeyword = QStringLiteral("13x");
    QString validationError;
    if (invalidQuery.validate(&validationError)) {
        std::printf("USER MANAGEMENT SMOKE TEST FAILED (invalid search)\n");
        return 1;
    }

    QObject::connect(&api, &AdminApiClient::loginSucceeded, &application, [&] {
        UserListQuery all;
        all.page = 1;
        all.pageSize = 2;
        all.activityFilter = QStringLiteral("ACTIVE");
        const bool allStarted = api.requestUsers(
            all, [&api](std::optional<UserListPage> page, const QString&) {
                bool chargingUserVisible = false;
                bool waitingUserVisible = false;
                if (page) {
                    for (const AdminUser& user : page->users) {
                        if (user.userId == 3 && user.activeOrder
                            && user.activeOrder->orderId == 9003
                            && user.activeOrder->chargerCode == QStringLiteral("CP-007")
                            && user.activeOrder->stationName == QStringLiteral("中关村科技园站")
                            && user.activityLabel() == QStringLiteral("充电中")) {
                            chargingUserVisible = true;
                        }
                        if (user.userId == 5 && user.activeOrder
                            && user.activeOrder->status
                                == QString::fromUtf8(protocol::orderStatus::kWaitSettlement)
                            && user.activityLabel() == QStringLiteral("待支付")) {
                            waitingUserVisible = true;
                        }
                    }
                }
                if (!page || page->users.size() != 2 || page->total != 2
                    || page->page != 1 || page->pageSize != 2) {
                    finish(false, "USER MANAGEMENT SMOKE TEST FAILED (activity filter)");
                    return;
                }
                if (!chargingUserVisible) {
                    finish(false, "USER MANAGEMENT SMOKE TEST FAILED (active order detail)");
                    return;
                }
                if (!waitingUserVisible) {
                    finish(false, "USER MANAGEMENT SMOKE TEST FAILED (wait settlement label)");
                    return;
                }
                UserListQuery search;
                search.page = 1;
                search.pageSize = 20;
                search.phoneKeyword = QStringLiteral("38002");
                const bool searchStarted = api.requestUsers(
                    search, [&api, search](std::optional<UserListPage> found,
                                          const QString&) {
                        if (!found || found->total != 1 || found->users.size() != 1
                            || found->users.first().userId != 2
                            || found->users.first().formattedBalance()
                                != QStringLiteral("¥ 100.50")) {
                            finish(false, "USER MANAGEMENT SMOKE TEST FAILED (server search)");
                            return;
                        }
                        UserStatusUpdateRequest freeze;
                        freeze.userId = 2;
                        freeze.expectedStatus = protocol::UserStatusNormal;
                        freeze.targetStatus = protocol::UserStatusFrozen;
                        freeze.reason = QStringLiteral("冒烟测试风险冻结");
                        const bool freezeStarted = api.updateUserStatus(
                            freeze, [&api, search, freeze](
                                std::optional<UserStatusUpdateResult> result,
                                const QString&) {
                                if (!result || result->status != protocol::UserStatusFrozen) {
                                    finish(false, "USER MANAGEMENT SMOKE TEST FAILED (freeze)");
                                    return;
                                }
                                const bool reloadStarted = api.requestUsers(
                                    search, [&api, search, freeze](
                                        std::optional<UserListPage> reloaded,
                                        const QString&) {
                                        if (!reloaded || reloaded->users.first().status
                                            != protocol::UserStatusFrozen) {
                                            finish(false, "USER MANAGEMENT SMOKE TEST FAILED (reload)");
                                            return;
                                        }
                                        UserStatusUpdateRequest stale = freeze;
                                        stale.reason = QStringLiteral("旧状态重复冻结");
                                        const bool staleStarted = api.updateUserStatus(
                                            stale, [&api, search](
                                                std::optional<UserStatusUpdateResult> staleResult,
                                                const QString&) {
                                                if (staleResult) {
                                                    finish(false, "USER MANAGEMENT SMOKE TEST FAILED (stale accepted)");
                                                    return;
                                                }
                                                UserStatusUpdateRequest unfreeze;
                                                unfreeze.userId = 2;
                                                unfreeze.expectedStatus = protocol::UserStatusFrozen;
                                                unfreeze.targetStatus = protocol::UserStatusNormal;
                                                unfreeze.reason = QStringLiteral("冒烟测试解除冻结");
                                                const bool unfreezeStarted = api.updateUserStatus(
                                                    unfreeze, [&api](
                                                        std::optional<UserStatusUpdateResult> unfrozen,
                                                        const QString&) {
                                                        if (!unfrozen || unfrozen->status
                                                            != protocol::UserStatusNormal) {
                                                            finish(false, "USER MANAGEMENT SMOKE TEST FAILED (unfreeze)");
                                                            return;
                                                        }
                                                        UserStatusUpdateRequest active;
                                                        active.userId = 3;
                                                        active.expectedStatus = protocol::UserStatusNormal;
                                                        active.targetStatus = protocol::UserStatusFrozen;
                                                        active.reason = QStringLiteral("验证服务端活跃订单规则");
                                                        const bool activeStarted = api.updateUserStatus(
                                                            active, [&api](
                                                                std::optional<UserStatusUpdateResult> activeResult,
                                                                const QString& message) {
                                                                if (activeResult || !message.contains(QStringLiteral("未完成订单"))) {
                                                                    finish(false, "USER MANAGEMENT SMOKE TEST FAILED (server rule)");
                                                                    return;
                                                                }
                                                                UserStatusUpdateRequest waiting;
                                                                waiting.userId = 5;
                                                                waiting.expectedStatus = protocol::UserStatusNormal;
                                                                waiting.targetStatus = protocol::UserStatusFrozen;
                                                                waiting.reason = QStringLiteral("验证待支付订单保护");
                                                                const bool waitingStarted = api.updateUserStatus(
                                                                    waiting, [&api](
                                                                        std::optional<UserStatusUpdateResult> waitingResult,
                                                                        const QString& waitingMessage) {
                                                                        if (waitingResult || !waitingMessage.contains(QStringLiteral("未完成订单"))) {
                                                                            finish(false, "USER MANAGEMENT SMOKE TEST FAILED (wait settlement freeze guard)");
                                                                            return;
                                                                        }
                                                                        UserListQuery none;
                                                                        none.phoneKeyword = QStringLiteral("99999999999");
                                                                        const bool noneStarted = api.requestUsers(
                                                                            none, [](std::optional<UserListPage> empty,
                                                                                     const QString&) {
                                                                                const bool ok = empty && empty->total == 0
                                                                                    && empty->users.isEmpty();
                                                                                finish(ok, ok
                                                                                    ? "USER MANAGEMENT SMOKE TEST PASSED"
                                                                                    : "USER MANAGEMENT SMOKE TEST FAILED (empty)");
                                                                            });
                                                                        if (!noneStarted) finish(false, "USER MANAGEMENT SMOKE TEST FAILED (empty start)");
                                                                    });
                                                                if (!waitingStarted) finish(false, "USER MANAGEMENT SMOKE TEST FAILED (wait settlement freeze guard start)");
                                                            });
                                                        if (!activeStarted) finish(false, "USER MANAGEMENT SMOKE TEST FAILED (server rule start)");
                                                    });
                                                if (!unfreezeStarted) finish(false, "USER MANAGEMENT SMOKE TEST FAILED (unfreeze start)");
                                            });
                                        if (!staleStarted) finish(false, "USER MANAGEMENT SMOKE TEST FAILED (stale start)");
                                    });
                                if (!reloadStarted) finish(false, "USER MANAGEMENT SMOKE TEST FAILED (reload start)");
                            });
                        if (!freezeStarted) finish(false, "USER MANAGEMENT SMOKE TEST FAILED (freeze start)");
                    });
                if (!searchStarted) finish(false, "USER MANAGEMENT SMOKE TEST FAILED (search start)");
            });
        if (!allStarted) finish(false, "USER MANAGEMENT SMOKE TEST FAILED (pagination start)");
    });
    QObject::connect(&api, &AdminApiClient::loginFailed, &application,
                     [](int, const QString&) { finish(false, "USER MANAGEMENT SMOKE TEST FAILED (login)"); });

    QTimer::singleShot(0, &application, [&api] { api.login(QStringLiteral("admin"), QStringLiteral("123456")); });
    QTimer::singleShot(15000, &application,
                       [] { finish(false, "USER MANAGEMENT SMOKE TEST FAILED (timeout)"); });
    return application.exec();
}
