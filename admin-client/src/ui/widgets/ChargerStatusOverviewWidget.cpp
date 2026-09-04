#include "ui/widgets/ChargerStatusOverviewWidget.h"

#include "api/AdminApiClient.h"
#include "model/ChargerStatusOverview.h"

#include <QDateTime>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPointer>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

QLabel* makeLabel(const QString& text, const QString& objectName, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setObjectName(objectName);
    return label;
}

QString formatPercent(double value)
{
    return QStringLiteral("%1%").arg(value, 0, 'f', 1);
}

} // namespace

ChargerStatusOverviewWidget::ChargerStatusOverviewWidget(AdminApiClient* api,
                                                         QWidget* parent)
    : QWidget(parent)
    , api_(api)
{
    Q_ASSERT(api_);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(14);

    auto* toolbar = new QHBoxLayout;
    auto* summaryLayout = new QVBoxLayout;
    summaryLayout->setSpacing(2);
    summaryLabel_ = makeLabel(QStringLiteral("总计 -- 台"),
                              QStringLiteral("stateTitle"), this);
    stateLabel_ = makeLabel(QStringLiteral("登录后自动获取服务端聚合数据"),
                            QStringLiteral("stateMessage"), this);
    summaryLayout->addWidget(summaryLabel_);
    summaryLayout->addWidget(stateLabel_);
    toolbar->addLayout(summaryLayout);
    toolbar->addStretch();
    refreshButton_ = new QPushButton(QStringLiteral("刷新状态"), this);
    refreshButton_->setObjectName(QStringLiteral("secondaryButton"));
    connect(refreshButton_, &QPushButton::clicked,
            this, &ChargerStatusOverviewWidget::refresh);
    toolbar->addWidget(refreshButton_);
    rootLayout->addLayout(toolbar);

    auto* cards = new QGridLayout;
    cards->setContentsMargins(0, 0, 0, 0);
    cards->setHorizontalSpacing(12);
    cards->addWidget(createStatusCard(0, QStringLiteral("闲置"),
                                      QStringLiteral("可预约或开始充电"),
                                      QStringLiteral("#38E6A5")), 0, 0);
    cards->addWidget(createStatusCard(1, QStringLiteral("在用"),
                                      QStringLiteral("已预约或正在充电"),
                                      QStringLiteral("#48C8FF")), 0, 1);
    cards->addWidget(createStatusCard(2, QStringLiteral("故障"),
                                      QStringLiteral("需要运维处理"),
                                      QStringLiteral("#FF6268")), 0, 2);
    cards->addWidget(createStatusCard(3, QStringLiteral("离线"),
                                      QStringLiteral("未连接平台"),
                                      QStringLiteral("#8DA8C0")), 0, 3);
    for (int column = 0; column < 4; ++column) {
        cards->setColumnStretch(column, 1);
    }
    rootLayout->addLayout(cards);
}

QWidget* ChargerStatusOverviewWidget::createStatusCard(int index,
                                                       const QString& title,
                                                       const QString& hint,
                                                       const QString& accent)
{
    auto* card = new QFrame(this);
    card->setObjectName(QStringLiteral("statusCard"));
    card->setMinimumHeight(132);
    card->setStyleSheet(QStringLiteral(
        "QFrame#statusCard { background:#091725; border:1px solid #183A55;"
        " border-radius:11px; }"));

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->setSpacing(5);
    auto* titleRow = new QHBoxLayout;
    auto* dot = new QLabel(QStringLiteral("●"), card);
    dot->setStyleSheet(QStringLiteral("color:%1; font-size:13px;").arg(accent));
    titleRow->addWidget(dot);
    titleRow->addWidget(makeLabel(title, QStringLiteral("metricLabel"), card));
    titleRow->addStretch();
    statuses_.at(index).percent = makeLabel(QStringLiteral("--%"),
                                            QStringLiteral("metricLabel"), card);
    titleRow->addWidget(statuses_.at(index).percent);
    layout->addLayout(titleRow);

    statuses_.at(index).count = makeLabel(QStringLiteral("--"),
                                          QStringLiteral("metricValue"), card);
    layout->addWidget(statuses_.at(index).count);

    statuses_.at(index).progress = new QProgressBar(card);
    statuses_.at(index).progress->setRange(0, 1000);
    statuses_.at(index).progress->setValue(0);
    statuses_.at(index).progress->setTextVisible(false);
    statuses_.at(index).progress->setFixedHeight(6);
    statuses_.at(index).progress->setStyleSheet(QStringLiteral(
        "QProgressBar { background:#102438; border:0; border-radius:3px; }"
        "QProgressBar::chunk { background:%1; border-radius:3px; }").arg(accent));
    layout->addWidget(statuses_.at(index).progress);
    layout->addWidget(makeLabel(hint, QStringLiteral("metricHint"), card));
    return card;
}

void ChargerStatusOverviewWidget::refresh()
{
    if (api_->isChargerOverviewInFlight()) {
        return;
    }

    setLoading(true);
    const QPointer<ChargerStatusOverviewWidget> guardedThis(this);
    const bool started = api_->requestChargerOverview(
        [guardedThis](std::optional<ChargerStatusOverview> overview,
                      const QString& errorMessage) {
            if (!guardedThis) {
                return;
            }
            guardedThis->setLoading(false);
            if (!overview) {
                guardedThis->showError(errorMessage);
                return;
            }
            guardedThis->showOverview(*overview);
        });
    if (!started) {
        setLoading(false);
    }
}

void ChargerStatusOverviewWidget::setLoading(bool loading)
{
    refreshButton_->setEnabled(!loading);
    refreshButton_->setText(loading ? QStringLiteral("刷新中…")
                                    : QStringLiteral("刷新状态"));
    if (loading) {
        stateLabel_->setText(QStringLiteral("正在获取服务端聚合数据，请稍候"));
        stateLabel_->setStyleSheet(QStringLiteral("color:#FFC04D;"));
    }
}

void ChargerStatusOverviewWidget::showError(const QString& message)
{
    stateLabel_->setText(message.isEmpty()
        ? QStringLiteral("状态加载失败，请稍后重试")
        : message);
    stateLabel_->setStyleSheet(QStringLiteral("color:#FF7D87;"));
}

void ChargerStatusOverviewWidget::showOverview(const ChargerStatusOverview& overview)
{
    const qint64 counts[] = {overview.idle, overview.charging,
                             overview.fault, overview.offline};
    const double percents[] = {overview.idlePercent, overview.chargingPercent,
                               overview.faultPercent, overview.offlinePercent};
    for (int index = 0; index < static_cast<int>(statuses_.size()); ++index) {
        statuses_.at(index).count->setText(QString::number(counts[index]));
        statuses_.at(index).percent->setText(formatPercent(percents[index]));
        statuses_.at(index).progress->setValue(
            qBound(0, qRound(percents[index] * 10.0), 1000));
    }

    summaryLabel_->setText(QStringLiteral("总计 %1 台").arg(overview.total));
    const QString updatedText = overview.updatedAtMs > 0
        ? QDateTime::fromMSecsSinceEpoch(overview.updatedAtMs)
              .toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))
        : QStringLiteral("服务端未提供更新时间");
    stateLabel_->setText(QStringLiteral("数据更新时间：%1").arg(updatedText));
    stateLabel_->setStyleSheet(QStringLiteral("color:#38E6A5;"));
    emit overviewUpdated(overview.total);
}
