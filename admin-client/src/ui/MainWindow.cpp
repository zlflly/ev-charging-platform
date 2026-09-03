#include "ui/MainWindow.h"

#include "config/AppConfig.h"
#include "net/NetworkClient.h"
#include "protocol/Protocol.h"
#include "ui/theme/Theme.h"
#include "ui/widgets/EntityTableView.h"

#include <QApplication>
#include <QDateTime>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
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

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , network_(new NetworkClient(this))
{
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
    pages_->addWidget(createOverviewPage());
    pages_->addWidget(createChargerPage());
    pages_->addWidget(createStationPage());
    pages_->addWidget(createUserPage());
    pages_->addWidget(createRevenuePage());
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
    divider->setStyleSheet(QStringLiteral("background:#18344E;"));
    layout->addWidget(divider);

    auto* admin = new QWidget(sidebar);
    auto* adminLayout = new QVBoxLayout(admin);
    adminLayout->setContentsMargins(20, 14, 20, 0);
    adminLayout->setSpacing(2);
    adminLayout->addWidget(makeLabel(QStringLiteral("管理员会话"),
                                     QStringLiteral("brandTitle"), admin));
    adminLayout->addWidget(makeLabel(QStringLiteral("等待 Commit 1 接入认证"),
                                     QStringLiteral("muted"), admin));
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

QWidget* MainWindow::createOverviewPage()
{
    auto* page = new QWidget(this);
    page->setObjectName(QStringLiteral("pageRoot"));
    auto* layout = new QGridLayout(page);
    layout->setContentsMargins(theme::kPageMargin, 20,
                               theme::kPageMargin, theme::kPageMargin);
    layout->setHorizontalSpacing(14);
    layout->setVerticalSpacing(14);

    layout->addWidget(createMetricCard(QStringLiteral("今日营收"), QStringLiteral("¥ --"),
                                       QStringLiteral("等待统计接口"), QStringLiteral("#38E6A5")), 0, 0);
    layout->addWidget(createMetricCard(QStringLiteral("本月营收"), QStringLiteral("¥ --"),
                                       QStringLiteral("等待统计接口"), QStringLiteral("#48C8FF")), 0, 1);
    layout->addWidget(createMetricCard(QStringLiteral("累计营收"), QStringLiteral("¥ --"),
                                       QStringLiteral("等待统计接口"), QStringLiteral("#9C8CFF")), 0, 2);
    layout->addWidget(createMetricCard(QStringLiteral("运营站点"), QStringLiteral("--"),
                                       QStringLiteral("等待站点接口"), QStringLiteral("#FFC04D")), 0, 3);
    layout->addWidget(createMetricCard(QStringLiteral("充电桩总数"), QStringLiteral("--"),
                                       QStringLiteral("等待设备接口"), QStringLiteral("#2AD4D9")), 0, 4);
    layout->addWidget(createMetricCard(QStringLiteral("平台用户"), QStringLiteral("--"),
                                       QStringLiteral("等待用户接口"), QStringLiteral("#FF8DA1")), 0, 5);

    auto* trend = createPlaceholderPanel(
        QStringLiteral("营收趋势 · 最近 7 日"),
        QStringLiteral("QChart 数据视图将在营收统计节点接入\n当前不展示任何模拟收入"),
        QStringLiteral("#48C8FF"));
    layout->addWidget(createSectionCard(QStringLiteral("销售业绩"), trend), 1, 0, 1, 4);

    auto* health = new QWidget(page);
    auto* healthLayout = new QHBoxLayout(health);
    healthLayout->setContentsMargins(8, 8, 8, 8);
    healthLayout->setSpacing(18);

    auto* ring = new QFrame(health);
    ring->setFixedSize(126, 126);
    ring->setStyleSheet(QStringLiteral(
        "QFrame { background:#091725; border:12px solid #24425E; border-radius:63px; }"));
    auto* ringLayout = new QVBoxLayout(ring);
    ringLayout->setContentsMargins(10, 10, 10, 10);
    auto* ringValue = makeLabel(QStringLiteral("--"), QStringLiteral("metricValue"), ring);
    ringValue->setAlignment(Qt::AlignCenter);
    auto* ringHint = makeLabel(QStringLiteral("总设备"), QStringLiteral("metricHint"), ring);
    ringHint->setAlignment(Qt::AlignCenter);
    ringLayout->addStretch();
    ringLayout->addWidget(ringValue);
    ringLayout->addWidget(ringHint);
    ringLayout->addStretch();
    healthLayout->addWidget(ring, 0, Qt::AlignCenter);

    auto* legendLayout = new QVBoxLayout;
    legendLayout->setSpacing(10);
    legendLayout->addWidget(makeLabel(QStringLiteral("状态分布"),
                                      QStringLiteral("sectionTitle"), health));
    const QStringList states = {
        QStringLiteral("在用    --"),
        QStringLiteral("闲置    --"),
        QStringLiteral("故障    --"),
    };
    const QStringList colors = {
        QStringLiteral("#48C8FF"),
        QStringLiteral("#38E6A5"),
        QStringLiteral("#FF6268"),
    };
    for (int index = 0; index < states.size(); ++index) {
        auto* row = new QHBoxLayout;
        auto* dot = new QLabel(QStringLiteral("●"), health);
        dot->setStyleSheet(QStringLiteral("color:%1; font-size:13px;").arg(colors.at(index)));
        row->addWidget(dot);
        row->addWidget(makeLabel(states.at(index), QStringLiteral("pageSubtitle"), health));
        row->addStretch();
        legendLayout->addLayout(row);
    }
    legendLayout->addStretch();
    healthLayout->addLayout(legendLayout, 1);
    layout->addWidget(createSectionCard(QStringLiteral("充电桩健康度"), health), 1, 4, 1, 2);

    auto* stationRanking = createPlaceholderPanel(
        QStringLiteral("等待站点规模数据"),
        QStringLiteral("这里将用柱状图比较各站点充电桩数量\n排名数据来自统一站点接口"),
        QStringLiteral("#38E6A5"));
    layout->addWidget(createSectionCard(QStringLiteral("各站点设备规模排行"), stationRanking),
                      2, 0, 1, 4);

    auto* faultTable = new EntityTableView(
        {QStringLiteral("电桩编号"), QStringLiteral("所属站点"), QStringLiteral("状态")}, page);
    faultTable->setState(EntityTableView::State::Empty,
                         QStringLiteral("故障设备将在此集中展示"));
    layout->addWidget(createSectionCard(QStringLiteral("故障设备"), faultTable), 2, 4, 1, 2);

    for (int column = 0; column < 6; ++column) {
        layout->setColumnStretch(column, 1);
    }
    layout->setRowStretch(1, 3);
    layout->setRowStretch(2, 2);
    return page;
}

QWidget* MainWindow::createChargerPage()
{
    auto* table = new EntityTableView(
        {QStringLiteral("电桩编号"), QStringLiteral("所属站点"), QStringLiteral("类型"),
         QStringLiteral("功率"), QStringLiteral("状态"), QStringLiteral("累计次数"),
         QStringLiteral("累计时长"), QStringLiteral("操作")}, this);
    auto* page = new QWidget(this);
    page->setObjectName(QStringLiteral("pageRoot"));
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(theme::kPageMargin, 20,
                               theme::kPageMargin, theme::kPageMargin);
    layout->addWidget(createSectionCard(QStringLiteral("全部充电桩"), table));
    return page;
}

QWidget* MainWindow::createStationPage()
{
    auto* table = new EntityTableView(
        {QStringLiteral("站点 ID"), QStringLiteral("站名"), QStringLiteral("地址"),
         QStringLiteral("经度"), QStringLiteral("纬度"), QStringLiteral("总桩数"),
         QStringLiteral("在线率"), QStringLiteral("操作")}, this);
    auto* page = new QWidget(this);
    page->setObjectName(QStringLiteral("pageRoot"));
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(theme::kPageMargin, 20,
                               theme::kPageMargin, theme::kPageMargin);
    layout->addWidget(createSectionCard(QStringLiteral("充电站列表"), table));
    return page;
}

QWidget* MainWindow::createUserPage()
{
    auto* table = new EntityTableView(
        {QStringLiteral("用户 ID"), QStringLiteral("手机号"), QStringLiteral("昵称"),
         QStringLiteral("余额"), QStringLiteral("注册时间"), QStringLiteral("状态"),
         QStringLiteral("操作")}, this);
    auto* page = new QWidget(this);
    page->setObjectName(QStringLiteral("pageRoot"));
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(theme::kPageMargin, 20,
                               theme::kPageMargin, theme::kPageMargin);
    layout->addWidget(createSectionCard(QStringLiteral("平台用户"), table));
    return page;
}

QWidget* MainWindow::createRevenuePage()
{
    auto* page = new QWidget(this);
    page->setObjectName(QStringLiteral("pageRoot"));
    auto* layout = new QGridLayout(page);
    layout->setContentsMargins(theme::kPageMargin, 20,
                               theme::kPageMargin, theme::kPageMargin);
    layout->setSpacing(14);
    layout->addWidget(createMetricCard(QStringLiteral("今日营收"), QStringLiteral("¥ --"),
                                       QStringLiteral("已结算订单口径待冻结"), QStringLiteral("#38E6A5")), 0, 0);
    layout->addWidget(createMetricCard(QStringLiteral("本月营收"), QStringLiteral("¥ --"),
                                       QStringLiteral("已结算订单口径待冻结"), QStringLiteral("#48C8FF")), 0, 1);
    layout->addWidget(createMetricCard(QStringLiteral("累计营收"), QStringLiteral("¥ --"),
                                       QStringLiteral("已结算订单口径待冻结"), QStringLiteral("#9C8CFF")), 0, 2);
    layout->addWidget(createSectionCard(
                          QStringLiteral("营收趋势 · 7 日 / 30 日"),
                          createPlaceholderPanel(
                              QStringLiteral("等待营收序列接口"),
                              QStringLiteral("缺失日期补零与迟到响应保护将在业务节点完成"),
                              QStringLiteral("#48C8FF"))),
                      1, 0, 1, 3);
    layout->setRowStretch(1, 1);
    return page;
}

QWidget* MainWindow::createMetricCard(const QString& label,
                                      const QString& value,
                                      const QString& hint,
                                      const QString& accent)
{
    auto* card = new QFrame(this);
    card->setObjectName(QStringLiteral("metricCard"));
    card->setMinimumHeight(104);
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(5);
    auto* line = new QFrame(card);
    line->setFixedHeight(2);
    line->setStyleSheet(QStringLiteral("background:%1; border-radius:1px;").arg(accent));
    layout->addWidget(line);
    layout->addWidget(makeLabel(label, QStringLiteral("metricLabel"), card));
    layout->addWidget(makeLabel(value, QStringLiteral("metricValue"), card));
    layout->addWidget(makeLabel(hint, QStringLiteral("metricHint"), card));
    return card;
}

QWidget* MainWindow::createSectionCard(const QString& title, QWidget* content)
{
    auto* card = new QFrame(this);
    card->setObjectName(QStringLiteral("sectionCard"));
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 14, 16, 16);
    layout->setSpacing(10);
    layout->addWidget(makeLabel(title, QStringLiteral("sectionTitle"), card));
    layout->addWidget(content, 1);
    return card;
}

QWidget* MainWindow::createPlaceholderPanel(const QString& title,
                                            const QString& message,
                                            const QString& accent)
{
    auto* panel = new QWidget(this);
    auto* layout = new QVBoxLayout(panel);
    layout->setAlignment(Qt::AlignCenter);
    auto* mark = new QLabel(QStringLiteral("━━━"), panel);
    mark->setAlignment(Qt::AlignCenter);
    mark->setStyleSheet(QStringLiteral("color:%1; font-size:22px;").arg(accent));
    auto* titleLabel = makeLabel(title, QStringLiteral("stateTitle"), panel);
    titleLabel->setAlignment(Qt::AlignCenter);
    auto* messageLabel = makeLabel(message, QStringLiteral("stateMessage"), panel);
    messageLabel->setAlignment(Qt::AlignCenter);
    messageLabel->setWordWrap(true);
    layout->addWidget(mark);
    layout->addWidget(titleLabel);
    layout->addWidget(messageLabel);
    return panel;
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
