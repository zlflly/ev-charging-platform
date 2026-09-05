#include "api/AdminApiClient.h"
#include "model/Order.h"
#include "net/NetworkClient.h"
#include "protocol/Protocol.h"
#include "session/AdminSession.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QTimer>

#include <cstdio>

namespace {

void finish(bool ok, const char* message)
{
    std::printf("%s\n", message);
    QCoreApplication::exit(ok ? 0 : 1);
}

bool verifyCrossFieldValidation()
{
    QJsonObject order {
        {QStringLiteral("orderId"), 1}, {QStringLiteral("userId"), 2},
        {QStringLiteral("phone"), QStringLiteral("13800138000")},
        {QStringLiteral("nickname"), QStringLiteral("测试用户")},
        {QStringLiteral("status"), QStringLiteral("WAIT_SETTLEMENT")},
        {QStringLiteral("paymentStatus"), QStringLiteral("UNPAID")},
        {QStringLiteral("amountKind"), QStringLiteral("FINAL")},
        {QStringLiteral("stationId"), 3},
        {QStringLiteral("stationName"), QStringLiteral("测试站")},
        {QStringLiteral("chargerId"), 4},
        {QStringLiteral("chargerCode"), QStringLiteral("CP-004")},
        {QStringLiteral("type"), protocol::ChargerTypeFast},
        {QStringLiteral("powerKw"), 120.0}, {QStringLiteral("pricePerKwh"), 1.5},
        {QStringLiteral("energyKwh"), 10.0}, {QStringLiteral("amount"), 15.0},
        {QStringLiteral("createdAt"), 1788500000000.0},
        {QStringLiteral("startTime"), 1788500100000.0},
        {QStringLiteral("stopTime"), 1788503700000.0},
        {QStringLiteral("settleTime"), 0}, {QStringLiteral("durationSeconds"), 3600},
    };
    AdminOrder parsed;
    QString error;
    if (!AdminOrder::fromJson(order, &parsed, &error)
        || parsed.paymentStatusLabel() != QStringLiteral("待支付")) return false;
    order.insert(QStringLiteral("paymentStatus"), QStringLiteral("PAID"));
    return !AdminOrder::fromJson(order, &parsed, &error);
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    if (!verifyCrossFieldValidation()) {
        std::printf("ORDER MANAGEMENT SMOKE TEST FAILED (payload validation)\n");
        return 1;
    }
    NetworkClient network;
    AdminSession session;
    AdminApiClient api(&network, &session);

    QObject::connect(&api, &AdminApiClient::loginSucceeded, &application, [&] {
        OrderListQuery first;
        first.pageSize = 2;
        const bool started = api.requestOrders(
            first, [&api](std::optional<OrderListPage> page, const QString&) {
                if (!page || page->orders.size() != 2 || page->total != 6
                    || page->platformSummary.totalOrders != 6
                    || page->platformSummary.chargingCount != 1
                    || page->platformSummary.waitSettlementCount != 1
                    || page->orders.at(1).amountKind != QStringLiteral("ESTIMATED")) {
                    finish(false, "ORDER MANAGEMENT SMOKE TEST FAILED (platform page)");
                    return;
                }
                OrderListQuery waiting;
                waiting.status = QStringLiteral("WAIT_SETTLEMENT");
                waiting.paymentStatus = QStringLiteral("UNPAID");
                const bool waitingStarted = api.requestOrders(
                    waiting, [&api](std::optional<OrderListPage> result, const QString&) {
                        if (!result || result->total != 1 || result->orders.size() != 1
                            || result->orders.first().orderId != 9008
                            || result->orders.first().paymentStatusLabel()
                                != QStringLiteral("待支付")) {
                            finish(false, "ORDER MANAGEMENT SMOKE TEST FAILED (status filters)");
                            return;
                        }
                        OrderListQuery device;
                        device.keyword = QStringLiteral("CP-007");
                        const bool deviceStarted = api.requestOrders(
                            device, [&api](std::optional<OrderListPage> found, const QString&) {
                                if (!found || found->total != 1 || found->orders.first().userId != 3
                                    || found->orders.first().stationName
                                        != QStringLiteral("中关村科技园站")) {
                                    finish(false, "ORDER MANAGEMENT SMOKE TEST FAILED (joined search)");
                                    return;
                                }
                                OrderListQuery none;
                                none.keyword = QStringLiteral("不存在的订单");
                                const bool noneStarted = api.requestOrders(
                                    none, [](std::optional<OrderListPage> empty, const QString&) {
                                        const bool ok = empty && empty->total == 0
                                            && empty->orders.isEmpty();
                                        finish(ok, ok ? "ORDER MANAGEMENT SMOKE TEST PASSED"
                                                      : "ORDER MANAGEMENT SMOKE TEST FAILED (empty)");
                                    });
                                if (!noneStarted) finish(false, "ORDER MANAGEMENT SMOKE TEST FAILED (empty start)");
                            });
                        if (!deviceStarted) finish(false, "ORDER MANAGEMENT SMOKE TEST FAILED (search start)");
                    });
                if (!waitingStarted) finish(false, "ORDER MANAGEMENT SMOKE TEST FAILED (filter start)");
            });
        const bool duplicate = api.requestOrders(first, {});
        if (!started || duplicate) {
            finish(false, "ORDER MANAGEMENT SMOKE TEST FAILED (request guard)");
        }
    });
    QObject::connect(&api, &AdminApiClient::loginFailed, &application,
                     [](int, const QString&) { finish(false, "ORDER MANAGEMENT SMOKE TEST FAILED (login)"); });
    QTimer::singleShot(0, &application, [&api] {
        api.login(QStringLiteral("admin"), QStringLiteral("123456"));
    });
    QTimer::singleShot(15000, &application, [] {
        finish(false, "ORDER MANAGEMENT SMOKE TEST FAILED (timeout)");
    });
    return application.exec();
}
