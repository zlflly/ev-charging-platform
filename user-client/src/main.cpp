#include "config/AppConfig.h"
#include "net/NetworkClient.h"
#include "session/Session.h"
#include "ui/ChargingPage.h"
#include "ui/HomePage.h"
#include "ui/LoginPage.h"
#include "ui/NavigationPage.h"
#include "ui/OrderPage.h"
#include "ui/ProfilePage.h"
#include "ui/SettlementPage.h"
#include "ui/StationDetailPage.h"
#include "ui/theme/Theme.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QPoint>
#include <QStackedWidget>
#include <QStringList>
#include <QTextStream>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <cstdio>

namespace {

// QtWebEngine 的 Chromium 网络栈不读取 QNetworkProxy，代理和软件渲染参数
// 必须在 QApplication 创建前写入环境变量。
void configureWebEngineRuntime()
{
    const QString proxyHost = QString::fromUtf8(appConfig::kHttpProxyHost);
    QStringList flags;
    if (!proxyHost.isEmpty()) {
        flags << QStringLiteral("--proxy-server=http://%1:%2")
                     .arg(proxyHost)
                     .arg(appConfig::kHttpProxyPort);
    }
    flags << QStringLiteral("--use-gl=swiftshader")
          << QStringLiteral("--enable-webgl")
          << QStringLiteral("--ignore-gpu-blocklist")
          << QStringLiteral("--no-sandbox");
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
            flags.join(QLatin1Char(' ')).toLocal8Bit());
}

// 从项目内被 gitignore 的 local.env 注入高德 key。源码不保存 key，
// 已存在的系统环境变量优先，方便本地和 CI 显式覆盖。
void loadLocalEnvironmentFile()
{
    QStringList candidateDirs;
    QDir directory(QCoreApplication::applicationDirPath());
    for (int level = 0; level < 4 && !directory.isRoot(); ++level) {
        candidateDirs << directory.absolutePath();
        if (!directory.cdUp()) {
            break;
        }
    }

    QString envFilePath;
    for (const QString& dirPath : candidateDirs) {
        const QString candidate = dirPath + QStringLiteral("/local.env");
        if (QFile::exists(candidate)) {
            envFilePath = candidate;
            break;
        }
    }
    if (envFilePath.isEmpty()) {
        return;
    }

    QFile envFile(envFilePath);
    if (!envFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QTextStream stream(&envFile);
    int injectedCount = 0;
    while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) {
            continue;
        }
        const int equalsIndex = line.indexOf(QLatin1Char('='));
        if (equalsIndex <= 0) {
            continue;
        }
        const QString name = line.left(equalsIndex).trimmed();
        const QString value = line.mid(equalsIndex + 1).trimmed();
        const QByteArray nameBytes = name.toLocal8Bit();
        if (name.isEmpty() || value.isEmpty() ||
            qEnvironmentVariableIsSet(nameBytes.constData())) {
            continue;
        }
        qputenv(nameBytes, value.toLocal8Bit());
        ++injectedCount;
    }
    if (injectedCount > 0) {
        std::fprintf(stdout, "local.env: loaded %d key(s)\n", injectedCount);
        std::fflush(stdout);
    }
}

} // namespace

int main(int argc, char* argv[])
{
    configureWebEngineRuntime();

    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("ev-user-client"));
    QApplication::setOrganizationName(QStringLiteral("ev-charging-platform"));
    loadLocalEnvironmentFile();

    QWidget appWindow;
    appWindow.setObjectName(QStringLiteral("AppWindow"));
    appWindow.setWindowTitle(QStringLiteral("东软充电"));
    QStackedWidget pageStack(&appWindow);
    auto* windowLayout = new QVBoxLayout(&appWindow);
    windowLayout->setContentsMargins(0, 0, 0, 0);
    windowLayout->addWidget(&pageStack);

    NetworkClient networkClient;
    LoginPage loginPage(&networkClient, &pageStack);
    HomePage homePage(&networkClient, &pageStack);
    StationDetailPage stationDetailPage(&networkClient, &pageStack);
    ChargingPage chargingPage(&networkClient, &pageStack);
    SettlementPage settlementPage(&networkClient, &pageStack);
    NavigationPage navigationPage(&networkClient, &pageStack);
    OrderPage orderPage(&networkClient, &pageStack);
    ProfilePage profilePage(&networkClient, &pageStack);
    pageStack.addWidget(&loginPage);
    pageStack.addWidget(&homePage);
    pageStack.addWidget(&stationDetailPage);
    pageStack.addWidget(&chargingPage);
    pageStack.addWidget(&settlementPage);
    pageStack.addWidget(&navigationPage);
    pageStack.addWidget(&orderPage);
    pageStack.addWidget(&profilePage);

    // 让外层窗口按当前手机画布切换高度，页面之间仍共享同一个顶层窗口。
    loginPage.setMinimumSize(0, 0);
    homePage.setMinimumSize(0, 0);
    stationDetailPage.setMinimumSize(0, 0);
    chargingPage.setMinimumSize(0, 0);
    settlementPage.setMinimumSize(0, 0);
    navigationPage.setMinimumSize(0, 0);
    orderPage.setMinimumSize(0, 0);
    profilePage.setMinimumSize(0, 0);

    appWindow.setMinimumSize(360, theme::detailCanvasHeight);

    auto showPage = [&](QWidget* page, int height) {
        const QPoint topLeft = appWindow.pos();
        pageStack.setCurrentWidget(page);
        appWindow.resize(theme::loginCanvasWidth, height);
        appWindow.move(topLeft);
        appWindow.show();
    };

    const bool detailPreview = qEnvironmentVariableIsSet("EV_DETAIL_PREVIEW")
        || !qgetenv("EV_DETAIL_SCREENSHOT_PATH").isEmpty();
    const bool homePreview = qEnvironmentVariableIsSet("EV_HOME_PREVIEW")
        || !qgetenv("EV_HOME_SCREENSHOT_PATH").isEmpty();
    const bool chargingPreview = qEnvironmentVariableIsSet("EV_CHARGING_PREVIEW")
        || !qgetenv("EV_CHARGING_SCREENSHOT_PATH").isEmpty();
    const bool settlementPreview = qEnvironmentVariableIsSet("EV_SETTLEMENT_PREVIEW")
        || !qgetenv("EV_SETTLEMENT_SCREENSHOT_PATH").isEmpty();
    const bool navigationPreview = qEnvironmentVariableIsSet("EV_NAVIGATION_PREVIEW")
        || qEnvironmentVariableIsSet("EV_NAVIGATION_ACTIVE_PREVIEW")
        || qEnvironmentVariableIsSet("EV_NAVIGATION_FINISHED_PREVIEW")
        || !qgetenv("EV_NAVIGATION_SCREENSHOT_PATH").isEmpty();

    if (navigationPreview) {
        Session::instance().setLocation(41.7087, 123.4312,
                                        QStringLiteral("沈阳市和平区三好街"));
        navigationPage.openRoute(41.7195, 123.4312,
                                 QStringLiteral("东软软件园充电站"), false);
        showPage(&navigationPage, theme::loginCanvasHeight);
    } else if (settlementPreview) {
        OrderInfo order;
        order.orderId = 202609030001;
        order.status = QStringLiteral("WAIT_SETTLEMENT");
        order.chargerId = 1;
        order.chargerCode = QStringLiteral("A-001");
        order.chargerType = protocol::ChargerTypeFast;
        order.stationName = QStringLiteral("东软软件园充电站");
        order.powerKw = 42.0;
        order.energyKwh = 12.35;
        order.estimatedAmount = 18.52;
        order.amount = 18.52;
        order.startTimeMs = QDateTime::currentMSecsSinceEpoch() - 18 * 60 * 1000;
        order.stopTimeMs = QDateTime::currentMSecsSinceEpoch();
        settlementPage.openOrder(order);
        showPage(&settlementPage, theme::loginCanvasHeight);
    } else if (chargingPreview) {
        ChargerInfo charger;
        charger.chargerId = 1;
        charger.code = QStringLiteral("A-001");
        charger.type = protocol::ChargerTypeFast;
        charger.status = protocol::ChargerStatusIdle;
        charger.powerKw = 120.0;
        chargingPage.openWithCharger(charger, QStringLiteral("东软软件园充电站"), 1.25);
        showPage(&chargingPage, theme::loginCanvasHeight);
    } else if (detailPreview) {
        stationDetailPage.openStation(1);
        showPage(&stationDetailPage, theme::detailCanvasHeight);
    } else if (homePreview) {
        showPage(&homePage, theme::loginCanvasHeight);
    } else {
        showPage(&loginPage, theme::loginCanvasHeight);
    }

    QObject::connect(&loginPage, &LoginPage::loginSucceeded, [&] {
        showPage(&homePage, theme::loginCanvasHeight);
    });
    QObject::connect(&loginPage, &LoginPage::sessionRestored, [&] {
        if (pageStack.currentWidget() == &loginPage) {
            showPage(&homePage, theme::loginCanvasHeight);
        } else if (pageStack.currentWidget() == &chargingPage) {
            chargingPage.openEmpty();
        } else if (pageStack.currentWidget() == &orderPage) {
            orderPage.reload();
        } else if (pageStack.currentWidget() == &profilePage) {
            profilePage.reload();
        } else {
            homePage.refreshStations();
        }
    });

    QObject::connect(&homePage, &HomePage::stationSelected,
                     [&](qint64 stationId) {
        stationDetailPage.openStation(stationId);
        showPage(&stationDetailPage, theme::detailCanvasHeight);
    });
    QObject::connect(&homePage, &HomePage::stationsRequested,
                     [&] { homePage.refreshStations(); });
    QObject::connect(&homePage, &HomePage::chargingRequested,
                     [&] {
        chargingPage.openEmpty();
        showPage(&chargingPage, theme::loginCanvasHeight);
    });
    QObject::connect(&homePage, &HomePage::ordersRequested,
                     [&] {
        orderPage.reload();
        showPage(&orderPage, theme::loginCanvasHeight);
    });
    QObject::connect(&homePage, &HomePage::profileRequested,
                     [&] {
        profilePage.reload();
        showPage(&profilePage, theme::loginCanvasHeight);
    });
    QObject::connect(&stationDetailPage, &StationDetailPage::chargerSelected,
                     [&](const ChargerInfo& charger, const QString& stationName,
                         double pricePerKwh) {
        chargingPage.openWithCharger(charger, stationName, pricePerKwh);
        showPage(&chargingPage, theme::loginCanvasHeight);
    });
    QObject::connect(&stationDetailPage, &StationDetailPage::backRequested,
                     [&] {
        showPage(&homePage, theme::loginCanvasHeight);
    });
    QObject::connect(&stationDetailPage, &StationDetailPage::navigationRequested,
                     [&](double latitude, double longitude,
                         const QString& destinationName, bool walking) {
        navigationPage.openRoute(latitude, longitude, destinationName, walking);
        showPage(&navigationPage, theme::loginCanvasHeight);
    });
    QObject::connect(&chargingPage, &ChargingPage::backRequested,
                     [&] {
        stationDetailPage.refresh();
        showPage(&stationDetailPage, theme::detailCanvasHeight);
    });
    QObject::connect(&chargingPage, &ChargingPage::settlementRequested,
                     [&](const OrderInfo& order) {
        settlementPage.openOrder(order);
        showPage(&settlementPage, theme::loginCanvasHeight);
    });
    QObject::connect(&settlementPage, &SettlementPage::backRequested,
                     [&] {
        showPage(&chargingPage, theme::loginCanvasHeight);
    });
    QObject::connect(&settlementPage, &SettlementPage::homeRequested,
                     [&] {
        showPage(&homePage, theme::loginCanvasHeight);
    });
    QObject::connect(&profilePage, &ProfilePage::backRequested,
                     [&] { showPage(&homePage, theme::loginCanvasHeight); });
    QObject::connect(&profilePage, &ProfilePage::homeRequested,
                     [&] { showPage(&homePage, theme::loginCanvasHeight); });
    QObject::connect(&profilePage, &ProfilePage::stationsRequested,
                     [&] {
        homePage.refreshStations();
        showPage(&homePage, theme::loginCanvasHeight);
    });
    QObject::connect(&profilePage, &ProfilePage::chargingRequested,
                     [&] {
        chargingPage.openEmpty();
        showPage(&chargingPage, theme::loginCanvasHeight);
    });
    QObject::connect(&profilePage, &ProfilePage::ordersRequested,
                     [&] {
        orderPage.reload();
        showPage(&orderPage, theme::loginCanvasHeight);
    });
    QObject::connect(&profilePage, &ProfilePage::logoutRequested,
                     [&] {
        loginPage.clearRememberedLogin();
        Session::instance().logout();
        showPage(&loginPage, theme::loginCanvasHeight);
    });
    QObject::connect(&orderPage, &OrderPage::backRequested,
                     [&] { showPage(&homePage, theme::loginCanvasHeight); });
    QObject::connect(&orderPage, &OrderPage::continueOrderRequested,
                     [&](const OrderInfo& order) {
        if (order.statusEnum() == OrderInfo::StatusWaitSettlement) {
            settlementPage.openOrder(order);
            showPage(&settlementPage, theme::loginCanvasHeight);
            return;
        }
        ChargerInfo charger;
        charger.chargerId = order.chargerId;
        charger.code = order.chargerCode;
        charger.type = order.chargerType;
        charger.status = protocol::ChargerStatusIdle;
        charger.powerKw = order.powerKw;
        chargingPage.openWithCharger(charger, order.stationName, order.pricePerKwh);
        showPage(&chargingPage, theme::loginCanvasHeight);
    });
    QObject::connect(&orderPage, &OrderPage::navigateAgainRequested,
                     [&](const OrderInfo& order) {
        if ((order.latitude != 0.0 || order.longitude != 0.0) &&
            Session::instance().hasLocation()) {
            navigationPage.openRoute(order.latitude, order.longitude,
                                     order.stationName, false);
            showPage(&navigationPage, theme::loginCanvasHeight);
        } else if (order.stationId > 0) {
            stationDetailPage.openStation(order.stationId);
            showPage(&stationDetailPage, theme::detailCanvasHeight);
        }
    });
    QObject::connect(&navigationPage, &NavigationPage::backRequested,
                     [&] {
        showPage(&stationDetailPage, theme::detailCanvasHeight);
    });
    QObject::connect(&navigationPage, &NavigationPage::backToStationRequested,
                     [&] {
        showPage(&stationDetailPage, theme::detailCanvasHeight);
    });

    const QByteArray screenshotPath = navigationPreview
        ? qgetenv("EV_NAVIGATION_SCREENSHOT_PATH")
        : settlementPreview
        ? qgetenv("EV_SETTLEMENT_SCREENSHOT_PATH")
        : chargingPreview
        ? qgetenv("EV_CHARGING_SCREENSHOT_PATH")
        : detailPreview
        ? qgetenv("EV_DETAIL_SCREENSHOT_PATH")
        : homePreview
        ? qgetenv("EV_HOME_SCREENSHOT_PATH")
        : qgetenv("EV_LOGIN_SCREENSHOT_PATH");
    if (!screenshotPath.isEmpty()) {
        QWidget* screenshotWidget = navigationPreview
            ? static_cast<QWidget*>(&navigationPage)
            : settlementPreview
            ? static_cast<QWidget*>(&settlementPage)
            : chargingPreview
            ? static_cast<QWidget*>(&chargingPage)
            : detailPreview
            ? static_cast<QWidget*>(&stationDetailPage)
            : homePreview
            ? static_cast<QWidget*>(&homePage)
            : static_cast<QWidget*>(&loginPage);
        const int screenshotDelay = navigationPreview ? 2500 : 500;
        QTimer::singleShot(screenshotDelay, [screenshotWidget, screenshotPath, &app] {
            screenshotWidget->grab().save(QString::fromLocal8Bit(screenshotPath));
            app.quit();
        });
    }

    // 只在没有显式预览页面时恢复登录，避免预览截图被 QSettings 中的账号抢走。
    const bool anyPreview = detailPreview || homePreview || chargingPreview ||
                            settlementPreview || navigationPreview;
    if (!anyPreview) {
        QTimer::singleShot(0, &loginPage, &LoginPage::tryRestoreLogin);
    }

    return app.exec();
}
