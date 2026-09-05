#include "ui/MainWindow.h"

#include "api/AdminApiClient.h"
#include "config/AppConfig.h"
#include "net/NetworkClient.h"
#include "protocol/Protocol.h"
#include "session/AdminSession.h"
#include "ui/ChargerManagementPage.h"
#include "ui/StationManagementPage.h"
#include "ui/UserManagementPage.h"
#include "ui/RevenueStatisticsPage.h"
#include "ui/OperationsOverviewPage.h"
#include "ui/theme/Theme.h"

#include <QApplication>
#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>

namespace {

const QStringList kPageTitles = {
    QStringLiteral("运营总览"),
    QStringLiteral("充电桩管理"),
    QStringLiteral("充电站管理"),
    QStringLiteral("用户管理"),
    QStringLiteral("营收统计"),
};

const QStringList kPageSubtitles = {
    QStringLiteral("全局关键指标与设备健康概览"),
    QStringLiteral("查询充电桩状态并执行受控运维操作"),
    QStringLiteral("维护站点资料并查看站内设备"),
    QStringLiteral("查询平台用户并管理账号状态"),
    QStringLiteral("核对今日、本月、累计营收与趋势"),
};

QLabel* makeLabel(const QString& text, const QString& objectName, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setObjectName(objectName);
    return label;
}

} // namespace

MainWindow::MainWindow(NetworkClient* network,
                       AdminSession* session,
                       AdminApiClient* api,
                       QWidget* parent)
    : QMainWindow(parent)
    , network_(network)
    , session_(session)
    , api_(api)
{
    Q_ASSERT(network_);
    Q_ASSERT(session_);
    Q_ASSERT(api_);
    setWindowTitle(QStringLiteral("EV 充电运营中心"));
    resize(1440, 900);
    setMinimumSize(1180, 720);

    auto* root = new QWidget(this);
    root->setObjectName(QStringLiteral("appRoot"));
    setCentralWidget(root);

    auto* rootLayout = new QHBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    rootLayout->addWidget(createSidebar());

    auto* workspace = new QWidget(root);
    auto* workspaceLayout = new QVBoxLayout(workspace);
    workspaceLayout->setContentsMargins(0, 0, 0, 0);
    workspaceLayout->setSpacing(0);
    workspaceLayout->addWidget(createTopbar());

    pages_ = new QStackedWidget(workspace);
    operationsOverviewPage_ = new OperationsOverviewPage(api_, pages_);
    pages_->addWidget(operationsOverviewPage_);
    chargerManagementPage_ = new ChargerManagementPage(api_, pages_);
    pages_->addWidget(chargerManagementPage_);
    stationManagementPage_ = new StationManagementPage(api_, pages_);
    pages_->addWidget(stationManagementPage_);
    userManagementPage_ = new UserManagementPage(api_, pages_);
    pages_->addWidget(userManagementPage_);
    revenueStatisticsPage_ = new RevenueStatisticsPage(api_, pages_);
    pages_->addWidget(revenueStatisticsPage_);
    workspaceLayout->addWidget(pages_, 1);
    rootLayout->addWidget(workspace, 1);

    connect(network_, &NetworkClient::connected, this, [this] {
        setConnectionState(QStringLiteral("checking"), QStringLiteral("正在验证服务"));
        sendPing();
    });
    connect(network_, &NetworkClient::disconnected, this, [this] {
        setConnectionState(QStringLiteral("offline"), QStringLiteral("服务未连接"));
    });
    connect(network_, &NetworkClient::transportError, this,
            [this](int, const QString&) {
                setConnectionState(QStringLiteral("offline"), QStringLiteral("服务未连接"));
            });
    connect(session_, &AdminSession::changed,
            this, &MainWindow::refreshAdminInfo);
    connect(session_, &AdminSession::authenticationChanged, this,
            [this](bool authenticated) {
        if (authenticated && operationsOverviewPage_) {
            operationsOverviewPage_->refresh();
        }
    });

    switchPage(0);
    auto* clockTimer = new QTimer(this);
    connect(clockTimer, &QTimer::timeout, this, &MainWindow::updateClock);
    clockTimer->start(1000);
    updateClock();
    QTimer::singleShot(0, this, &MainWindow::connectToServer);
}

QWidget* MainWindow::createSidebar()
{
    auto* sidebar = new QFrame(this);
    sidebar->setObjectName(QStringLiteral("sidebar"));
    sidebar->setFixedWidth(theme::kSidebarWidth);

    auto* layout = new QVBoxLayout(sidebar);
    layout->setContentsMargins(0, 24, 0, 20);
    layout->setSpacing(4);

    auto* brand = new QWidget(sidebar);
    auto* brandLayout = new QHBoxLayout(brand);
    brandLayout->setContentsMargins(20, 0, 16, 22);
    brandLayout->setSpacing(10);
    auto* mark = makeLabel(QStringLiteral("EV"), QStringLiteral("brandMark"), brand);
    auto* brandText = new QVBoxLayout;
    brandText->setSpacing(0);
    brandText->addWidget(makeLabel(QStringLiteral("充电运营中心"),
                                   QStringLiteral("brandTitle"), brand));
    brandText->addWidget(makeLabel(QStringLiteral("ADMIN CONSOLE"),
                                   QStringLiteral("eyebrow"), brand));
    brandLayout->addWidget(mark);
    brandLayout->addLayout(brandText);
    brandLayout->addStretch();
    layout->addWidget(brand);

    auto* groupLabel = makeLabel(QStringLiteral("运营工作台"), QStringLiteral("muted"), sidebar);
    groupLabel->setContentsMargins(20, 8, 0, 8);
    layout->addWidget(groupLabel);

    addNavigationButton(QStringLiteral("  运营总览"), 0);
    addNavigationButton(QStringLiteral("  充电桩管理"), 1);
    addNavigationButton(QStringLiteral("  充电站管理"), 2);
    addNavigationButton(QStringLiteral("  用户管理"), 3);
    addNavigationButton(QStringLiteral("  营收统计"), 4);
    for (auto* button : navigationButtons_) {
        layout->addWidget(button);
    }

    layout->addStretch();

    auto* divider = new QFrame(sidebar);
    divider->setFixedHeight(1);
    divider->setStyleSheet(QStringLiteral("background:#DCE5EE;"));
    layout->addWidget(divider);

    auto* admin = new QWidget(sidebar);
    auto* adminLayout = new QVBoxLayout(admin);
    adminLayout->setContentsMargins(20, 14, 20, 0);
    adminLayout->setSpacing(2);
    adminNameLabel_ = makeLabel(QStringLiteral("管理员"),
                                QStringLiteral("brandTitle"), admin);
    adminAccountLabel_ = makeLabel(QStringLiteral("未登录"),
                                   QStringLiteral("muted"), admin);
    auto* logoutButton = new QPushButton(QStringLiteral("退出登录"), admin);
    logoutButton->setObjectName(QStringLiteral("secondaryButton"));
    connect(logoutButton, &QPushButton::clicked,
            this, &MainWindow::logoutRequested);
    adminLayout->addWidget(adminNameLabel_);
    adminLayout->addWidget(adminAccountLabel_);
    adminLayout->addSpacing(8);
    adminLayout->addWidget(logoutButton);
    refreshAdminInfo();
    layout->addWidget(admin);

    return sidebar;
}

QWidget* MainWindow::createTopbar()
{
    auto* topbar = new QFrame(this);
    topbar->setObjectName(QStringLiteral("topbar"));
    topbar->setFixedHeight(82);

    auto* layout = new QHBoxLayout(topbar);
    layout->setContentsMargins(theme::kPageMargin, 14, theme::kPageMargin, 14);
    layout->setSpacing(12);

    auto* titleLayout = new QVBoxLayout;
    titleLayout->setSpacing(2);
    pageTitle_ = makeLabel({}, QStringLiteral("pageTitle"), topbar);
    pageSubtitle_ = makeLabel({}, QStringLiteral("pageSubtitle"), topbar);
    titleLayout->addWidget(pageTitle_);
    titleLayout->addWidget(pageSubtitle_);
    layout->addLayout(titleLayout);
    layout->addStretch();

    clockLabel_ = makeLabel({}, QStringLiteral("pageSubtitle"), topbar);
    clockLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    connectionBadge_ = makeLabel(QStringLiteral("服务未连接"),
                                 QStringLiteral("connectionBadge"), topbar);
    connectionBadge_->setProperty("connectionState", QStringLiteral("offline"));
    connectionButton_ = new QPushButton(QStringLiteral("重新连接"), topbar);
    connectionButton_->setObjectName(QStringLiteral("secondaryButton"));
    connect(connectionButton_, &QPushButton::clicked, this, &MainWindow::connectToServer);
    layout->addWidget(clockLabel_);
    layout->addSpacing(4);
    layout->addWidget(connectionBadge_);
    layout->addWidget(connectionButton_);
    return topbar;
}

void MainWindow::addNavigationButton(const QString& text, int pageIndex)
{
    auto* button = new QPushButton(text, this);
    button->setObjectName(QStringLiteral("navButton"));
    button->setCheckable(true);
    button->setAutoExclusive(true);
    connect(button, &QPushButton::clicked, this, [this, pageIndex] {
        switchPage(pageIndex);
    });
    navigationButtons_.append(button);
}

void MainWindow::switchPage(int pageIndex)
{
    if (!pages_ || pageIndex < 0 || pageIndex >= kPageTitles.size()) {
        return;
    }
    pages_->setCurrentIndex(pageIndex);
    pageTitle_->setText(kPageTitles.at(pageIndex));
    pageSubtitle_->setText(kPageSubtitles.at(pageIndex));
    navigationButtons_.at(pageIndex)->setChecked(true);
    if (pageIndex == 0 && session_->isAuthenticated() && operationsOverviewPage_) {
        operationsOverviewPage_->refresh();
    }
    if (pageIndex == 1 && session_->isAuthenticated() && chargerManagementPage_) {
        chargerManagementPage_->refresh();
    }
    if (pageIndex == 2 && session_->isAuthenticated() && stationManagementPage_) {
        stationManagementPage_->refresh();
    }
    if (pageIndex == 3 && session_->isAuthenticated() && userManagementPage_) {
        userManagementPage_->refresh();
    }
    if (pageIndex == 4 && session_->isAuthenticated() && revenueStatisticsPage_) {
        revenueStatisticsPage_->refresh();
    }
}

void MainWindow::connectToServer()
{
    setConnectionState(QStringLiteral("checking"), QStringLiteral("正在连接服务"));
    connectionButton_->setEnabled(false);
    network_->connectToServer(QString::fromUtf8(config::kDefaultServerHost),
                              config::kDefaultServerPort);
    QTimer::singleShot(1500, this, [this] {
        connectionButton_->setEnabled(true);
    });
}

void MainWindow::sendPing()
{
    QJsonObject data;
    data.insert(QStringLiteral("client"), QStringLiteral("admin-client"));
    network_->sendRequest(QString::fromUtf8(protocol::action::kPing), data,
                          [this](const protocol::Response& response) {
        if (response.isOk()) {
            setConnectionState(QStringLiteral("online"), QStringLiteral("服务运行正常"));
        } else {
            setConnectionState(QStringLiteral("offline"),
                               protocol::describeError(response.code, response.message));
        }
    });
}

void MainWindow::updateClock()
{
    if (!clockLabel_) {
        return;
    }
    clockLabel_->setText(QDateTime::currentDateTime().toString(
        QStringLiteral("yyyy-MM-dd  HH:mm:ss")));
}

void MainWindow::setConnectionState(const QString& state, const QString& text)
{
    connectionBadge_->setProperty("connectionState", state);
    connectionBadge_->setText(text);
    connectionBadge_->style()->unpolish(connectionBadge_);
    connectionBadge_->style()->polish(connectionBadge_);
    connectionBadge_->update();
    connectionButton_->setText(state == QStringLiteral("online")
                                   ? QStringLiteral("检测连接")
                                   : QStringLiteral("重新连接"));
}

void MainWindow::refreshAdminInfo()
{
    if (!adminNameLabel_ || !adminAccountLabel_) {
        return;
    }
    adminNameLabel_->setText(session_->isAuthenticated()
        ? session_->displayName()
        : QStringLiteral("管理员"));
    adminAccountLabel_->setText(session_->isAuthenticated()
        ? QStringLiteral("账号：%1").arg(session_->account())
        : QStringLiteral("未登录"));
}
