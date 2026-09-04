#include "ui/ChargerManagementPage.h"

#include "api/AdminApiClient.h"
#include "protocol/Protocol.h"
#include "ui/theme/Theme.h"
#include "ui/widgets/EntityTableView.h"

#include <QApplication>
#include <QBrush>
#include <QComboBox>
#include <QDialog>
#include <QFrame>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPointer>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <QTableView>
#include <QVBoxLayout>

#include <algorithm>

namespace {

class StatusItemDelegate final : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override
    {
        QStyleOptionViewItem backgroundOption(option);
        initStyleOption(&backgroundOption, index);
        backgroundOption.text.clear();
        QStyle* style = backgroundOption.widget
            ? backgroundOption.widget->style() : QApplication::style();
        style->drawControl(QStyle::CE_ItemViewItem, &backgroundOption, painter,
                           backgroundOption.widget);

        const QString text = index.data(Qt::DisplayRole).toString();
        const QBrush brush = qvariant_cast<QBrush>(index.data(Qt::ForegroundRole));
        const QColor color = brush.color().isValid() ? brush.color()
                                                     : option.palette.text().color();
        const int width = option.fontMetrics.horizontalAdvance(text) + 22;
        QRect badge(option.rect.left() + 8, option.rect.center().y() - 13,
                    std::min(width, option.rect.width() - 16), 26);

        QColor fill = color;
        fill.setAlpha(35);
        QColor border = color;
        border.setAlpha(120);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(QPen(border));
        painter->setBrush(fill);
        painter->drawRoundedRect(badge, 8, 8);
        painter->setPen(color);
        QFont font = option.font;
        font.setBold(true);
        painter->setFont(font);
        painter->drawText(badge.adjusted(10, 0, -8, 0),
                          Qt::AlignLeft | Qt::AlignVCenter, text);
        painter->restore();
    }
};

QString statusText(const Charger& charger)
{
    return charger.status == protocol::ChargerStatusFault
        ? QStringLiteral("● 故障 · 需处理")
        : QStringLiteral("● %1").arg(charger.statusLabel());
}

QColor statusColor(int status)
{
    switch (status) {
    case protocol::ChargerStatusIdle:     return QColor(QStringLiteral("#38E6A5"));
    case protocol::ChargerStatusCharging: return QColor(QStringLiteral("#48C8FF"));
    case protocol::ChargerStatusFault:    return QColor(QStringLiteral("#FF6268"));
    case protocol::ChargerStatusOffline:  return QColor(QStringLiteral("#8DA8C0"));
    default:                              return QColor(QStringLiteral("#FFC04D"));
    }
}

bool confirmRestart(QWidget* parent, const Charger& charger)
{
    QDialog dialog(parent);
    dialog.setObjectName(QStringLiteral("operationDialog"));
    dialog.setWindowTitle(QStringLiteral("确认远程重启"));
    dialog.setModal(true);
    dialog.setMinimumWidth(480);

    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(26, 24, 26, 22);
    layout->setSpacing(14);

    auto* eyebrow = new QLabel(QStringLiteral("受控设备操作"), &dialog);
    eyebrow->setObjectName(QStringLiteral("dialogEyebrow"));
    auto* title = new QLabel(QStringLiteral("确认远程重启？"), &dialog);
    title->setObjectName(QStringLiteral("dialogTitle"));
    auto* description = new QLabel(
        QStringLiteral("系统将向服务端提交重启指令，客户端不会自行修改设备状态。"),
        &dialog);
    description->setObjectName(QStringLiteral("dialogText"));
    description->setWordWrap(true);
    layout->addWidget(eyebrow);
    layout->addWidget(title);
    layout->addWidget(description);

    auto* targetPanel = new QFrame(&dialog);
    targetPanel->setObjectName(QStringLiteral("dialogPanel"));
    auto* targetLayout = new QVBoxLayout(targetPanel);
    targetLayout->setContentsMargins(16, 14, 16, 14);
    targetLayout->setSpacing(5);
    auto* code = new QLabel(charger.code, targetPanel);
    code->setObjectName(QStringLiteral("dialogTarget"));
    auto* details = new QLabel(
        QStringLiteral("所属电站  %1\n当前状态  %2\n设备 ID   %3")
            .arg(charger.stationName, charger.statusLabel())
            .arg(charger.chargerId),
        targetPanel);
    details->setObjectName(QStringLiteral("dialogDetails"));
    targetLayout->addWidget(code);
    targetLayout->addWidget(details);
    layout->addWidget(targetPanel);

    auto* warning = new QLabel(
        QStringLiteral("⚠ 若设备正在服务订单，服务端将拒绝本次操作并返回原因。"),
        &dialog);
    warning->setObjectName(QStringLiteral("dialogWarning"));
    warning->setWordWrap(true);
    layout->addWidget(warning);

    auto* buttons = new QHBoxLayout;
    buttons->addStretch();
    auto* cancel = new QPushButton(QStringLiteral("取消"), &dialog);
    cancel->setObjectName(QStringLiteral("secondaryButton"));
    cancel->setMinimumWidth(104);
    auto* confirm = new QPushButton(QStringLiteral("确认重启"), &dialog);
    confirm->setObjectName(QStringLiteral("dangerButton"));
    confirm->setMinimumWidth(112);
    cancel->setDefault(true);
    QObject::connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    QObject::connect(confirm, &QPushButton::clicked, &dialog, &QDialog::accept);
    buttons->addWidget(cancel);
    buttons->addWidget(confirm);
    layout->addLayout(buttons);
    return dialog.exec() == QDialog::Accepted;
}

void showOperationResult(QWidget* parent, bool ok,
                         const QString& titleText, const QString& message)
{
    QDialog dialog(parent);
    dialog.setObjectName(QStringLiteral("operationDialog"));
    dialog.setWindowTitle(titleText);
    dialog.setModal(true);
    dialog.setMinimumWidth(430);

    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(26, 24, 26, 22);
    layout->setSpacing(12);
    auto* status = new QLabel(ok ? QStringLiteral("✓") : QStringLiteral("!"), &dialog);
    status->setObjectName(QStringLiteral("dialogStatus"));
    status->setProperty("result", ok ? QStringLiteral("success")
                                     : QStringLiteral("error"));
    status->setAlignment(Qt::AlignCenter);
    auto* title = new QLabel(titleText, &dialog);
    title->setObjectName(QStringLiteral("dialogTitle"));
    title->setAlignment(Qt::AlignCenter);
    auto* body = new QLabel(message, &dialog);
    body->setObjectName(QStringLiteral("dialogText"));
    body->setAlignment(Qt::AlignCenter);
    body->setWordWrap(true);
    auto* close = new QPushButton(QStringLiteral("知道了"), &dialog);
    close->setObjectName(ok ? QStringLiteral("primaryButton")
                            : QStringLiteral("secondaryButton"));
    close->setMinimumWidth(112);
    QObject::connect(close, &QPushButton::clicked, &dialog, &QDialog::accept);
    layout->addWidget(status, 0, Qt::AlignHCenter);
    layout->addWidget(title);
    layout->addWidget(body);
    layout->addSpacing(4);
    layout->addWidget(close, 0, Qt::AlignHCenter);
    dialog.exec();
}

} // namespace

ChargerManagementPage::ChargerManagementPage(AdminApiClient* api, QWidget* parent)
    : QWidget(parent)
    , api_(api)
{
    Q_ASSERT(api_);
    setObjectName(QStringLiteral("pageRoot"));

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(theme::kPageMargin, 20,
                                   theme::kPageMargin, theme::kPageMargin);
    rootLayout->setSpacing(14);

    auto* toolbar = new QFrame(this);
    toolbar->setObjectName(QStringLiteral("sectionCard"));
    auto* toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(16, 12, 16, 12);
    auto* titleLayout = new QVBoxLayout;
    titleLayout->setSpacing(2);
    auto* title = new QLabel(QStringLiteral("设备运维列表"), toolbar);
    title->setObjectName(QStringLiteral("sectionTitle"));
    stateLabel_ = new QLabel(QStringLiteral("进入页面后从服务端加载真实设备"), toolbar);
    stateLabel_->setObjectName(QStringLiteral("stateMessage"));
    titleLayout->addWidget(title);
    titleLayout->addWidget(stateLabel_);
    toolbarLayout->addLayout(titleLayout);
    toolbarLayout->addStretch();
    faultAlertButton_ = new QPushButton(toolbar);
    faultAlertButton_->setObjectName(QStringLiteral("faultAlertButton"));
    faultAlertButton_->setVisible(false);
    faultAlertButton_->setToolTip(QStringLiteral("仅显示需要管理员关注的故障设备"));
    connect(faultAlertButton_, &QPushButton::clicked, this, [this] {
        const int faultIndex = statusFilter_->findData(protocol::ChargerStatusFault);
        if (faultIndex >= 0) {
            statusFilter_->setCurrentIndex(faultIndex);
        }
    });
    toolbarLayout->addWidget(faultAlertButton_);
    refreshButton_ = new QPushButton(QStringLiteral("刷新列表"), toolbar);
    refreshButton_->setObjectName(QStringLiteral("secondaryButton"));
    connect(refreshButton_, &QPushButton::clicked,
            this, &ChargerManagementPage::refresh);
    toolbarLayout->addWidget(refreshButton_);
    rootLayout->addWidget(toolbar);

    auto* filterBar = new QFrame(this);
    filterBar->setObjectName(QStringLiteral("filterBar"));
    auto* filterLayout = new QHBoxLayout(filterBar);
    filterLayout->setContentsMargins(16, 11, 16, 11);
    filterLayout->setSpacing(10);

    searchEdit_ = new QLineEdit(filterBar);
    searchEdit_->setPlaceholderText(QStringLiteral("搜索电桩编号 / 电站名称"));
    searchEdit_->setClearButtonEnabled(true);
    searchEdit_->setMinimumWidth(230);
    searchEdit_->setAccessibleName(QStringLiteral("电桩关键词"));

    stationFilter_ = new QComboBox(filterBar);
    stationFilter_->setMinimumWidth(180);
    stationFilter_->setAccessibleName(QStringLiteral("所属电站筛选"));
    stationFilter_->addItem(QStringLiteral("全部电站"), 0);

    typeFilter_ = new QComboBox(filterBar);
    typeFilter_->setMinimumWidth(120);
    typeFilter_->setAccessibleName(QStringLiteral("充电类型筛选"));
    typeFilter_->addItem(QStringLiteral("全部类型"), -1);
    typeFilter_->addItem(QStringLiteral("快充"), protocol::ChargerTypeFast);
    typeFilter_->addItem(QStringLiteral("慢充"), protocol::ChargerTypeSlow);

    statusFilter_ = new QComboBox(filterBar);
    statusFilter_->setMinimumWidth(120);
    statusFilter_->setAccessibleName(QStringLiteral("运行状态筛选"));
    statusFilter_->addItem(QStringLiteral("全部状态"), -1);
    statusFilter_->addItem(QStringLiteral("空闲"), protocol::ChargerStatusIdle);
    statusFilter_->addItem(QStringLiteral("在用"), protocol::ChargerStatusCharging);
    statusFilter_->addItem(QStringLiteral("故障"), protocol::ChargerStatusFault);
    statusFilter_->addItem(QStringLiteral("离线"), protocol::ChargerStatusOffline);

    resetFilterButton_ = new QPushButton(QStringLiteral("清空筛选"), filterBar);
    resetFilterButton_->setObjectName(QStringLiteral("ghostButton"));
    filterCountLabel_ = new QLabel(QStringLiteral("显示 0 / 0 台"), filterBar);
    filterCountLabel_->setObjectName(QStringLiteral("filterCount"));

    filterLayout->addWidget(searchEdit_, 2);
    filterLayout->addWidget(stationFilter_);
    filterLayout->addWidget(typeFilter_);
    filterLayout->addWidget(statusFilter_);
    filterLayout->addWidget(resetFilterButton_);
    filterLayout->addStretch();
    filterLayout->addWidget(filterCountLabel_);
    rootLayout->addWidget(filterBar);

    connect(searchEdit_, &QLineEdit::textChanged,
            this, [this] { applyFilters(); });
    connect(stationFilter_, &QComboBox::currentIndexChanged,
            this, [this] { applyFilters(); });
    connect(typeFilter_, &QComboBox::currentIndexChanged,
            this, [this] { applyFilters(); });
    connect(statusFilter_, &QComboBox::currentIndexChanged,
            this, [this] { applyFilters(); });
    connect(resetFilterButton_, &QPushButton::clicked,
            this, &ChargerManagementPage::resetFilters);

    table_ = new EntityTableView(
        {QStringLiteral("电桩编号 ↕"), QStringLiteral("所属电站 ↕"),
         QStringLiteral("类型 ↕"), QStringLiteral("功率 ↕"),
         QStringLiteral("当前状态 ↕"), QStringLiteral("累计充电次数 ↕"),
         QStringLiteral("累计充电时长 ↕")}, this);
    table_->tableView()->setItemDelegateForColumn(
        4, new StatusItemDelegate(table_->tableView()));
    connect(table_, &EntityTableView::retryRequested,
            this, &ChargerManagementPage::refresh);
    connect(table_->tableView()->selectionModel(),
            &QItemSelectionModel::currentRowChanged,
            this, &ChargerManagementPage::updateSelection);

    auto* tableCard = new QFrame(this);
    tableCard->setObjectName(QStringLiteral("sectionCard"));
    auto* tableLayout = new QVBoxLayout(tableCard);
    tableLayout->setContentsMargins(16, 14, 16, 16);
    tableLayout->setSpacing(10);
    auto* tableTitle = new QLabel(QStringLiteral("全部充电桩"), tableCard);
    tableTitle->setObjectName(QStringLiteral("sectionTitle"));
    auto* tableTitleLayout = new QHBoxLayout;
    sortHintLabel_ = new QLabel(tableCard);
    sortHintLabel_->setObjectName(QStringLiteral("sortHint"));
    tableTitleLayout->addWidget(tableTitle);
    tableTitleLayout->addStretch();
    tableTitleLayout->addWidget(sortHintLabel_);
    tableLayout->addLayout(tableTitleLayout);
    tableLayout->addWidget(table_, 1);
    rootLayout->addWidget(tableCard, 1);

    connect(table_->tableView()->horizontalHeader(),
            &QHeaderView::sortIndicatorChanged,
            this, &ChargerManagementPage::updateSortHint);
    table_->tableView()->sortByColumn(0, Qt::AscendingOrder);
    updateSortHint(0, Qt::AscendingOrder);

    auto* operationBar = new QFrame(this);
    operationBar->setObjectName(QStringLiteral("sectionCard"));
    auto* operationLayout = new QHBoxLayout(operationBar);
    operationLayout->setContentsMargins(16, 12, 16, 12);
    selectionLabel_ = new QLabel(QStringLiteral("请选择一台充电桩"), operationBar);
    selectionLabel_->setObjectName(QStringLiteral("pageSubtitle"));
    operationLayout->addWidget(selectionLabel_);
    operationLayout->addStretch();
    restartButton_ = new QPushButton(QStringLiteral("远程重启"), operationBar);
    restartButton_->setObjectName(QStringLiteral("dangerButton"));
    restartButton_->setEnabled(false);
    connect(restartButton_, &QPushButton::clicked,
            this, &ChargerManagementPage::restartSelectedCharger);
    operationLayout->addWidget(restartButton_);
    rootLayout->addWidget(operationBar);
}

void ChargerManagementPage::refresh()
{
    if (api_->isChargerListInFlight() || api_->isChargerRestartInFlight()) {
        return;
    }

    setLoading(true);
    table_->setState(EntityTableView::State::Loading,
                     QStringLiteral("正在从服务端加载充电桩列表…"));
    const QPointer<ChargerManagementPage> guardedThis(this);
    const bool started = api_->requestChargers(
        [guardedThis](std::optional<QList<Charger>> chargers,
                      const QString& errorMessage) {
            if (!guardedThis) {
                return;
            }
            guardedThis->setLoading(false);
            if (!chargers) {
                guardedThis->showError(errorMessage);
                return;
            }
            guardedThis->showChargers(*chargers);
        });
    if (!started) {
        setLoading(false);
    }
}

void ChargerManagementPage::updateSelection(const QModelIndex& current)
{
    const qint64 chargerId = table_->entityIdAt(current).toLongLong();
    const auto iterator = chargersById_.constFind(chargerId);
    if (iterator == chargersById_.constEnd()) {
        selectionLabel_->setText(QStringLiteral("请选择一台充电桩"));
        restartButton_->setEnabled(false);
        return;
    }

    const Charger& charger = iterator.value();
    selectionLabel_->setText(
        QStringLiteral("已选择：%1 · %2 · %3（ID %4）")
            .arg(charger.code, charger.stationName, charger.statusLabel())
            .arg(charger.chargerId));
    restartButton_->setEnabled(!api_->isChargerRestartInFlight()
                               && !api_->isChargerListInFlight());
}

void ChargerManagementPage::restartSelectedCharger()
{
    const qint64 chargerId = selectedChargerId();
    const auto iterator = chargersById_.constFind(chargerId);
    if (iterator == chargersById_.constEnd() || api_->isChargerRestartInFlight()) {
        return;
    }

    const Charger charger = iterator.value();
    if (!confirmRestart(this, charger)) {
        return;
    }

    restartButton_->setEnabled(false);
    restartButton_->setText(QStringLiteral("正在提交…"));
    refreshButton_->setEnabled(false);
    const QPointer<ChargerManagementPage> guardedThis(this);
    const bool started = api_->restartCharger(
        chargerId,
        [guardedThis, code = charger.code](bool ok, const QString& message) {
            if (!guardedThis) {
                return;
            }
            guardedThis->restartButton_->setText(QStringLiteral("远程重启"));
            if (!ok) {
                guardedThis->restartButton_->setEnabled(true);
                guardedThis->refreshButton_->setEnabled(true);
                showOperationResult(guardedThis, false,
                                    QStringLiteral("重启未执行"), message);
                return;
            }

            showOperationResult(
                guardedThis, true, QStringLiteral("重启指令已接受"),
                QStringLiteral("%1：%2\n列表将重新从服务端加载。")
                    .arg(code, message));
            guardedThis->refresh();
        });
    if (!started) {
        restartButton_->setText(QStringLiteral("远程重启"));
        restartButton_->setEnabled(true);
        refreshButton_->setEnabled(true);
    }
}

void ChargerManagementPage::setLoading(bool loading)
{
    refreshButton_->setEnabled(!loading);
    refreshButton_->setText(loading ? QStringLiteral("加载中…")
                                    : QStringLiteral("刷新列表"));
    searchEdit_->setEnabled(!loading);
    stationFilter_->setEnabled(!loading);
    typeFilter_->setEnabled(!loading);
    statusFilter_->setEnabled(!loading);
    resetFilterButton_->setEnabled(!loading);
    if (loading) {
        stateLabel_->setText(QStringLiteral("正在同步服务端设备状态"));
        stateLabel_->setStyleSheet(QStringLiteral("color:#FFC04D;"));
        restartButton_->setEnabled(false);
    }
}

void ChargerManagementPage::showError(const QString& message)
{
    allChargers_.clear();
    chargersById_.clear();
    faultAlertButton_->setVisible(false);
    searchEdit_->setEnabled(false);
    stationFilter_->setEnabled(false);
    typeFilter_->setEnabled(false);
    statusFilter_->setEnabled(false);
    resetFilterButton_->setEnabled(false);
    filterCountLabel_->setText(QStringLiteral("显示 0 / 0 台"));
    selectionLabel_->setText(QStringLiteral("请选择一台充电桩"));
    restartButton_->setEnabled(false);
    stateLabel_->setText(message.isEmpty()
        ? QStringLiteral("充电桩列表加载失败") : message);
    stateLabel_->setStyleSheet(QStringLiteral("color:#FF7D87;"));
    table_->setState(EntityTableView::State::Error, stateLabel_->text());
}

void ChargerManagementPage::showChargers(const QList<Charger>& chargers)
{
    allChargers_ = chargers;
    chargersById_.clear();
    for (const Charger& charger : chargers) {
        chargersById_.insert(charger.chargerId, charger);
    }
    const qsizetype faultCount = std::count_if(
        chargers.cbegin(), chargers.cend(), [](const Charger& charger) {
            return charger.status == protocol::ChargerStatusFault;
        });
    faultAlertButton_->setText(QStringLiteral("⚠ %1 台故障待处理").arg(faultCount));
    faultAlertButton_->setVisible(faultCount > 0);
    populateStationFilter();
    applyFilters();
    stateLabel_->setText(QStringLiteral("已同步 %1 台设备 · 筛选只改变当前视图")
                             .arg(chargers.size()));
    stateLabel_->setStyleSheet(QStringLiteral("color:#38E6A5;"));
}

void ChargerManagementPage::populateStationFilter()
{
    const qint64 previousStationId = stationFilter_->currentData().toLongLong();
    QList<QPair<QString, qint64>> stations;
    QHash<qint64, QString> namesById;
    for (const Charger& charger : allChargers_) {
        namesById.insert(charger.stationId, charger.stationName);
    }
    for (auto iterator = namesById.constBegin(); iterator != namesById.constEnd();
         ++iterator) {
        stations.append({iterator.value(), iterator.key()});
    }
    std::sort(stations.begin(), stations.end(), [](const auto& left, const auto& right) {
        const int nameOrder = QString::localeAwareCompare(left.first, right.first);
        return nameOrder == 0 ? left.second < right.second : nameOrder < 0;
    });

    const QSignalBlocker blocker(stationFilter_);
    stationFilter_->clear();
    stationFilter_->addItem(QStringLiteral("全部电站"), 0);
    int restoredIndex = 0;
    for (const auto& station : stations) {
        stationFilter_->addItem(station.first, station.second);
        if (station.second == previousStationId) {
            restoredIndex = stationFilter_->count() - 1;
        }
    }
    stationFilter_->setCurrentIndex(restoredIndex);
}

void ChargerManagementPage::applyFilters()
{
    ChargerFilter filter;
    filter.keyword = searchEdit_->text();
    filter.stationId = stationFilter_->currentData().toLongLong();
    filter.type = typeFilter_->currentData().toInt();
    filter.status = statusFilter_->currentData().toInt();

    QList<EntityTableView::Row> rows;
    rows.reserve(allChargers_.size());
    for (const Charger& charger : allChargers_) {
        if (!filter.matches(charger)) {
            continue;
        }

        QList<EntityTableView::CellStyle> styles;
        if (charger.status == protocol::ChargerStatusFault) {
            for (int column = 0; column < 7; ++column) {
                styles.append({column, {}, QColor(QStringLiteral("#28151D")), false});
            }
        }
        styles.append({4, statusColor(charger.status), {}, true});
        rows.append(EntityTableView::Row{
            charger.chargerId,
            {charger.code,
             charger.stationName,
             charger.typeLabel(),
             QStringLiteral("%1 kW").arg(charger.powerKw, 0, 'f', 1),
             statusText(charger),
             QString::number(charger.totalChargeCount),
             formatDuration(charger.totalChargeDurationSeconds)},
            styles,
            {charger.code,
             charger.stationName,
             charger.type,
             charger.powerKw,
             charger.status,
             charger.totalChargeCount,
             charger.totalChargeDurationSeconds},
        });
    }

    table_->setRows(rows);
    if (rows.isEmpty()) {
        table_->setState(EntityTableView::State::Empty,
                         allChargers_.isEmpty()
                             ? QStringLiteral("服务端当前没有充电桩记录")
                             : QStringLiteral("没有符合当前筛选条件的充电桩，可清空筛选后查看全部"));
    }
    selectionLabel_->setText(QStringLiteral("请选择一台充电桩"));
    restartButton_->setEnabled(false);
    filterCountLabel_->setText(QStringLiteral("显示 %1 / %2 台")
                                   .arg(rows.size())
                                   .arg(allChargers_.size()));
}

void ChargerManagementPage::resetFilters()
{
    const QSignalBlocker searchBlocker(searchEdit_);
    const QSignalBlocker stationBlocker(stationFilter_);
    const QSignalBlocker typeBlocker(typeFilter_);
    const QSignalBlocker statusBlocker(statusFilter_);
    searchEdit_->clear();
    stationFilter_->setCurrentIndex(0);
    typeFilter_->setCurrentIndex(0);
    statusFilter_->setCurrentIndex(0);
    applyFilters();
}

void ChargerManagementPage::updateSortHint(int column, Qt::SortOrder order)
{
    static const QStringList labels = {
        QStringLiteral("电桩编号"), QStringLiteral("所属电站"),
        QStringLiteral("类型"), QStringLiteral("功率"),
        QStringLiteral("当前状态"), QStringLiteral("累计充电次数"),
        QStringLiteral("累计充电时长"),
    };
    if (column < 0 || column >= labels.size()) {
        sortHintLabel_->setText(QStringLiteral("↕ 点击列名排序"));
        return;
    }
    sortHintLabel_->setText(QStringLiteral("当前排序：%1 %2 · 点击列名切换")
                                .arg(labels.at(column),
                                     order == Qt::AscendingOrder
                                         ? QStringLiteral("↑")
                                         : QStringLiteral("↓")));
}

qint64 ChargerManagementPage::selectedChargerId() const
{
    return table_->entityIdAt(table_->tableView()->currentIndex()).toLongLong();
}

QString ChargerManagementPage::formatDuration(qint64 totalSeconds)
{
    if (totalSeconds <= 0) {
        return QStringLiteral("0 分钟");
    }

    const qint64 totalMinutes = totalSeconds / 60;
    if (totalMinutes == 0) {
        return QStringLiteral("不足 1 分钟");
    }
    const qint64 days = totalMinutes / (24 * 60);
    const qint64 hours = (totalMinutes / 60) % 24;
    const qint64 minutes = totalMinutes % 60;
    if (days > 0) {
        return QStringLiteral("%1 天 %2 小时").arg(days).arg(hours);
    }
    if (hours > 0) {
        return QStringLiteral("%1 小时 %2 分钟").arg(hours).arg(minutes);
    }
    return QStringLiteral("%1 分钟").arg(minutes);
}
