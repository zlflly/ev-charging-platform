#include "ui/OperationsOverviewPage.h"

#include "api/AdminApiClient.h"
#include "model/Charger.h"
#include "model/Revenue.h"
#include "model/Station.h"
#include "model/User.h"
#include "protocol/Protocol.h"
#include "ui/theme/Theme.h"
#include "ui/widgets/ChargerStatusOverviewWidget.h"
#include "ui/widgets/RevenueTrendChart.h"

#include <QAbstractItemView>
#include <QColor>
#include <QDateTime>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPointer>
#include <QPushButton>
#include <QSize>
#include <QTimer>
#include <QVBoxLayout>

namespace {

QLabel* makeLabel(const QString& text, const QString& objectName, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setObjectName(objectName);
    return label;
}

QString money(double amount)
{
    return QStringLiteral("¥ %1").arg(amount, 0, 'f', 2);
}

QListWidget* createAttentionList(QWidget* parent)
{
    auto* list = new QListWidget(parent);
    list->setObjectName(QStringLiteral("attentionList"));
    list->setSelectionMode(QAbstractItemView::NoSelection);
    list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    list->setWordWrap(true);
    list->setSpacing(2);
    list->setMinimumHeight(72);
    return list;
}

void showAttentionState(QListWidget* list, const QString& text, const QColor& color)
{
    list->clear();
    auto* item = new QListWidgetItem(text, list);
    item->setForeground(color);
    item->setFlags(Qt::ItemIsEnabled);
    item->setSizeHint(QSize(0, 44));
}

void addAttentionRow(QListWidget* list,
                     const QString& primary,
                     const QString& secondary,
                     const QColor& color)
{
    auto* item = new QListWidgetItem(
        QStringLiteral("%1\n%2").arg(primary, secondary), list);
    item->setForeground(color);
    item->setFlags(Qt::ItemIsEnabled);
    item->setSizeHint(QSize(0, 50));
    item->setToolTip(QStringLiteral("%1 · %2").arg(primary, secondary));
}

} // namespace

OperationsOverviewPage::OperationsOverviewPage(AdminApiClient* api, QWidget* parent)
    : QWidget(parent)
    , api_(api)
{
    Q_ASSERT(api_);
    setObjectName(QStringLiteral("pageRoot"));

    auto* layout = new QGridLayout(this);
    layout->setContentsMargins(theme::kPageMargin, 20,
                               theme::kPageMargin, theme::kPageMargin);
    layout->setHorizontalSpacing(14);
    layout->setVerticalSpacing(14);

    layout->addWidget(createMetricCard(QStringLiteral("今日营收"), QStringLiteral("¥ --"),
                                       QStringLiteral("FINISHED 已结算订单"), QStringLiteral("#14865A"),
                                       &todayRevenueLabel_), 0, 0);
    layout->addWidget(createMetricCard(QStringLiteral("本月营收"), QStringLiteral("¥ --"),
                                       QStringLiteral("自然月 · Asia/Shanghai"), QStringLiteral("#1769E8"),
                                       &monthRevenueLabel_), 0, 1);
    layout->addWidget(createMetricCard(QStringLiteral("累计营收"), QStringLiteral("¥ --"),
                                       QStringLiteral("已结算订单累计"), QStringLiteral("#7954C8"),
                                       &totalRevenueLabel_), 0, 2);
    layout->addWidget(createMetricCard(QStringLiteral("运营站点"), QStringLiteral("--"),
                                       QStringLiteral("站点列表实时聚合"), QStringLiteral("#A66C00"),
                                       &stationTotalLabel_), 0, 3);
    layout->addWidget(createMetricCard(QStringLiteral("充电桩总数"), QStringLiteral("--"),
                                       QStringLiteral("设备状态实时聚合"), QStringLiteral("#2AD4D9"),
                                       &chargerTotalLabel_), 0, 4);
    layout->addWidget(createMetricCard(QStringLiteral("平台用户"), QStringLiteral("--"),
                                       QStringLiteral("活跃业务 -- 人"), QStringLiteral("#D9486E"),
                                       &userTotalLabel_, &userHintLabel_), 0, 5);

    chargerOverviewWidget_ = new ChargerStatusOverviewWidget(api_, this);
    connect(chargerOverviewWidget_, &ChargerStatusOverviewWidget::overviewUpdated,
            this, [this](qint64 total) {
        chargerTotalLabel_->setText(QString::number(total));
    });
    layout->addWidget(createSectionCard(QStringLiteral("充电桩状态总览"),
                                        chargerOverviewWidget_),
                      1, 0, 1, 6);

    auto* trendPanel = new QWidget(this);
    auto* trendLayout = new QVBoxLayout(trendPanel);
    trendLayout->setContentsMargins(0, 0, 0, 0);
    trendLayout->setSpacing(6);
    revenueUpdatedLabel_ = makeLabel(QStringLiteral("最近 7 个自然日 · 等待服务端数据"),
                                     QStringLiteral("metricHint"), trendPanel);
    revenueTrendChart_ = new RevenueTrendChart(trendPanel);
    revenueTrendChart_->setMinimumHeight(220);
    trendLayout->addWidget(revenueUpdatedLabel_);
    trendLayout->addWidget(revenueTrendChart_, 1);
    layout->addWidget(createSectionCard(QStringLiteral("营收趋势 · 最近 7 日"), trendPanel),
                      2, 0, 1, 4);

    layout->addWidget(createSectionCard(QStringLiteral("运营关注"), createAttentionPanel()),
                      2, 4, 1, 2);

    for (int column = 0; column < 6; ++column) {
        layout->setColumnStretch(column, 1);
    }
    layout->setRowStretch(1, 2);
    layout->setRowStretch(2, 3);
}

QWidget* OperationsOverviewPage::createMetricCard(const QString& label,
                                                   const QString& value,
                                                   const QString& hint,
                                                   const QString& accent,
                                                   QLabel** valueLabel,
                                                   QLabel** hintLabel)
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
    auto* metricValue = makeLabel(value, QStringLiteral("metricValue"), card);
    if (valueLabel) *valueLabel = metricValue;
    layout->addWidget(metricValue);
    auto* metricHint = makeLabel(hint, QStringLiteral("metricHint"), card);
    if (hintLabel) *hintLabel = metricHint;
    layout->addWidget(metricHint);
    return card;
}

QWidget* OperationsOverviewPage::createSectionCard(const QString& title, QWidget* content)
{
    auto* card = new QFrame(this);
    card->setObjectName(QStringLiteral("sectionCard"));
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 14, 16, 16);
    layout->setSpacing(8);
    layout->addWidget(makeLabel(title, QStringLiteral("sectionTitle"), card));
    layout->addWidget(content, 1);
    return card;
}

QWidget* OperationsOverviewPage::createAttentionPanel()
{
    auto* panel = new QWidget(this);
    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    auto* summaryRow = new QHBoxLayout;
    attentionSummaryLabel_ = makeLabel(QStringLiteral("活跃业务 -- 人 · 故障设备 -- 台"),
                                       QStringLiteral("metricHint"), panel);
    refreshButton_ = new QPushButton(QStringLiteral("刷新看板"), panel);
    refreshButton_->setObjectName(QStringLiteral("secondaryButton"));
    connect(refreshButton_, &QPushButton::clicked, this, &OperationsOverviewPage::refresh);
    summaryRow->addWidget(attentionSummaryLabel_, 1);
    summaryRow->addWidget(refreshButton_);
    auto* activeTitle = makeLabel(QStringLiteral("活跃业务（服务端结果前 3 条）"),
                                  QStringLiteral("metricLabel"), panel);
    activeUserList_ = createAttentionList(panel);
    showAttentionState(activeUserList_, QStringLiteral("正在加载活跃业务…"),
                       QColor(QStringLiteral("#64778B")));
    auto* faultTitle = makeLabel(QStringLiteral("故障设备"),
                                 QStringLiteral("metricLabel"), panel);
    faultList_ = createAttentionList(panel);
    showAttentionState(faultList_, QStringLiteral("正在加载故障设备…"),
                       QColor(QStringLiteral("#64778B")));
    layout->addLayout(summaryRow);
    layout->addWidget(activeTitle);
    layout->addWidget(activeUserList_, 1);
    layout->addWidget(faultTitle);
    layout->addWidget(faultList_, 1);
    return panel;
}

void OperationsOverviewPage::refresh()
{
    if (!refreshButton_->isEnabled()) return;
    refreshButton_->setEnabled(false);
    refreshButton_->setText(QStringLiteral("刷新中…"));
    QTimer::singleShot(800, this, [this] {
        refreshButton_->setEnabled(true);
        refreshButton_->setText(QStringLiteral("刷新看板"));
    });
    chargerOverviewWidget_->refresh();
    refreshRevenue();
    refreshStationCount();
    refreshFaultChargers();
    refreshUserCounts();
}

void OperationsOverviewPage::refreshRevenue()
{
    todayRevenueLabel_->setText(QStringLiteral("¥ --"));
    monthRevenueLabel_->setText(QStringLiteral("¥ --"));
    totalRevenueLabel_->setText(QStringLiteral("¥ --"));
    api_->requestRevenueSummary(
        [guardedThis = QPointer<OperationsOverviewPage>(this)](
            std::optional<RevenueSummary> summary, const QString&) {
            if (!guardedThis) return;
            if (!summary) {
                guardedThis->todayRevenueLabel_->setText(QStringLiteral("¥ --"));
                guardedThis->monthRevenueLabel_->setText(QStringLiteral("¥ --"));
                guardedThis->totalRevenueLabel_->setText(QStringLiteral("¥ --"));
                return;
            }
            guardedThis->todayRevenueLabel_->setText(money(summary->todayRevenue));
            guardedThis->monthRevenueLabel_->setText(money(summary->monthRevenue));
            guardedThis->totalRevenueLabel_->setText(money(summary->totalRevenue));
        });

    const int generation = ++trendGeneration_;
    revenueTrendChart_->showLoading(7);
    revenueUpdatedLabel_->setText(QStringLiteral("最近 7 个自然日 · 正在获取服务端统计"));
    const bool started = api_->requestRevenueTrend(
        7, [guardedThis = QPointer<OperationsOverviewPage>(this), generation](
               std::optional<RevenueTrend> trend, const QString& errorMessage) {
            if (!guardedThis || generation != guardedThis->trendGeneration_) return;
            if (!trend) {
                guardedThis->revenueTrendChart_->showError(errorMessage);
                guardedThis->revenueUpdatedLabel_->setText(QStringLiteral("趋势加载失败 · 可重新进入页面刷新"));
                return;
            }
            guardedThis->revenueTrendChart_->showTrend(*trend);
            guardedThis->revenueUpdatedLabel_->setText(
                QStringLiteral("最近 7 个自然日 · 服务端按日聚合 · 更新于 %1")
                    .arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"))));
        });
    if (!started) {
        revenueTrendChart_->showError(QStringLiteral("趋势请求未发出，请稍后重试"));
    }
}

void OperationsOverviewPage::refreshStationCount()
{
    stationTotalLabel_->setText(QStringLiteral("--"));
    stationTotalLabel_->setToolTip(QString());
    const bool started = api_->requestStations(
        [guardedThis = QPointer<OperationsOverviewPage>(this)](
            std::optional<QList<Station>> stations, const QString&) {
            if (!guardedThis) return;
            guardedThis->stationTotalLabel_->setText(
                stations ? QString::number(stations->size()) : QStringLiteral("--"));
        });
    if (!started) stationTotalLabel_->setToolTip(QStringLiteral("站点列表请求正在处理中"));
}

void OperationsOverviewPage::refreshFaultChargers()
{
    faultChargerCount_ = -1;
    updateAttentionSummary();
    showAttentionState(faultList_, QStringLiteral("正在加载故障设备…"),
                       QColor(QStringLiteral("#64778B")));
    const bool started = api_->requestChargers(
        [guardedThis = QPointer<OperationsOverviewPage>(this)](
            std::optional<QList<Charger>> chargers, const QString& errorMessage) {
            if (!guardedThis) return;
            if (!chargers) {
                guardedThis->faultChargerCount_ = -1;
                showAttentionState(
                    guardedThis->faultList_,
                    errorMessage.isEmpty() ? QStringLiteral("故障设备加载失败") : errorMessage,
                    QColor(QStringLiteral("#C43742")));
                guardedThis->updateAttentionSummary();
                return;
            }
            guardedThis->faultList_->clear();
            for (const Charger& charger : *chargers) {
                if (charger.status != protocol::ChargerStatusFault) continue;
                addAttentionRow(
                    guardedThis->faultList_,
                    QStringLiteral("%1 · %2").arg(charger.code, charger.statusLabel()),
                    charger.stationName,
                    QColor(QStringLiteral("#C43742")));
            }
            guardedThis->faultChargerCount_ = guardedThis->faultList_->count();
            if (guardedThis->faultList_->count() == 0) {
                showAttentionState(guardedThis->faultList_,
                                   QStringLiteral("当前没有故障设备"),
                                   QColor(QStringLiteral("#14865A")));
            }
            guardedThis->updateAttentionSummary();
        });
    if (!started) {
        showAttentionState(faultList_,
                           QStringLiteral("设备列表请求正在处理中，请稍后刷新"),
                           QColor(QStringLiteral("#A66C00")));
    }
}

void OperationsOverviewPage::refreshUserCounts()
{
    userTotalLabel_->setText(QStringLiteral("--"));
    userHintLabel_->setText(QStringLiteral("活跃业务 -- 人"));
    activeUserCount_ = -1;
    showAttentionState(activeUserList_, QStringLiteral("正在加载用户总数…"),
                       QColor(QStringLiteral("#64778B")));
    updateAttentionSummary();
    UserListQuery query;
    query.page = 1;
    query.pageSize = 1;
    query.activityFilter = QStringLiteral("ALL");
    const bool started = api_->requestUsers(
        query, [guardedThis = QPointer<OperationsOverviewPage>(this)](
                   std::optional<UserListPage> page, const QString&) {
            if (!guardedThis) return;
            guardedThis->userTotalLabel_->setText(
                page ? QString::number(page->total) : QStringLiteral("--"));
            guardedThis->refreshActiveUserCount();
        });
    if (!started) {
        userHintLabel_->setText(QStringLiteral("活跃业务稍后刷新"));
    }
}

void OperationsOverviewPage::refreshActiveUserCount()
{
    showAttentionState(activeUserList_, QStringLiteral("正在加载活跃业务…"),
                       QColor(QStringLiteral("#64778B")));
    UserListQuery query;
    query.page = 1;
    query.pageSize = 3;
    query.activityFilter = QStringLiteral("ACTIVE");
    const bool started = api_->requestUsers(
        query, [guardedThis = QPointer<OperationsOverviewPage>(this)](
                   std::optional<UserListPage> page, const QString&) {
            if (!guardedThis) return;
            guardedThis->activeUserCount_ = page ? page->total : -1;
            guardedThis->userHintLabel_->setText(page
                ? QStringLiteral("活跃业务 %1 人").arg(page->total)
                : QStringLiteral("活跃业务 -- 人"));
            if (!page) {
                showAttentionState(guardedThis->activeUserList_,
                                   QStringLiteral("活跃业务加载失败"),
                                   QColor(QStringLiteral("#C43742")));
            } else {
                guardedThis->activeUserList_->clear();
                for (const AdminUser& user : page->users) {
                    addAttentionRow(
                        guardedThis->activeUserList_,
                        QStringLiteral("%1 · %2").arg(user.phone, user.activityLabel()),
                        user.currentDeviceLabel(),
                        QColor(QStringLiteral("#1769E8")));
                }
                if (guardedThis->activeUserList_->count() == 0) {
                    showAttentionState(guardedThis->activeUserList_,
                                       QStringLiteral("当前没有未完成业务"),
                                       QColor(QStringLiteral("#14865A")));
                }
            }
            guardedThis->updateAttentionSummary();
        });
    if (!started) {
        activeUserCount_ = -1;
        showAttentionState(activeUserList_,
                           QStringLiteral("用户查询正在处理中，请稍后刷新"),
                           QColor(QStringLiteral("#A66C00")));
        updateAttentionSummary();
    }
}

void OperationsOverviewPage::updateAttentionSummary()
{
    const QString active = activeUserCount_ >= 0
        ? QString::number(activeUserCount_) : QStringLiteral("--");
    const QString fault = faultChargerCount_ >= 0
        ? QString::number(faultChargerCount_) : QStringLiteral("--");
    attentionSummaryLabel_->setText(
        QStringLiteral("活跃业务 %1 人 · 故障设备 %2 台").arg(active, fault));
}
