#include "ui/StationManagementPage.h"

#include "api/AdminApiClient.h"
#include "protocol/Protocol.h"
#include "ui/theme/Theme.h"
#include "ui/widgets/EntityTableView.h"

#include <QDialog>
#include <QAbstractItemModel>
#include <QColor>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QTableView>
#include <QVBoxLayout>

#include <algorithm>
#include <limits>

namespace {

QColor onlineRateColor(const Station& station)
{
    if (station.totalCount == 0) {
        return QColor(QStringLiteral("#718399"));
    }
    if (station.onlineRate >= 95.0) {
        return QColor(QStringLiteral("#14865A"));
    }
    if (station.onlineRate >= 80.0) {
        return QColor(QStringLiteral("#1769E8"));
    }
    if (station.onlineRate >= 60.0) {
        return QColor(QStringLiteral("#A66C00"));
    }
    return QColor(QStringLiteral("#C43742"));
}

QColor chargerStatusColor(int status)
{
    switch (status) {
    case protocol::ChargerStatusIdle:     return QColor(QStringLiteral("#14865A"));
    case protocol::ChargerStatusCharging: return QColor(QStringLiteral("#1769E8"));
    case protocol::ChargerStatusFault:    return QColor(QStringLiteral("#C43742"));
    case protocol::ChargerStatusOffline:  return QColor(QStringLiteral("#718399"));
    default:                              return QColor(QStringLiteral("#A66C00"));
    }
}

bool promptNewStation(QWidget* parent, StationCreateRequest* request)
{
    if (!request) {
        return false;
    }

    QDialog dialog(parent);
    dialog.setObjectName(QStringLiteral("operationDialog"));
    dialog.setWindowTitle(QStringLiteral("新增充电站"));
    dialog.setModal(true);
    dialog.setMinimumWidth(560);

    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(28, 24, 28, 24);
    layout->setSpacing(14);

    auto* eyebrow = new QLabel(QStringLiteral("STATION ONBOARDING"), &dialog);
    eyebrow->setObjectName(QStringLiteral("dialogEyebrow"));
    auto* title = new QLabel(QStringLiteral("新增运营站点"), &dialog);
    title->setObjectName(QStringLiteral("dialogTitle"));
    auto* description = new QLabel(
        QStringLiteral("站点和初始电桩将由服务端一次创建；提交失败不会写入本地列表。"),
        &dialog);
    description->setObjectName(QStringLiteral("dialogText"));
    description->setWordWrap(true);
    layout->addWidget(eyebrow);
    layout->addWidget(title);
    layout->addWidget(description);

    auto* panel = new QFrame(&dialog);
    panel->setObjectName(QStringLiteral("dialogPanel"));
    auto* form = new QFormLayout(panel);
    form->setContentsMargins(18, 16, 18, 16);
    form->setHorizontalSpacing(16);
    form->setVerticalSpacing(12);
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    auto* nameEdit = new QLineEdit(panel);
    nameEdit->setPlaceholderText(QStringLiteral("例如：良乡大学城站"));
    nameEdit->setMaxLength(60);
    auto* addressEdit = new QLineEdit(panel);
    addressEdit->setPlaceholderText(QStringLiteral("请输入完整运营地址"));
    addressEdit->setMaxLength(200);
    auto* latitudeEdit = new QDoubleSpinBox(panel);
    latitudeEdit->setRange(-90.0, 90.0);
    latitudeEdit->setDecimals(6);
    latitudeEdit->setSingleStep(0.0001);
    latitudeEdit->setValue(39.904200);
    auto* longitudeEdit = new QDoubleSpinBox(panel);
    longitudeEdit->setRange(-180.0, 180.0);
    longitudeEdit->setDecimals(6);
    longitudeEdit->setSingleStep(0.0001);
    longitudeEdit->setValue(116.407400);
    auto* priceEdit = new QDoubleSpinBox(panel);
    priceEdit->setRange(0.01, std::numeric_limits<double>::max());
    priceEdit->setDecimals(2);
    priceEdit->setSingleStep(0.10);
    priceEdit->setValue(1.50);
    priceEdit->setPrefix(QStringLiteral("¥ "));
    priceEdit->setSuffix(QStringLiteral(" / 度"));
    auto* chargerCountEdit = new QSpinBox(panel);
    chargerCountEdit->setRange(0, 100);
    chargerCountEdit->setValue(4);
    chargerCountEdit->setSuffix(QStringLiteral(" 台"));

    form->addRow(QStringLiteral("站点名称"), nameEdit);
    form->addRow(QStringLiteral("详细地址"), addressEdit);
    form->addRow(QStringLiteral("纬度"), latitudeEdit);
    form->addRow(QStringLiteral("经度"), longitudeEdit);
    form->addRow(QStringLiteral("充电单价"), priceEdit);
    form->addRow(QStringLiteral("初始电桩"), chargerCountEdit);
    layout->addWidget(panel);

    auto* hint = new QLabel(
        QStringLiteral("可创建 0 台电桩的空站点；初始桩规格与编号由服务端统一生成。"),
        &dialog);
    hint->setObjectName(QStringLiteral("dialogWarning"));
    hint->setWordWrap(true);
    layout->addWidget(hint);

    auto* errorLabel = new QLabel(&dialog);
    errorLabel->setObjectName(QStringLiteral("formError"));
    errorLabel->setWordWrap(true);
    errorLabel->hide();
    layout->addWidget(errorLabel);

    auto* buttons = new QHBoxLayout;
    buttons->addStretch();
    auto* cancel = new QPushButton(QStringLiteral("取消"), &dialog);
    cancel->setObjectName(QStringLiteral("secondaryButton"));
    auto* submit = new QPushButton(QStringLiteral("提交创建"), &dialog);
    submit->setObjectName(QStringLiteral("primaryButton"));
    cancel->setMinimumWidth(104);
    submit->setMinimumWidth(116);
    QObject::connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    QObject::connect(submit, &QPushButton::clicked, &dialog, [&] {
        StationCreateRequest candidate;
        candidate.name = nameEdit->text();
        candidate.address = addressEdit->text();
        candidate.latitude = latitudeEdit->value();
        candidate.longitude = longitudeEdit->value();
        candidate.pricePerKwh = priceEdit->value();
        candidate.chargerCount = chargerCountEdit->value();
        QString error;
        if (!candidate.validate(&error)) {
            errorLabel->setText(error);
            errorLabel->show();
            return;
        }
        *request = candidate;
        dialog.accept();
    });
    buttons->addWidget(cancel);
    buttons->addWidget(submit);
    layout->addLayout(buttons);
    nameEdit->setFocus();
    return dialog.exec() == QDialog::Accepted;
}

bool promptEditStation(QWidget* parent, const Station& station,
                       StationUpdateRequest* request)
{
    if (!request) {
        return false;
    }
    QDialog dialog(parent);
    dialog.setObjectName(QStringLiteral("operationDialog"));
    dialog.setWindowTitle(QStringLiteral("编辑充电站"));
    dialog.setModal(true);
    dialog.setMinimumWidth(560);
    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(28, 24, 28, 24);
    layout->setSpacing(14);
    auto* eyebrow = new QLabel(QStringLiteral("STATION PROFILE"), &dialog);
    eyebrow->setObjectName(QStringLiteral("dialogEyebrow"));
    auto* title = new QLabel(QStringLiteral("编辑站点资料"), &dialog);
    title->setObjectName(QStringLiteral("dialogTitle"));
    auto* description = new QLabel(
        QStringLiteral("站点 ID %1 · 版本 %2。总桩数和在线率由服务端统计，不能在此修改。")
            .arg(station.stationId)
            .arg(station.version),
        &dialog);
    description->setObjectName(QStringLiteral("dialogText"));
    description->setWordWrap(true);
    layout->addWidget(eyebrow);
    layout->addWidget(title);
    layout->addWidget(description);

    auto* panel = new QFrame(&dialog);
    panel->setObjectName(QStringLiteral("dialogPanel"));
    auto* form = new QFormLayout(panel);
    form->setContentsMargins(18, 16, 18, 16);
    form->setHorizontalSpacing(16);
    form->setVerticalSpacing(12);
    auto* name = new QLineEdit(station.name, panel);
    name->setMaxLength(60);
    auto* address = new QLineEdit(station.address, panel);
    address->setMaxLength(200);
    auto* latitude = new QDoubleSpinBox(panel);
    latitude->setRange(-90.0, 90.0);
    latitude->setDecimals(6);
    latitude->setSingleStep(0.0001);
    latitude->setValue(station.latitude);
    auto* longitude = new QDoubleSpinBox(panel);
    longitude->setRange(-180.0, 180.0);
    longitude->setDecimals(6);
    longitude->setSingleStep(0.0001);
    longitude->setValue(station.longitude);
    auto* price = new QDoubleSpinBox(panel);
    price->setRange(0.01, std::numeric_limits<double>::max());
    price->setDecimals(2);
    price->setSingleStep(0.10);
    price->setValue(station.pricePerKwh);
    price->setPrefix(QStringLiteral("¥ "));
    price->setSuffix(QStringLiteral(" / 度"));
    form->addRow(QStringLiteral("站点名称"), name);
    form->addRow(QStringLiteral("详细地址"), address);
    form->addRow(QStringLiteral("纬度"), latitude);
    form->addRow(QStringLiteral("经度"), longitude);
    form->addRow(QStringLiteral("充电单价"), price);
    layout->addWidget(panel);

    auto* error = new QLabel(&dialog);
    error->setObjectName(QStringLiteral("formError"));
    error->setWordWrap(true);
    error->hide();
    layout->addWidget(error);
    auto* buttons = new QHBoxLayout;
    buttons->addStretch();
    auto* cancel = new QPushButton(QStringLiteral("取消"), &dialog);
    cancel->setObjectName(QStringLiteral("secondaryButton"));
    auto* submit = new QPushButton(QStringLiteral("保存修改"), &dialog);
    submit->setObjectName(QStringLiteral("primaryButton"));
    QObject::connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    QObject::connect(submit, &QPushButton::clicked, &dialog, [&] {
        StationUpdateRequest candidate;
        candidate.stationId = station.stationId;
        candidate.expectedVersion = station.version;
        candidate.name = name->text();
        candidate.address = address->text();
        candidate.latitude = latitude->value();
        candidate.longitude = longitude->value();
        candidate.pricePerKwh = price->value();
        QString validationError;
        if (!candidate.validate(&validationError)) {
            error->setText(validationError);
            error->show();
            return;
        }
        *request = candidate;
        dialog.accept();
    });
    buttons->addWidget(cancel);
    buttons->addWidget(submit);
    layout->addLayout(buttons);
    name->setFocus();
    name->selectAll();
    return dialog.exec() == QDialog::Accepted;
}

bool promptChargerStatusUpdate(QWidget* parent, const StationCharger& charger,
                               ChargerStatusUpdateRequest* request)
{
    if (!request || charger.status == protocol::ChargerStatusCharging) {
        return false;
    }
    QDialog dialog(parent);
    dialog.setObjectName(QStringLiteral("operationDialog"));
    dialog.setWindowTitle(QStringLiteral("调整站内设备状态"));
    dialog.setModal(true);
    dialog.setMinimumWidth(520);
    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(26, 24, 26, 22);
    layout->setSpacing(13);
    auto* title = new QLabel(QStringLiteral("调整 %1").arg(charger.code), &dialog);
    title->setObjectName(QStringLiteral("dialogTitle"));
    auto* description = new QLabel(
        QStringLiteral("当前状态：%1。服务端将再次校验状态与活动订单。")
            .arg(charger.statusLabel()),
        &dialog);
    description->setObjectName(QStringLiteral("dialogText"));
    layout->addWidget(title);
    layout->addWidget(description);
    auto* panel = new QFrame(&dialog);
    panel->setObjectName(QStringLiteral("dialogPanel"));
    auto* form = new QFormLayout(panel);
    form->setContentsMargins(18, 16, 18, 16);
    auto* target = new QComboBox(panel);
    const QList<QPair<QString, int>> targets = {
        {QStringLiteral("空闲"), protocol::ChargerStatusIdle},
        {QStringLiteral("故障"), protocol::ChargerStatusFault},
        {QStringLiteral("离线"), protocol::ChargerStatusOffline},
    };
    for (const auto& option : targets) {
        if (option.second != charger.status) {
            target->addItem(option.first, option.second);
        }
    }
    auto* reason = new QLineEdit(panel);
    reason->setMaxLength(200);
    reason->setPlaceholderText(QStringLiteral("必填，例如：设备检修下线"));
    form->addRow(QStringLiteral("目标状态"), target);
    form->addRow(QStringLiteral("变更原因"), reason);
    layout->addWidget(panel);
    auto* warning = new QLabel(
        QStringLiteral("不能手工设置为“在用”；正在服务订单的设备禁止强制修改。"),
        &dialog);
    warning->setObjectName(QStringLiteral("dialogWarning"));
    warning->setWordWrap(true);
    layout->addWidget(warning);
    auto* error = new QLabel(&dialog);
    error->setObjectName(QStringLiteral("formError"));
    error->hide();
    layout->addWidget(error);
    auto* buttons = new QHBoxLayout;
    buttons->addStretch();
    auto* cancel = new QPushButton(QStringLiteral("取消"), &dialog);
    cancel->setObjectName(QStringLiteral("secondaryButton"));
    auto* submit = new QPushButton(QStringLiteral("确认变更"), &dialog);
    submit->setObjectName(QStringLiteral("dangerButton"));
    QObject::connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    QObject::connect(submit, &QPushButton::clicked, &dialog, [&] {
        ChargerStatusUpdateRequest candidate;
        candidate.chargerId = charger.chargerId;
        candidate.expectedStatus = charger.status;
        candidate.targetStatus = target->currentData().toInt();
        candidate.reason = reason->text();
        QString validationError;
        if (!candidate.validate(&validationError)) {
            error->setText(validationError);
            error->show();
            return;
        }
        *request = candidate;
        dialog.accept();
    });
    buttons->addWidget(cancel);
    buttons->addWidget(submit);
    layout->addLayout(buttons);
    reason->setFocus();
    return dialog.exec() == QDialog::Accepted;
}

bool confirmStationChargerRestart(QWidget* parent, const StationCharger& charger)
{
    QDialog dialog(parent);
    dialog.setObjectName(QStringLiteral("operationDialog"));
    dialog.setWindowTitle(QStringLiteral("确认远程重启"));
    dialog.setModal(true);
    dialog.setMinimumWidth(470);
    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(26, 24, 26, 22);
    layout->setSpacing(13);
    auto* title = new QLabel(QStringLiteral("重启 %1？").arg(charger.code), &dialog);
    title->setObjectName(QStringLiteral("dialogTitle"));
    auto* body = new QLabel(
        QStringLiteral("设备 ID %1 · 当前状态 %2。客户端只提交指令，实际结果由服务端决定。")
            .arg(charger.chargerId)
            .arg(charger.statusLabel()),
        &dialog);
    body->setObjectName(QStringLiteral("dialogText"));
    body->setWordWrap(true);
    layout->addWidget(title);
    layout->addWidget(body);
    auto* warning = new QLabel(
        QStringLiteral("若设备正在服务订单，服务端必须拒绝本次重启。"), &dialog);
    warning->setObjectName(QStringLiteral("dialogWarning"));
    layout->addWidget(warning);
    auto* buttons = new QHBoxLayout;
    buttons->addStretch();
    auto* cancel = new QPushButton(QStringLiteral("取消"), &dialog);
    cancel->setObjectName(QStringLiteral("secondaryButton"));
    auto* confirm = new QPushButton(QStringLiteral("确认重启"), &dialog);
    confirm->setObjectName(QStringLiteral("dangerButton"));
    QObject::connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    QObject::connect(confirm, &QPushButton::clicked, &dialog, &QDialog::accept);
    buttons->addWidget(cancel);
    buttons->addWidget(confirm);
    layout->addLayout(buttons);
    return dialog.exec() == QDialog::Accepted;
}

void showResult(QWidget* parent, bool ok,
                const QString& titleText, const QString& message)
{
    QDialog dialog(parent);
    dialog.setObjectName(QStringLiteral("operationDialog"));
    dialog.setWindowTitle(titleText);
    dialog.setModal(true);
    dialog.setMinimumWidth(440);
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
    QObject::connect(close, &QPushButton::clicked, &dialog, &QDialog::accept);
    layout->addWidget(status, 0, Qt::AlignHCenter);
    layout->addWidget(title);
    layout->addWidget(body);
    layout->addWidget(close, 0, Qt::AlignHCenter);
    dialog.exec();
}

} // namespace

StationManagementPage::StationManagementPage(AdminApiClient* api, QWidget* parent)
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
    auto* title = new QLabel(QStringLiteral("充电站运营台账"), toolbar);
    title->setObjectName(QStringLiteral("sectionTitle"));
    stateLabel_ = new QLabel(QStringLiteral("进入页面后从服务端加载站点"), toolbar);
    stateLabel_->setObjectName(QStringLiteral("stateMessage"));
    titleLayout->addWidget(title);
    titleLayout->addWidget(stateLabel_);
    toolbarLayout->addLayout(titleLayout);
    toolbarLayout->addStretch();
    stationCountLabel_ = new QLabel(QStringLiteral("0 个站点"), toolbar);
    stationCountLabel_->setObjectName(QStringLiteral("filterCount"));
    chargerCountLabel_ = new QLabel(QStringLiteral("0 台电桩"), toolbar);
    chargerCountLabel_->setObjectName(QStringLiteral("filterCount"));
    createButton_ = new QPushButton(QStringLiteral("＋ 新增站点"), toolbar);
    createButton_->setObjectName(QStringLiteral("primaryButton"));
    editButton_ = new QPushButton(QStringLiteral("编辑站点"), toolbar);
    editButton_->setObjectName(QStringLiteral("secondaryButton"));
    editButton_->setEnabled(false);
    refreshButton_ = new QPushButton(QStringLiteral("刷新列表"), toolbar);
    refreshButton_->setObjectName(QStringLiteral("secondaryButton"));
    toolbarLayout->addWidget(stationCountLabel_);
    toolbarLayout->addWidget(chargerCountLabel_);
    toolbarLayout->addWidget(createButton_);
    toolbarLayout->addWidget(editButton_);
    toolbarLayout->addWidget(refreshButton_);
    rootLayout->addWidget(toolbar);
    connect(createButton_, &QPushButton::clicked,
            this, &StationManagementPage::createStation);
    connect(editButton_, &QPushButton::clicked,
            this, &StationManagementPage::editSelectedStation);
    connect(refreshButton_, &QPushButton::clicked,
            this, &StationManagementPage::refresh);

    auto* filterBar = new QFrame(this);
    filterBar->setObjectName(QStringLiteral("filterBar"));
    auto* filterLayout = new QHBoxLayout(filterBar);
    filterLayout->setContentsMargins(16, 11, 16, 11);
    filterLayout->setSpacing(10);
    searchEdit_ = new QLineEdit(filterBar);
    searchEdit_->setPlaceholderText(QStringLiteral("搜索站点 ID / 站名 / 地址"));
    searchEdit_->setClearButtonEnabled(true);
    searchEdit_->setMinimumWidth(300);
    filterCountLabel_ = new QLabel(QStringLiteral("显示 0 / 0 个"), filterBar);
    filterCountLabel_->setObjectName(QStringLiteral("filterCount"));
    filterLayout->addWidget(searchEdit_, 2);
    filterLayout->addStretch();
    filterLayout->addWidget(filterCountLabel_);
    rootLayout->addWidget(filterBar);
    connect(searchEdit_, &QLineEdit::textChanged,
            this, &StationManagementPage::applyFilter);

    stationTable_ = new EntityTableView(
        {QStringLiteral("站点 ID ↕"), QStringLiteral("站点名称 ↕"),
         QStringLiteral("详细地址 ↕"), QStringLiteral("纬度 ↕"),
         QStringLiteral("经度 ↕"), QStringLiteral("总桩数 ↕"),
         QStringLiteral("充电单价 ↕"), QStringLiteral("在线率 ↕")}, this);
    stationTable_->tableView()->horizontalHeader()->setSectionResizeMode(
        2, QHeaderView::Stretch);
    stationTable_->tableView()->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::ResizeToContents);
    stationTable_->tableView()->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::ResizeToContents);
    for (int column = 3; column <= 7; ++column) {
        stationTable_->tableView()->horizontalHeader()->setSectionResizeMode(
            column, QHeaderView::ResizeToContents);
    }
    connect(stationTable_, &EntityTableView::retryRequested,
            this, &StationManagementPage::refresh);
    connect(stationTable_->tableView()->selectionModel(),
            &QItemSelectionModel::currentRowChanged,
            this, &StationManagementPage::updateSelection);

    auto* stationCard = new QFrame(this);
    stationCard->setObjectName(QStringLiteral("sectionCard"));
    auto* stationLayout = new QVBoxLayout(stationCard);
    stationLayout->setContentsMargins(16, 14, 16, 16);
    stationLayout->setSpacing(10);
    auto* stationTitle = new QLabel(QStringLiteral("站点列表"), stationCard);
    stationTitle->setObjectName(QStringLiteral("sectionTitle"));
    stationLayout->addWidget(stationTitle);
    stationLayout->addWidget(stationTable_, 1);

    detailTable_ = new EntityTableView(
        {QStringLiteral("电桩编号 ↕"), QStringLiteral("类型 ↕"),
         QStringLiteral("功率 ↕"), QStringLiteral("实时状态 ↕"),
         QStringLiteral("充电单价 ↕")}, this);
    connect(detailTable_->tableView()->selectionModel(),
            &QItemSelectionModel::currentRowChanged,
            this, &StationManagementPage::updateDetailSelection);
    connect(detailTable_, &EntityTableView::retryRequested, this, [this] {
        const qint64 stationId = selectedStationId();
        if (stationId > 0) {
            loadDetail(stationId);
        }
    });

    auto* detailCard = new QFrame(this);
    detailCard->setObjectName(QStringLiteral("sectionCard"));
    auto* detailLayout = new QVBoxLayout(detailCard);
    detailLayout->setContentsMargins(16, 14, 16, 16);
    detailLayout->setSpacing(8);
    auto* detailHeading = new QHBoxLayout;
    detailTitle_ = new QLabel(QStringLiteral("站内设备详情"), detailCard);
    detailTitle_->setObjectName(QStringLiteral("sectionTitle"));
    detailMeta_ = new QLabel(QStringLiteral("选择一个站点查看实时明细"), detailCard);
    detailMeta_->setObjectName(QStringLiteral("stateMessage"));
    detailHeading->addWidget(detailTitle_);
    detailHeading->addStretch();
    detailHeading->addWidget(detailMeta_);
    detailLayout->addLayout(detailHeading);
    detailLayout->addWidget(detailTable_, 1);
    auto* detailOperationLayout = new QHBoxLayout;
    detailSelectionLabel_ = new QLabel(QStringLiteral("请选择一台站内设备"), detailCard);
    detailSelectionLabel_->setObjectName(QStringLiteral("pageSubtitle"));
    detailStatusButton_ = new QPushButton(QStringLiteral("调整状态"), detailCard);
    detailStatusButton_->setObjectName(QStringLiteral("secondaryButton"));
    detailStatusButton_->setEnabled(false);
    detailRestartButton_ = new QPushButton(QStringLiteral("远程重启"), detailCard);
    detailRestartButton_->setObjectName(QStringLiteral("dangerButton"));
    detailRestartButton_->setEnabled(false);
    connect(detailStatusButton_, &QPushButton::clicked,
            this, &StationManagementPage::changeSelectedChargerStatus);
    connect(detailRestartButton_, &QPushButton::clicked,
            this, &StationManagementPage::restartSelectedCharger);
    detailOperationLayout->addWidget(detailSelectionLabel_);
    detailOperationLayout->addStretch();
    detailOperationLayout->addWidget(detailStatusButton_);
    detailOperationLayout->addWidget(detailRestartButton_);
    detailLayout->addLayout(detailOperationLayout);
    detailTable_->setState(EntityTableView::State::Empty,
                           QStringLiteral("请先在上方选择一个站点"));

    auto* splitter = new QSplitter(Qt::Vertical, this);
    splitter->setObjectName(QStringLiteral("stationSplitter"));
    splitter->addWidget(stationCard);
    splitter->addWidget(detailCard);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    splitter->setSizes({410, 260});
    rootLayout->addWidget(splitter, 1);
}

void StationManagementPage::refresh()
{
    if (api_->isStationListInFlight() || api_->isStationCreateInFlight()
        || api_->isStationUpdateInFlight()
        || api_->isChargerStatusUpdateInFlight()
        || api_->isChargerRestartInFlight()) {
        return;
    }

    if (pendingSelectionId_ <= 0) {
        pendingSelectionId_ = selectedStationId();
    }
    setListLoading(true);
    stationTable_->setState(EntityTableView::State::Loading,
                            QStringLiteral("正在从服务端加载充电站列表…"));
    const QPointer<StationManagementPage> guardedThis(this);
    const bool started = api_->requestStations(
        [guardedThis](std::optional<QList<Station>> stations,
                      const QString& errorMessage) {
            if (!guardedThis) {
                return;
            }
            guardedThis->setListLoading(false);
            if (!stations) {
                guardedThis->allStations_.clear();
                guardedThis->stationsById_.clear();
                guardedThis->stationCountLabel_->setText(QStringLiteral("0 个站点"));
                guardedThis->chargerCountLabel_->setText(QStringLiteral("0 台电桩"));
                guardedThis->filterCountLabel_->setText(QStringLiteral("显示 0 / 0 个"));
                guardedThis->searchEdit_->setEnabled(false);
                guardedThis->stateLabel_->setText(errorMessage);
                guardedThis->stationTable_->setState(
                    EntityTableView::State::Error, errorMessage);
                return;
            }
            const qint64 selectId = guardedThis->pendingSelectionId_;
            guardedThis->pendingSelectionId_ = 0;
            guardedThis->showStations(*stations, selectId);
        });
    if (!started) {
        setListLoading(false);
    }
}

void StationManagementPage::applyFilter()
{
    StationFilter filter;
    filter.keyword = searchEdit_->text();
    QList<EntityTableView::Row> rows;
    for (const Station& station : allStations_) {
        if (!filter.matches(station)) {
            continue;
        }
        EntityTableView::Row row;
        row.entityId = station.stationId;
        row.cells = {
            QString::number(station.stationId),
            station.name,
            station.address,
            QString::number(station.latitude, 'f', 6),
            QString::number(station.longitude, 'f', 6),
            QStringLiteral("%1 台").arg(station.totalCount),
            QStringLiteral("¥ %1 / 度").arg(station.pricePerKwh, 0, 'f', 2),
            QStringLiteral("%1%").arg(station.onlineRate, 0, 'f', 1),
        };
        row.sortValues = {
            station.stationId, station.name, station.address,
            station.latitude, station.longitude, station.totalCount,
            station.pricePerKwh, station.onlineRate,
        };
        row.styles.append({7, onlineRateColor(station), {}, true});
        rows.append(row);
    }
    stationTable_->setRows(rows);
    if (rows.isEmpty()) {
        stationTable_->setState(EntityTableView::State::Empty,
                                allStations_.isEmpty()
                                    ? QStringLiteral("当前没有充电站数据")
                                    : QStringLiteral("没有符合搜索条件的站点"));
    }
    filterCountLabel_->setText(
        QStringLiteral("显示 %1 / %2 个").arg(rows.size()).arg(allStations_.size()));
}

void StationManagementPage::showStations(const QList<Station>& stations,
                                          qint64 selectStationId)
{
    allStations_ = stations;
    stationsById_.clear();
    qint64 totalChargers = 0;
    for (const Station& station : stations) {
        stationsById_.insert(station.stationId, station);
        totalChargers += station.totalCount;
    }
    stationCountLabel_->setText(QStringLiteral("%1 个站点").arg(stations.size()));
    chargerCountLabel_->setText(QStringLiteral("%1 台电桩").arg(totalChargers));
    stateLabel_->setText(stations.isEmpty()
        ? QStringLiteral("服务端当前没有站点数据")
        : QStringLiteral("站点数据已同步 · 在线率由服务端统一计算"));
    applyFilter();

    if (selectStationId > 0 && stationsById_.contains(selectStationId)) {
        selectStation(selectStationId);
    } else if (!stations.isEmpty()) {
        selectStation(stations.first().stationId);
    } else {
        detailTitle_->setText(QStringLiteral("站内设备详情"));
        detailMeta_->setText(QStringLiteral("暂无可选站点"));
        detailTable_->setState(EntityTableView::State::Empty,
                               QStringLiteral("新增站点后可在此查看站内设备"));
        editButton_->setEnabled(false);
        detailChargersById_.clear();
        detailSelectionLabel_->setText(QStringLiteral("请选择一台站内设备"));
        detailStatusButton_->setEnabled(false);
        detailRestartButton_->setEnabled(false);
    }
}

void StationManagementPage::updateSelection(const QModelIndex& current)
{
    const qint64 stationId = stationTable_->entityIdAt(current).toLongLong();
    const auto iterator = stationsById_.constFind(stationId);
    if (iterator == stationsById_.constEnd()) {
        ++detailRequestGeneration_;
        detailTitle_->setText(QStringLiteral("站内设备详情"));
        detailMeta_->setText(QStringLiteral("选择一个站点查看实时明细"));
        detailTable_->setState(EntityTableView::State::Empty,
                               QStringLiteral("请先在上方选择一个站点"));
        editButton_->setEnabled(false);
        detailChargersById_.clear();
        detailSelectionLabel_->setText(QStringLiteral("请选择一台站内设备"));
        detailStatusButton_->setEnabled(false);
        detailRestartButton_->setEnabled(false);
        return;
    }
    editButton_->setEnabled(!api_->isStationListInFlight()
                            && !api_->isStationCreateInFlight()
                            && !api_->isStationUpdateInFlight());
    detailTitle_->setText(QStringLiteral("%1 · 站内设备").arg(iterator->name));
    detailMeta_->setText(QStringLiteral("站点 ID %1 · 正在查询实时状态")
                             .arg(stationId));
    loadDetail(stationId);
}

void StationManagementPage::loadDetail(qint64 stationId)
{
    const quint64 generation = ++detailRequestGeneration_;
    detailTable_->setState(EntityTableView::State::Loading,
                           QStringLiteral("正在按 stationId 加载站内充电桩…"));
    detailChargersById_.clear();
    detailSelectionLabel_->setText(QStringLiteral("正在加载站内设备…"));
    detailStatusButton_->setEnabled(false);
    detailRestartButton_->setEnabled(false);
    const QPointer<StationManagementPage> guardedThis(this);
    const bool started = api_->requestStationDetail(
        stationId,
        [guardedThis, generation](std::optional<StationDetail> detail,
                                  const QString& errorMessage) {
            if (!guardedThis || generation != guardedThis->detailRequestGeneration_) {
                return;
            }
            if (!detail) {
                guardedThis->detailMeta_->setText(errorMessage);
                guardedThis->detailTable_->setState(
                    EntityTableView::State::Error, errorMessage);
                return;
            }
            guardedThis->showDetail(*detail);
        });
    if (!started) {
        detailTable_->setState(EntityTableView::State::Error,
                               QStringLiteral("无法发起站内详情请求"));
    }
}

void StationManagementPage::showDetail(const StationDetail& detail)
{
    QList<EntityTableView::Row> rows;
    detailChargersById_.clear();
    rows.reserve(detail.chargers.size());
    for (const StationCharger& charger : detail.chargers) {
        detailChargersById_.insert(charger.chargerId, charger);
        EntityTableView::Row row;
        row.entityId = charger.chargerId;
        row.cells = {
            charger.code,
            charger.typeLabel(),
            QStringLiteral("%1 kW").arg(charger.powerKw, 0, 'f', 1),
            QStringLiteral("● %1").arg(charger.statusLabel()),
            QStringLiteral("¥ %1 / 度").arg(charger.pricePerKwh, 0, 'f', 2),
        };
        row.sortValues = {
            charger.code, charger.type, charger.powerKw,
            charger.status, charger.pricePerKwh,
        };
        row.styles.append({3, chargerStatusColor(charger.status), {}, true});
        rows.append(row);
    }
    detailTable_->setRows(rows);
    detailMeta_->setText(
        QStringLiteral("%1 台设备 · 空闲 %2 台 · ¥ %3 / 度 · %4, %5")
            .arg(detail.chargers.size())
            .arg(detail.availableCount)
            .arg(detail.pricePerKwh, 0, 'f', 2)
            .arg(detail.latitude, 0, 'f', 6)
            .arg(detail.longitude, 0, 'f', 6));
    if (rows.isEmpty()) {
        detailTable_->setState(EntityTableView::State::Empty,
                               QStringLiteral("该站点当前没有充电桩"));
    }
    detailSelectionLabel_->setText(rows.isEmpty()
        ? QStringLiteral("该站点暂无可操作设备")
        : QStringLiteral("请选择一台站内设备"));
    detailStatusButton_->setEnabled(false);
    detailRestartButton_->setEnabled(false);
}

void StationManagementPage::createStation()
{
    if (api_->isStationCreateInFlight() || api_->isStationUpdateInFlight()
        || api_->isStationListInFlight()) {
        return;
    }
    StationCreateRequest request;
    if (!promptNewStation(this, &request)) {
        return;
    }

    createButton_->setEnabled(false);
    createButton_->setText(QStringLiteral("正在创建…"));
    editButton_->setEnabled(false);
    refreshButton_->setEnabled(false);
    stateLabel_->setText(QStringLiteral("正在等待服务端确认站点与初始电桩…"));
    const QPointer<StationManagementPage> guardedThis(this);
    const QString stationName = request.name.trimmed();
    const bool started = api_->createStation(
        request,
        [guardedThis, stationName](std::optional<StationCreateResult> result,
                                   const QString& errorMessage) {
            if (!guardedThis) {
                return;
            }
            guardedThis->createButton_->setEnabled(true);
            guardedThis->createButton_->setText(QStringLiteral("＋ 新增站点"));
            guardedThis->refreshButton_->setEnabled(true);
            if (!result) {
                guardedThis->stateLabel_->setText(
                    QStringLiteral("新增失败，当前列表未被修改"));
                showResult(guardedThis, false,
                           QStringLiteral("站点创建失败"), errorMessage);
                guardedThis->updateSelection(
                    guardedThis->stationTable_->tableView()->currentIndex());
                return;
            }

            showResult(
                guardedThis, true, QStringLiteral("站点创建成功"),
                QStringLiteral("%1（ID %2）已由服务端创建，包含 %3 台初始电桩。\n"
                               "现在重新查询站点列表和站内详情。")
                    .arg(stationName)
                    .arg(result->stationId)
                    .arg(result->createdChargerCount));
            guardedThis->searchEdit_->clear();
            guardedThis->pendingSelectionId_ = result->stationId;
            guardedThis->refresh();
        });
    if (!started && !api_->isStationCreateInFlight()) {
        createButton_->setEnabled(true);
        createButton_->setText(QStringLiteral("＋ 新增站点"));
        refreshButton_->setEnabled(true);
        updateSelection(stationTable_->tableView()->currentIndex());
    }
}

void StationManagementPage::editSelectedStation()
{
    const qint64 stationId = selectedStationId();
    const auto iterator = stationsById_.constFind(stationId);
    if (iterator == stationsById_.constEnd() || api_->isStationUpdateInFlight()
        || api_->isStationListInFlight() || api_->isStationCreateInFlight()) {
        return;
    }
    StationUpdateRequest request;
    if (!promptEditStation(this, iterator.value(), &request)) {
        return;
    }

    editButton_->setEnabled(false);
    editButton_->setText(QStringLiteral("正在保存…"));
    createButton_->setEnabled(false);
    refreshButton_->setEnabled(false);
    stateLabel_->setText(QStringLiteral("正在等待服务端校验站点版本并保存…"));
    const QPointer<StationManagementPage> guardedThis(this);
    const QString updatedName = request.name.trimmed();
    const bool started = api_->updateStation(
        request,
        [guardedThis, stationId, updatedName](
            std::optional<StationUpdateResult> result,
            const QString& errorMessage) {
            if (!guardedThis) {
                return;
            }
            guardedThis->editButton_->setText(QStringLiteral("编辑站点"));
            guardedThis->createButton_->setEnabled(true);
            guardedThis->refreshButton_->setEnabled(true);
            if (!result) {
                showResult(guardedThis, false,
                           QStringLiteral("站点修改未保存"), errorMessage);
                guardedThis->pendingSelectionId_ = stationId;
                guardedThis->refresh();
                return;
            }
            showResult(
                guardedThis, true, QStringLiteral("站点资料已更新"),
                QStringLiteral("%1 已由服务端保存为版本 %2，正在重新查询。")
                    .arg(updatedName)
                    .arg(result->version));
            guardedThis->searchEdit_->clear();
            guardedThis->pendingSelectionId_ = stationId;
            guardedThis->refresh();
        });
    if (!started && !api_->isStationUpdateInFlight()) {
        editButton_->setText(QStringLiteral("编辑站点"));
        createButton_->setEnabled(true);
        refreshButton_->setEnabled(true);
        updateSelection(stationTable_->tableView()->currentIndex());
    }
}

void StationManagementPage::updateDetailSelection(const QModelIndex& current)
{
    const qint64 chargerId = detailTable_->entityIdAt(current).toLongLong();
    const auto iterator = detailChargersById_.constFind(chargerId);
    if (iterator == detailChargersById_.constEnd()) {
        detailSelectionLabel_->setText(QStringLiteral("请选择一台站内设备"));
        detailStatusButton_->setEnabled(false);
        detailRestartButton_->setEnabled(false);
        return;
    }
    const StationCharger& charger = iterator.value();
    detailSelectionLabel_->setText(
        QStringLiteral("已选择：%1 · %2（ID %3）")
            .arg(charger.code, charger.statusLabel())
            .arg(charger.chargerId));
    const bool orderControlled = charger.status == protocol::ChargerStatusCharging;
    detailStatusButton_->setEnabled(!api_->isChargerStatusUpdateInFlight());
    detailStatusButton_->setToolTip(orderControlled
        ? QStringLiteral("点击查看在用设备的状态保护规则")
        : QStringLiteral("在空闲、故障和离线之间执行受控变更"));
    detailRestartButton_->setEnabled(!api_->isChargerRestartInFlight());
}

void StationManagementPage::changeSelectedChargerStatus()
{
    const qint64 chargerId = selectedDetailChargerId();
    const auto iterator = detailChargersById_.constFind(chargerId);
    if (iterator == detailChargersById_.constEnd()
        || api_->isChargerStatusUpdateInFlight()) {
        return;
    }
    const StationCharger charger = iterator.value();
    if (charger.status == protocol::ChargerStatusCharging) {
        showResult(
            this, false, QStringLiteral("状态由订单流程控制"),
            QStringLiteral("%1 正在服务预约或充电订单，管理员不能强制修改状态。\n"
                           "请等待订单正常结束；如设备异常，可先联系服务端负责人核查订单。")
                .arg(charger.code));
        return;
    }
    ChargerStatusUpdateRequest request;
    if (!promptChargerStatusUpdate(this, charger, &request)) {
        return;
    }
    const qint64 stationId = selectedStationId();
    detailStatusButton_->setEnabled(false);
    detailStatusButton_->setText(QStringLiteral("正在提交…"));
    detailRestartButton_->setEnabled(false);
    refreshButton_->setEnabled(false);
    const QPointer<StationManagementPage> guardedThis(this);
    const bool started = api_->updateChargerStatus(
        request,
        [guardedThis, stationId, code = charger.code](
            std::optional<ChargerStatusUpdateResult> result,
            const QString& errorMessage) {
            if (!guardedThis) {
                return;
            }
            guardedThis->detailStatusButton_->setText(QStringLiteral("调整状态"));
            guardedThis->refreshButton_->setEnabled(true);
            if (!result) {
                showResult(guardedThis, false,
                           QStringLiteral("状态变更未执行"), errorMessage);
            } else {
                showResult(
                    guardedThis, true, QStringLiteral("设备状态已更新"),
                    QStringLiteral("%1 的状态已由服务端确认，正在同步详情和在线率。")
                        .arg(code));
            }
            guardedThis->pendingSelectionId_ = stationId;
            guardedThis->refresh();
        });
    if (!started && !api_->isChargerStatusUpdateInFlight()) {
        detailStatusButton_->setText(QStringLiteral("调整状态"));
        refreshButton_->setEnabled(true);
        updateDetailSelection(detailTable_->tableView()->currentIndex());
    }
}

void StationManagementPage::restartSelectedCharger()
{
    const qint64 chargerId = selectedDetailChargerId();
    const auto iterator = detailChargersById_.constFind(chargerId);
    if (iterator == detailChargersById_.constEnd()
        || api_->isChargerRestartInFlight()) {
        return;
    }
    const StationCharger charger = iterator.value();
    if (!confirmStationChargerRestart(this, charger)) {
        return;
    }
    const qint64 stationId = selectedStationId();
    detailRestartButton_->setEnabled(false);
    detailRestartButton_->setText(QStringLiteral("正在提交…"));
    detailStatusButton_->setEnabled(false);
    refreshButton_->setEnabled(false);
    const QPointer<StationManagementPage> guardedThis(this);
    const bool started = api_->restartCharger(
        chargerId,
        [guardedThis, stationId, code = charger.code](bool ok,
                                                      const QString& message) {
            if (!guardedThis) {
                return;
            }
            guardedThis->detailRestartButton_->setText(QStringLiteral("远程重启"));
            guardedThis->refreshButton_->setEnabled(true);
            showResult(guardedThis, ok,
                       ok ? QStringLiteral("重启指令已接受")
                          : QStringLiteral("重启未执行"),
                       QStringLiteral("%1：%2").arg(code, message));
            guardedThis->pendingSelectionId_ = stationId;
            guardedThis->refresh();
        });
    if (!started) {
        detailRestartButton_->setText(QStringLiteral("远程重启"));
        refreshButton_->setEnabled(true);
        updateDetailSelection(detailTable_->tableView()->currentIndex());
    }
}

void StationManagementPage::selectStation(qint64 stationId)
{
    QAbstractItemModel* model = stationTable_->tableView()->model();
    for (int row = 0; row < model->rowCount(); ++row) {
        const QModelIndex index = model->index(row, 0);
        if (stationTable_->entityIdAt(index).toLongLong() == stationId) {
            stationTable_->tableView()->selectRow(row);
            stationTable_->tableView()->setCurrentIndex(index);
            return;
        }
    }
}

void StationManagementPage::setListLoading(bool loading)
{
    refreshButton_->setEnabled(!loading && !api_->isStationCreateInFlight());
    createButton_->setEnabled(!loading && !api_->isStationCreateInFlight());
    editButton_->setEnabled(false);
    detailStatusButton_->setEnabled(false);
    detailRestartButton_->setEnabled(false);
    refreshButton_->setText(loading ? QStringLiteral("正在刷新…")
                                    : QStringLiteral("刷新列表"));
    searchEdit_->setEnabled(!loading);
    if (loading) {
        stateLabel_->setText(QStringLiteral("正在同步站点基础信息与服务端统计…"));
    }
}

qint64 StationManagementPage::selectedStationId() const
{
    return stationTable_->entityIdAt(
        stationTable_->tableView()->currentIndex()).toLongLong();
}

qint64 StationManagementPage::selectedDetailChargerId() const
{
    return detailTable_->entityIdAt(
        detailTable_->tableView()->currentIndex()).toLongLong();
}
