#include "ui/RevenueStatisticsPage.h"

#include "api/AdminApiClient.h"
#include "ui/widgets/RevenueTrendChart.h"

#include <QButtonGroup>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

QLabel* makeLabel(const QString& text, const QString& objectName, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setObjectName(objectName);
    return label;
}

} // namespace

RevenueStatisticsPage::RevenueStatisticsPage(AdminApiClient* api, QWidget* parent)
    : QWidget(parent)
    , api_(api)
{
    Q_ASSERT(api_);
    setObjectName(QStringLiteral("pageRoot"));
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 24);
    root->setSpacing(14);

    auto* controlCard = new QFrame(this);
    controlCard->setObjectName(QStringLiteral("sectionCard"));
    auto* controlLayout = new QHBoxLayout(controlCard);
    controlLayout->setContentsMargins(16, 12, 16, 12);
    auto* copy = new QVBoxLayout;
    copy->setSpacing(2);
    copy->addWidget(makeLabel(QStringLiteral("已结算营收看板"),
                              QStringLiteral("sectionTitle"), controlCard));
    copy->addWidget(makeLabel(
        QStringLiteral("仅统计 FINISHED 订单 · 按结算日期归属自然日"),
        QStringLiteral("muted"), controlCard));
    controlLayout->addLayout(copy);
    controlLayout->addStretch();
    summaryState_ = makeLabel(QStringLiteral("等待加载"),
                              QStringLiteral("metricChip"), controlCard);
    refreshButton_ = new QPushButton(QStringLiteral("刷新营收"), controlCard);
    refreshButton_->setObjectName(QStringLiteral("secondaryButton"));
    connect(refreshButton_, &QPushButton::clicked,
            this, &RevenueStatisticsPage::refresh);
    controlLayout->addWidget(summaryState_);
    controlLayout->addWidget(refreshButton_);
    root->addWidget(controlCard);

    auto* metrics = new QGridLayout;
    metrics->setContentsMargins(0, 0, 0, 0);
    metrics->setHorizontalSpacing(14);
    metrics->addWidget(createMetricCard(QStringLiteral("今日营收"),
                                        QStringLiteral("#14865A"), &todayValue_), 0, 0);
    metrics->addWidget(createMetricCard(QStringLiteral("本月营收"),
                                        QStringLiteral("#1769E8"), &monthValue_), 0, 1);
    metrics->addWidget(createMetricCard(QStringLiteral("累计营收"),
                                        QStringLiteral("#7954C8"), &totalValue_), 0, 2);
    for (int column = 0; column < 3; ++column) metrics->setColumnStretch(column, 1);
    root->addLayout(metrics);

    auto* trendCard = new QFrame(this);
    trendCard->setObjectName(QStringLiteral("sectionCard"));
    auto* trendLayout = new QVBoxLayout(trendCard);
    trendLayout->setContentsMargins(16, 14, 16, 16);
    trendLayout->setSpacing(10);
    auto* toolbar = new QHBoxLayout;
    auto* titleCopy = new QVBoxLayout;
    titleCopy->setSpacing(2);
    titleCopy->addWidget(makeLabel(QStringLiteral("营收趋势"),
                                   QStringLiteral("sectionTitle"), trendCard));
    trendState_ = makeLabel(QStringLiteral("最近 7 个自然日 · 服务端补齐零收入日期"),
                            QStringLiteral("muted"), trendCard);
    titleCopy->addWidget(trendState_);
    toolbar->addLayout(titleCopy);
    toolbar->addStretch();
    sevenDayButton_ = new QPushButton(QStringLiteral("近 7 日"), trendCard);
    thirtyDayButton_ = new QPushButton(QStringLiteral("近 30 日"), trendCard);
    sevenDayButton_->setObjectName(QStringLiteral("segmentButton"));
    thirtyDayButton_->setObjectName(QStringLiteral("segmentButton"));
    sevenDayButton_->setCheckable(true);
    thirtyDayButton_->setCheckable(true);
    auto* group = new QButtonGroup(this);
    group->setExclusive(true);
    group->addButton(sevenDayButton_);
    group->addButton(thirtyDayButton_);
    sevenDayButton_->setChecked(true);
    connect(sevenDayButton_, &QPushButton::clicked, this,
            [this] { selectDays(7); });
    connect(thirtyDayButton_, &QPushButton::clicked, this,
            [this] { selectDays(30); });
    toolbar->addWidget(sevenDayButton_);
    toolbar->addWidget(thirtyDayButton_);
    trendLayout->addLayout(toolbar);

    chart_ = new RevenueTrendChart(trendCard);
    trendLayout->addWidget(chart_, 1);

    auto* note = makeLabel(
        QStringLiteral("口径说明：待支付订单尚未完成扣款，不计入营收；金额与日期桶均由服务端聚合。"),
        QStringLiteral("revenueNote"), trendCard);
    note->setWordWrap(true);
    trendLayout->addWidget(note);
    root->addWidget(trendCard, 1);
}

QWidget* RevenueStatisticsPage::createMetricCard(const QString& title,
                                                 const QString& accent,
                                                 QLabel** valueLabel)
{
    auto* card = new QFrame(this);
    card->setObjectName(QStringLiteral("metricCard"));
    card->setMinimumHeight(116);
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(17, 14, 17, 14);
    layout->setSpacing(6);
    auto* line = new QFrame(card);
    line->setFixedHeight(3);
    line->setStyleSheet(QStringLiteral("background:%1; border-radius:1px;").arg(accent));
    layout->addWidget(line);
    layout->addWidget(makeLabel(title, QStringLiteral("metricLabel"), card));
    *valueLabel = makeLabel(QStringLiteral("¥ --"), QStringLiteral("metricValue"), card);
    layout->addWidget(*valueLabel);
    layout->addWidget(makeLabel(QStringLiteral("服务端已结算订单口径"),
                                QStringLiteral("metricHint"), card));
    return card;
}

void RevenueStatisticsPage::refresh()
{
    requestSummary();
    requestTrend();
}

void RevenueStatisticsPage::refreshSummary()
{
    requestSummary();
}

void RevenueStatisticsPage::selectDays(int days)
{
    if (days != 7 && days != 30) return;
    selectedDays_ = days;
    requestTrend();
}

void RevenueStatisticsPage::requestSummary()
{
    if (api_->isRevenueSummaryInFlight()) return;
    setSummaryLoading(true);
    const QPointer<RevenueStatisticsPage> guardedThis(this);
    const bool started = api_->requestRevenueSummary(
        [guardedThis](std::optional<RevenueSummary> summary,
                      const QString& error) {
            if (!guardedThis) return;
            guardedThis->setSummaryLoading(false);
            if (!summary) guardedThis->showSummaryError(error);
            else guardedThis->showSummary(*summary);
        });
    if (!started) setSummaryLoading(false);
}

void RevenueStatisticsPage::requestTrend()
{
    const int requestedDays = selectedDays_;
    const quint64 generation = ++trendGeneration_;
    chart_->showLoading(requestedDays);
    trendState_->setText(QStringLiteral("正在加载最近 %1 个自然日…").arg(requestedDays));
    const QPointer<RevenueStatisticsPage> guardedThis(this);
    const bool started = api_->requestRevenueTrend(
        requestedDays,
        [guardedThis, generation](std::optional<RevenueTrend> trend,
                                  const QString& error) {
            if (!guardedThis || generation != guardedThis->trendGeneration_) return;
            if (!trend) {
                guardedThis->chart_->showError(error);
                guardedThis->trendState_->setText(QStringLiteral("加载失败 · 可切换周期或刷新重试"));
                return;
            }
            guardedThis->showTrend(*trend);
        });
    if (!started && generation == trendGeneration_) {
        chart_->showError(QStringLiteral("趋势请求未能启动"));
    }
}

void RevenueStatisticsPage::setSummaryLoading(bool loading)
{
    refreshButton_->setEnabled(!loading);
    refreshButton_->setText(loading ? QStringLiteral("刷新中…")
                                    : QStringLiteral("刷新营收"));
    if (loading) {
        summaryState_->setText(QStringLiteral("汇总加载中"));
        summaryState_->setStyleSheet(QStringLiteral("color:#A66C00;"));
    }
}

void RevenueStatisticsPage::showSummary(const RevenueSummary& summary)
{
    todayValue_->setText(summary.formattedTodayRevenue());
    monthValue_->setText(summary.formattedMonthRevenue());
    totalValue_->setText(summary.formattedTotalRevenue());
    const QString updatedAt = summary.formattedGeneratedAt();
    summaryState_->setText(QStringLiteral("截至 %1").arg(updatedAt));
    summaryState_->setStyleSheet(QStringLiteral("color:#14865A;"));
    emit summaryUpdated(summary.todayRevenue, summary.monthRevenue,
                        summary.totalRevenue, updatedAt);
}

void RevenueStatisticsPage::showSummaryError(const QString& message)
{
    todayValue_->setText(QStringLiteral("¥ --"));
    monthValue_->setText(QStringLiteral("¥ --"));
    totalValue_->setText(QStringLiteral("¥ --"));
    summaryState_->setText(message.isEmpty() ? QStringLiteral("汇总加载失败") : message);
    summaryState_->setStyleSheet(QStringLiteral("color:#C43742;"));
    emit summaryInvalidated();
}

void RevenueStatisticsPage::showTrend(const RevenueTrend& trend)
{
    chart_->showTrend(trend);
    trendState_->setText(
        QStringLiteral("%1 个连续自然日 · 更新时间 %2")
            .arg(trend.days).arg(trend.formattedGeneratedAt()));
}
