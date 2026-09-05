#include "ui/OrderManagementPage.h"

#include "api/AdminApiClient.h"
#include "protocol/Protocol.h"
#include "ui/widgets/EntityTableView.h"

#include <QColor>
#include <QComboBox>
#include <QFrame>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>

namespace {

QFrame* makeMetricCard(const QString& title, QLabel** value,
                       const QString& hint, QWidget* parent)
{
    auto* card = new QFrame(parent);
    card->setObjectName(QStringLiteral("metricCard"));
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(14, 10, 14, 10);
    layout->setSpacing(3);
    auto* caption = new QLabel(title, card);
    caption->setObjectName(QStringLiteral("metricLabel"));
    *value = new QLabel(QStringLiteral("--"), card);
    (*value)->setObjectName(QStringLiteral("metricValue"));
    auto* footnote = new QLabel(hint, card);
    footnote->setObjectName(QStringLiteral("metricHint"));
    layout->addWidget(caption);
    layout->addWidget(*value);
    layout->addWidget(footnote);
    return card;
}

QLabel* makeDetailValue(QWidget* parent)
{
    auto* label = new QLabel(QStringLiteral("—"), parent);
    label->setObjectName(QStringLiteral("dialogDetails"));
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    return label;
}

} // namespace

OrderManagementPage::OrderManagementPage(AdminApiClient* api, QWidget* parent)
    : QWidget(parent), api_(api)
{
    Q_ASSERT(api_);
    setObjectName(QStringLiteral("pageRoot"));
    query_.pageSize = 20;

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 18, 24, 22);
    root->setSpacing(12);

    auto* metrics = new QHBoxLayout;
    metrics->setSpacing(12);
    metrics->addWidget(makeMetricCard(QStringLiteral("平台订单"), &totalValue_,
                                      QStringLiteral("全部状态"), this), 1);
    metrics->addWidget(makeMetricCard(QStringLiteral("已预约"), &reservedValue_,
                                      QStringLiteral("已占用充电桩"), this), 1);
    metrics->addWidget(makeMetricCard(QStringLiteral("充电中"), &chargingValue_,
                                      QStringLiteral("金额为实时预估"), this), 1);
    metrics->addWidget(makeMetricCard(QStringLiteral("待支付"), &unpaidValue_,
                                      QStringLiteral("账单已生成"), this), 1);
    metrics->addWidget(makeMetricCard(QStringLiteral("已支付金额"), &paidAmountValue_,
                                      QStringLiteral("仅 FINISHED"), this), 1);
    root->addLayout(metrics);

    auto* filter = new QFrame(this);
    filter->setObjectName(QStringLiteral("sectionCard"));
    auto* filterLayout = new QHBoxLayout(filter);
    filterLayout->setContentsMargins(16, 11, 16, 11);
    filterLayout->setSpacing(8);
    keywordEdit_ = new QLineEdit(filter);
    keywordEdit_->setPlaceholderText(
        QStringLiteral("搜索订单号 / 手机号 / 昵称 / 站点 / 充电桩"));
    keywordEdit_->setMaxLength(50);
    keywordEdit_->setClearButtonEnabled(true);
    statusFilter_ = new QComboBox(filter);
    statusFilter_->addItem(QStringLiteral("全部业务状态"), QStringLiteral("ALL"));
    statusFilter_->addItem(QStringLiteral("已预约"), QString::fromUtf8(protocol::orderStatus::kReserved));
    statusFilter_->addItem(QStringLiteral("充电中"), QString::fromUtf8(protocol::orderStatus::kCharging));
    statusFilter_->addItem(QStringLiteral("待支付"), QString::fromUtf8(protocol::orderStatus::kWaitSettlement));
    statusFilter_->addItem(QStringLiteral("已完成"), QString::fromUtf8(protocol::orderStatus::kFinished));
    paymentFilter_ = new QComboBox(filter);
    paymentFilter_->addItem(QStringLiteral("全部支付状态"), QStringLiteral("ALL"));
    paymentFilter_->addItem(QStringLiteral("未到结算"), QStringLiteral("NOT_DUE"));
    paymentFilter_->addItem(QStringLiteral("待支付"), QStringLiteral("UNPAID"));
    paymentFilter_->addItem(QStringLiteral("已支付"), QStringLiteral("PAID"));
    searchButton_ = new QPushButton(QStringLiteral("查询"), filter);
    searchButton_->setObjectName(QStringLiteral("primaryButton"));
    resetButton_ = new QPushButton(QStringLiteral("重置"), filter);
    resetButton_->setObjectName(QStringLiteral("secondaryButton"));
    refreshButton_ = new QPushButton(QStringLiteral("刷新实时数据"), filter);
    refreshButton_->setObjectName(QStringLiteral("secondaryButton"));
    filterLayout->addWidget(keywordEdit_, 1);
    filterLayout->addWidget(statusFilter_);
    filterLayout->addWidget(paymentFilter_);
    filterLayout->addWidget(searchButton_);
    filterLayout->addWidget(resetButton_);
    filterLayout->addWidget(refreshButton_);
    root->addWidget(filter);

    auto* body = new QHBoxLayout;
    body->setSpacing(12);
    auto* tableCard = new QFrame(this);
    tableCard->setObjectName(QStringLiteral("sectionCard"));
    auto* tableLayout = new QVBoxLayout(tableCard);
    tableLayout->setContentsMargins(16, 12, 16, 12);
    tableLayout->setSpacing(8);
    auto* tableHeader = new QHBoxLayout;
    auto* tableTitle = new QLabel(QStringLiteral("订单流水"), tableCard);
    tableTitle->setObjectName(QStringLiteral("sectionTitle"));
    updatedLabel_ = new QLabel(QStringLiteral("等待服务端数据"), tableCard);
    updatedLabel_->setObjectName(QStringLiteral("muted"));
    tableHeader->addWidget(tableTitle);
    tableHeader->addStretch();
    tableHeader->addWidget(updatedLabel_);
    tableLayout->addLayout(tableHeader);
    table_ = new EntityTableView(
        {QStringLiteral("订单号"), QStringLiteral("用户"), QStringLiteral("站点 / 充电桩"),
         QStringLiteral("业务状态"), QStringLiteral("支付状态"), QStringLiteral("电量"),
         QStringLiteral("金额"), QStringLiteral("下单时间")}, tableCard);
    table_->tableView()->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table_->tableView()->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    table_->tableView()->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
    table_->tableView()->sortByColumn(0, Qt::DescendingOrder);
    tableLayout->addWidget(table_, 1);
    auto* pager = new QHBoxLayout;
    queryLabel_ = new QLabel(QStringLiteral("匹配 -- 笔订单"), tableCard);
    queryLabel_->setObjectName(QStringLiteral("muted"));
    previousButton_ = new QPushButton(QStringLiteral("上一页"), tableCard);
    previousButton_->setObjectName(QStringLiteral("secondaryButton"));
    pageLabel_ = new QLabel(QStringLiteral("第 1 页"), tableCard);
    pageLabel_->setObjectName(QStringLiteral("muted"));
    nextButton_ = new QPushButton(QStringLiteral("下一页"), tableCard);
    nextButton_->setObjectName(QStringLiteral("secondaryButton"));
    pager->addWidget(queryLabel_);
    pager->addStretch();
    pager->addWidget(previousButton_);
    pager->addWidget(pageLabel_);
    pager->addWidget(nextButton_);
    tableLayout->addLayout(pager);
    body->addWidget(tableCard, 3);

    auto* detail = new QFrame(this);
    detail->setObjectName(QStringLiteral("sectionCard"));
    detail->setMinimumWidth(310);
    detail->setMaximumWidth(390);
    auto* detailLayout = new QVBoxLayout(detail);
    detailLayout->setContentsMargins(16, 14, 16, 14);
    detailLayout->setSpacing(9);
    detailTitle_ = new QLabel(QStringLiteral("订单详情"), detail);
    detailTitle_->setObjectName(QStringLiteral("sectionTitle"));
    auto* detailHint = new QLabel(
        QStringLiteral("用户、设备、计费与生命周期由服务端关联返回"), detail);
    detailHint->setObjectName(QStringLiteral("muted"));
    detailHint->setWordWrap(true);
    detailUser_ = makeDetailValue(detail);
    detailDevice_ = makeDetailValue(detail);
    detailBilling_ = makeDetailValue(detail);
    detailTimes_ = makeDetailValue(detail);
    detailLayout->addWidget(detailTitle_);
    detailLayout->addWidget(detailHint);
    detailLayout->addSpacing(3);
    detailLayout->addWidget(detailUser_);
    detailLayout->addWidget(detailDevice_);
    detailLayout->addWidget(detailBilling_);
    detailLayout->addWidget(detailTimes_);
    detailLayout->addStretch();
    body->addWidget(detail, 1);
    root->addLayout(body, 1);

    connect(searchButton_, &QPushButton::clicked, this, &OrderManagementPage::search);
    connect(keywordEdit_, &QLineEdit::returnPressed, this, &OrderManagementPage::search);
    connect(resetButton_, &QPushButton::clicked, this, &OrderManagementPage::resetSearch);
    connect(refreshButton_, &QPushButton::clicked, this, &OrderManagementPage::refresh);
    connect(previousButton_, &QPushButton::clicked, this,
            [this] { loadPage(query_.page - 1); });
    connect(nextButton_, &QPushButton::clicked, this,
            [this] { loadPage(query_.page + 1); });
    connect(table_, &EntityTableView::retryRequested, this, &OrderManagementPage::refresh);
    connect(table_->tableView()->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, &OrderManagementPage::updateSelection);
    showOrderDetail(nullptr);
}

void OrderManagementPage::refresh() { loadPage(query_.page); }

void OrderManagementPage::search()
{
    OrderListQuery next = query_;
    next.page = 1;
    next.keyword = keywordEdit_->text();
    next.status = statusFilter_->currentData().toString();
    next.paymentStatus = paymentFilter_->currentData().toString();
    QString error;
    if (!next.validate(&error)) {
        QMessageBox::warning(this, QStringLiteral("查询条件无效"), error);
        return;
    }
    query_ = next;
    loadPage(1);
}

void OrderManagementPage::resetSearch()
{
    keywordEdit_->clear();
    statusFilter_->setCurrentIndex(0);
    paymentFilter_->setCurrentIndex(0);
    query_.keyword.clear();
    query_.status = QStringLiteral("ALL");
    query_.paymentStatus = QStringLiteral("ALL");
    loadPage(1);
}

void OrderManagementPage::loadPage(int page)
{
    if (page < 1 || api_->isOrderListInFlight()) return;
    OrderListQuery requested = query_;
    requested.page = page;
    setLoading(true);
    table_->setState(EntityTableView::State::Loading,
                     QStringLiteral("正在聚合订单、用户与设备信息…"));
    const bool started = api_->requestOrders(
        requested, [this, requested](std::optional<OrderListPage> result,
                                    const QString& error) {
            setLoading(false);
            if (!result) {
                ordersById_.clear();
                showOrderDetail(nullptr);
                table_->setState(EntityTableView::State::Error, error);
                previousButton_->setEnabled(false);
                nextButton_->setEnabled(false);
                return;
            }
            query_ = requested;
            if (result->orders.isEmpty() && result->total > 0 && result->page > 1) {
                loadPage(result->page - 1);
                return;
            }
            showPage(*result);
        });
    if (!started) setLoading(false);
}

void OrderManagementPage::showPage(const OrderListPage& page)
{
    ordersById_.clear();
    QList<EntityTableView::Row> rows;
    for (const AdminOrder& order : page.orders) {
        ordersById_.insert(order.orderId, order);
        EntityTableView::Row row;
        row.entityId = order.orderId;
        row.cells = {QString::number(order.orderId), order.userLabel(), order.deviceLabel(),
                     order.statusLabel(), order.paymentStatusLabel(), order.formattedEnergy(),
                     order.formattedAmount(), order.formattedCreatedAt()};
        row.sortValues = {order.orderId, order.phone, order.stationName, order.status,
                          order.paymentStatus, order.energyKwh, order.amount,
                          order.createdAtEpochMs};
        QColor statusColor(QStringLiteral("#1769E8"));
        if (order.status == QString::fromUtf8(protocol::orderStatus::kWaitSettlement)) {
            statusColor = QColor(QStringLiteral("#A66B00"));
        } else if (order.status == QString::fromUtf8(protocol::orderStatus::kFinished)) {
            statusColor = QColor(QStringLiteral("#14865A"));
        }
        const QColor paymentColor = order.paymentStatus == QStringLiteral("UNPAID")
            ? QColor(QStringLiteral("#C43742"))
            : order.paymentStatus == QStringLiteral("PAID")
            ? QColor(QStringLiteral("#14865A")) : QColor(QStringLiteral("#718399"));
        row.styles.append({3, statusColor, {}, true});
        row.styles.append({4, paymentColor, {}, true});
        rows.append(row);
    }
    table_->setRows(rows);
    if (rows.isEmpty()) {
        table_->setState(EntityTableView::State::Empty,
                         QStringLiteral("当前服务端筛选条件下没有订单"));
    }
    total_ = page.total;
    queryLabel_->setText(QStringLiteral("匹配 %1 笔订单").arg(page.total));
    const int pageCount = qMax(1, static_cast<int>((page.total + page.pageSize - 1) / page.pageSize));
    pageLabel_->setText(QStringLiteral("第 %1 / %2 页").arg(page.page).arg(pageCount));
    previousButton_->setEnabled(page.page > 1);
    nextButton_->setEnabled(page.page < pageCount);
    updatedLabel_->setText(QStringLiteral("数据时间 %1 · 充电中金额为服务端预估")
                               .arg(page.formattedGeneratedAt()));
    totalValue_->setText(QString::number(page.platformSummary.totalOrders));
    reservedValue_->setText(QString::number(page.platformSummary.reservedCount));
    chargingValue_->setText(QString::number(page.platformSummary.chargingCount));
    unpaidValue_->setText(QString::number(page.platformSummary.waitSettlementCount));
    paidAmountValue_->setText(QStringLiteral("¥ %1").arg(page.platformSummary.paidAmount, 0, 'f', 2));
    showOrderDetail(nullptr);
    if (!rows.isEmpty()) table_->tableView()->selectRow(0);
}

void OrderManagementPage::updateSelection(const QModelIndex& current)
{
    const qint64 id = table_->entityIdAt(current).toLongLong();
    const auto iterator = ordersById_.constFind(id);
    showOrderDetail(iterator == ordersById_.cend() ? nullptr : &iterator.value());
}

void OrderManagementPage::showOrderDetail(const AdminOrder* order)
{
    if (!order) {
        detailTitle_->setText(QStringLiteral("订单详情"));
        detailUser_->setText(QStringLiteral("请选择一笔订单查看完整业务链路"));
        detailDevice_->setText(QStringLiteral("—"));
        detailBilling_->setText(QStringLiteral("—"));
        detailTimes_->setText(QStringLiteral("—"));
        return;
    }
    detailTitle_->setText(QStringLiteral("订单 #%1 · %2")
                              .arg(order->orderId).arg(order->statusLabel()));
    detailUser_->setText(QStringLiteral("用户\n%1（ID %2）")
                             .arg(order->userLabel()).arg(order->userId));
    detailDevice_->setText(
        QStringLiteral("设备\n%1（站点 ID %2 / 桩 ID %3）\n%4 · %5 kW · %6")
            .arg(order->deviceLabel()).arg(order->stationId).arg(order->chargerId)
            .arg(order->chargerType == protocol::ChargerTypeFast
                     ? QStringLiteral("快充") : QStringLiteral("慢充"))
            .arg(order->powerKw, 0, 'f', 1).arg(order->formattedPrice()));
    detailBilling_->setText(
        QStringLiteral("计费\n%1 · %2\n%3 · %4\n支付：%5")
            .arg(order->billingAmountLabel(), order->formattedAmount(),
                 order->formattedEnergy(), order->formattedDuration(),
                 order->paymentStatusLabel()));
    detailTimes_->setText(
        QStringLiteral("生命周期\n下单：%1\n开始：%2\n停止：%3\n结算：%4")
            .arg(order->formattedCreatedAt(), order->formattedStartTime(),
                 order->formattedStopTime(), order->formattedSettleTime()));
}

void OrderManagementPage::setLoading(bool loading)
{
    keywordEdit_->setEnabled(!loading);
    statusFilter_->setEnabled(!loading);
    paymentFilter_->setEnabled(!loading);
    searchButton_->setEnabled(!loading);
    resetButton_->setEnabled(!loading);
    refreshButton_->setEnabled(!loading);
    if (loading) {
        previousButton_->setEnabled(false);
        nextButton_->setEnabled(false);
    }
}
