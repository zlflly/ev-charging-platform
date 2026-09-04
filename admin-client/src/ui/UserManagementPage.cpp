#include "ui/UserManagementPage.h"

#include "api/AdminApiClient.h"
#include "protocol/Protocol.h"
#include "ui/widgets/EntityTableView.h"

#include <QColor>
#include <QComboBox>
#include <QDialog>
#include <QFrame>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QStyle>
#include <QTableView>
#include <QVBoxLayout>

namespace {

bool confirmActiveFreeze(QWidget* parent, const AdminUser& user)
{
    if (user.status != protocol::UserStatusNormal || !user.activeOrder) return true;

    QDialog dialog(parent);
    dialog.setObjectName(QStringLiteral("operationDialog"));
    dialog.setWindowTitle(QStringLiteral("未完成订单风险确认"));
    dialog.setModal(true);
    dialog.setMinimumWidth(560);
    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(28, 24, 28, 24);
    layout->setSpacing(14);

    auto* eyebrow = new QLabel(QStringLiteral("ACTIVE ORDER WARNING"), &dialog);
    eyebrow->setObjectName(QStringLiteral("dialogEyebrow"));
    auto* title = new QLabel(QStringLiteral("该用户有未完成订单"), &dialog);
    title->setObjectName(QStringLiteral("dialogTitle"));
    auto* target = new QLabel(
        QStringLiteral("%1 · %2（用户 ID %3）")
            .arg(user.nickname, user.phone).arg(user.userId), &dialog);
    target->setObjectName(QStringLiteral("dialogTarget"));
    auto* details = new QLabel(
        QStringLiteral("业务阶段：%1\n订单编号：%2\n关联设备：%3\n充电桩 ID：%4")
            .arg(user.activeOrder->statusLabel())
            .arg(user.activeOrder->orderId)
            .arg(user.activeOrder->deviceLabel())
            .arg(user.activeOrder->chargerId), &dialog);
    details->setObjectName(QStringLiteral("dialogDetails"));
    details->setWordWrap(true);
    const QString explanation = user.activeOrder->status
            == QString::fromUtf8(protocol::orderStatus::kWaitSettlement)
        ? QStringLiteral("“待支付”表示充电已经停止、账单已由服务端生成，但扣款和订单完结尚未成功。")
        : QStringLiteral("用户仍处于预约或充电流程，冻结可能使订单无法继续完成。");
    auto* warning = new QLabel(
        explanation + QStringLiteral(
            "\n\n继续操作只会把请求提交给服务端重新核验，不代表一定冻结成功；"
            "服务端拒绝时，管理端不会绕过保护或修改本地状态。"), &dialog);
    warning->setObjectName(QStringLiteral("dialogWarning"));
    warning->setWordWrap(true);

    auto* buttons = new QHBoxLayout;
    buttons->addStretch();
    auto* cancel = new QPushButton(QStringLiteral("取消冻结"), &dialog);
    cancel->setObjectName(QStringLiteral("secondaryButton"));
    auto* proceed = new QPushButton(QStringLiteral("继续填写原因"), &dialog);
    proceed->setObjectName(QStringLiteral("dangerButton"));
    QObject::connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    QObject::connect(proceed, &QPushButton::clicked, &dialog, &QDialog::accept);
    buttons->addWidget(cancel);
    buttons->addWidget(proceed);

    layout->addWidget(eyebrow);
    layout->addWidget(title);
    layout->addWidget(target);
    layout->addWidget(details);
    layout->addWidget(warning);
    layout->addLayout(buttons);
    return dialog.exec() == QDialog::Accepted;
}

void showStatusResult(QWidget* parent, bool success, const QString& title,
                      const QString& message)
{
    QDialog dialog(parent);
    dialog.setObjectName(QStringLiteral("operationDialog"));
    dialog.setWindowTitle(title);
    dialog.setModal(true);
    dialog.setMinimumWidth(500);
    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(28, 24, 28, 24);
    layout->setSpacing(14);
    auto* heading = new QLabel(title, &dialog);
    heading->setObjectName(QStringLiteral("dialogTitle"));
    auto* body = new QLabel(message, &dialog);
    body->setObjectName(success ? QStringLiteral("dialogText")
                                : QStringLiteral("dialogWarning"));
    body->setWordWrap(true);
    auto* buttons = new QHBoxLayout;
    buttons->addStretch();
    auto* close = new QPushButton(QStringLiteral("知道了"), &dialog);
    close->setObjectName(success ? QStringLiteral("primaryButton")
                                 : QStringLiteral("secondaryButton"));
    QObject::connect(close, &QPushButton::clicked, &dialog, &QDialog::accept);
    buttons->addWidget(close);
    layout->addWidget(heading);
    layout->addWidget(body);
    layout->addLayout(buttons);
    dialog.exec();
}

bool promptReason(QWidget* parent, const AdminUser& user, QString* reason)
{
    if (!reason) return false;
    const bool freezing = user.status == protocol::UserStatusNormal;
    QDialog dialog(parent);
    dialog.setObjectName(QStringLiteral("operationDialog"));
    dialog.setWindowTitle(freezing ? QStringLiteral("冻结用户")
                                   : QStringLiteral("解冻用户"));
    dialog.setModal(true);
    dialog.setMinimumWidth(520);
    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(28, 24, 28, 24);
    layout->setSpacing(14);
    auto* eyebrow = new QLabel(QStringLiteral("ACCOUNT RISK CONTROL"), &dialog);
    eyebrow->setObjectName(QStringLiteral("dialogEyebrow"));
    auto* title = new QLabel(
        freezing ? QStringLiteral("确认冻结该账号？")
                 : QStringLiteral("确认恢复该账号？"), &dialog);
    title->setObjectName(QStringLiteral("dialogTitle"));
    auto* description = new QLabel(
        QStringLiteral("用户 %1 · %2（ID %3）\n%4")
            .arg(user.nickname, user.phone)
            .arg(user.userId)
            .arg(freezing && user.activeOrder
                ? QStringLiteral("当前业务：%1，订单 %2，设备 %3（ID %4）。\n服务端将重新核验活跃订单并可能拒绝冻结，避免订单无法停止或结算。")
                      .arg(user.activeOrder->statusLabel())
                      .arg(user.activeOrder->orderId)
                      .arg(user.activeOrder->deviceLabel())
                      .arg(user.activeOrder->chargerId)
                : freezing
                ? QStringLiteral("当前没有活跃订单；服务端仍会在写入时重新核验最新状态。")
                : QStringLiteral("解冻成功后，用户仍需重新发起业务请求取得最新状态。")),
        &dialog);
    description->setObjectName(QStringLiteral("dialogText"));
    description->setWordWrap(true);
    auto* reasonEdit = new QLineEdit(&dialog);
    reasonEdit->setMaxLength(200);
    reasonEdit->setPlaceholderText(
        freezing ? QStringLiteral("请输入冻结原因（2～200 字）")
                 : QStringLiteral("请输入解冻原因（2～200 字）"));
    auto* error = new QLabel(&dialog);
    error->setObjectName(QStringLiteral("formError"));
    error->hide();
    auto* buttons = new QHBoxLayout;
    buttons->addStretch();
    auto* cancel = new QPushButton(QStringLiteral("取消"), &dialog);
    cancel->setObjectName(QStringLiteral("secondaryButton"));
    auto* submit = new QPushButton(
        freezing ? QStringLiteral("确认冻结") : QStringLiteral("确认解冻"), &dialog);
    submit->setObjectName(freezing ? QStringLiteral("dangerButton")
                                   : QStringLiteral("primaryButton"));
    QObject::connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    QObject::connect(submit, &QPushButton::clicked, &dialog, [&] {
        const QString normalized = reasonEdit->text().trimmed();
        if (normalized.size() < 2) {
            error->setText(QStringLiteral("操作原因至少需要 2 个字符"));
            error->show();
            return;
        }
        *reason = normalized;
        dialog.accept();
    });
    buttons->addWidget(cancel);
    buttons->addWidget(submit);
    layout->addWidget(eyebrow);
    layout->addWidget(title);
    layout->addWidget(description);
    if (freezing && user.activeOrder) {
        auto* warning = new QLabel(
            QStringLiteral("该用户存在未完成订单。若服务端拒绝，请先完成订单；客户端不会绕过此保护。"),
            &dialog);
        warning->setObjectName(QStringLiteral("dialogWarning"));
        warning->setWordWrap(true);
        layout->addWidget(warning);
    }
    layout->addWidget(reasonEdit);
    layout->addWidget(error);
    layout->addLayout(buttons);
    reasonEdit->setFocus();
    return dialog.exec() == QDialog::Accepted;
}

} // namespace

UserManagementPage::UserManagementPage(AdminApiClient* api, QWidget* parent)
    : QWidget(parent), api_(api)
{
    Q_ASSERT(api_);
    setObjectName(QStringLiteral("pageRoot"));
    query_.pageSize = 20;

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 24);
    layout->setSpacing(14);

    auto* summary = new QFrame(this);
    summary->setObjectName(QStringLiteral("sectionCard"));
    auto* summaryLayout = new QHBoxLayout(summary);
    summaryLayout->setContentsMargins(16, 12, 16, 12);
    auto* copy = new QVBoxLayout;
    auto* title = new QLabel(QStringLiteral("用户风险控制台"), summary);
    title->setObjectName(QStringLiteral("sectionTitle"));
    auto* hint = new QLabel(
        QStringLiteral("列表与筛选均来自服务端 · 待支付表示充电已停止、等待扣款"), summary);
    hint->setObjectName(QStringLiteral("muted"));
    copy->addWidget(title);
    copy->addWidget(hint);
    totalLabel_ = new QLabel(QStringLiteral("匹配 -- 位用户"), summary);
    totalLabel_->setObjectName(QStringLiteral("metricChip"));
    refreshButton_ = new QPushButton(QStringLiteral("刷新当前查询"), summary);
    refreshButton_->setObjectName(QStringLiteral("secondaryButton"));
    summaryLayout->addLayout(copy);
    summaryLayout->addStretch();
    summaryLayout->addWidget(totalLabel_);
    summaryLayout->addWidget(refreshButton_);
    layout->addWidget(summary);

    auto* filter = new QFrame(this);
    filter->setObjectName(QStringLiteral("sectionCard"));
    auto* filterLayout = new QHBoxLayout(filter);
    filterLayout->setContentsMargins(16, 12, 16, 12);
    phoneEdit_ = new QLineEdit(filter);
    phoneEdit_->setPlaceholderText(QStringLiteral("输入手机号中的关键数字"));
    phoneEdit_->setMaxLength(11);
    phoneEdit_->setClearButtonEnabled(true);
    phoneEdit_->setValidator(new QRegularExpressionValidator(
        QRegularExpression(QStringLiteral("\\d{0,11}")), phoneEdit_));
    statusFilter_ = new QComboBox(filter);
    statusFilter_->addItem(QStringLiteral("全部状态"), -1);
    statusFilter_->addItem(QStringLiteral("正常"), protocol::UserStatusNormal);
    statusFilter_->addItem(QStringLiteral("已冻结"), protocol::UserStatusFrozen);
    activityFilter_ = new QComboBox(filter);
    activityFilter_->addItem(QStringLiteral("全部业务状态"), QStringLiteral("ALL"));
    activityFilter_->addItem(QStringLiteral("有未完成订单"), QStringLiteral("ACTIVE"));
    activityFilter_->addItem(QStringLiteral("当前无订单"), QStringLiteral("IDLE"));
    searchButton_ = new QPushButton(QStringLiteral("查询"), filter);
    searchButton_->setObjectName(QStringLiteral("primaryButton"));
    resetButton_ = new QPushButton(QStringLiteral("重置"), filter);
    resetButton_->setObjectName(QStringLiteral("secondaryButton"));
    filterLayout->addWidget(phoneEdit_, 1);
    filterLayout->addWidget(statusFilter_);
    filterLayout->addWidget(activityFilter_);
    filterLayout->addWidget(searchButton_);
    filterLayout->addWidget(resetButton_);
    layout->addWidget(filter);

    auto* card = new QFrame(this);
    card->setObjectName(QStringLiteral("sectionCard"));
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(16, 14, 16, 14);
    cardLayout->setSpacing(10);
    table_ = new EntityTableView(
        {QStringLiteral("用户 ID"), QStringLiteral("手机号"), QStringLiteral("昵称"),
         QStringLiteral("余额"), QStringLiteral("账号状态"), QStringLiteral("业务状态"),
         QStringLiteral("关联站点 / 充电桩"), QStringLiteral("注册时间")}, card);
    table_->tableView()->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    table_->tableView()->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch);
    table_->tableView()->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
    cardLayout->addWidget(table_, 1);
    auto* footer = new QHBoxLayout;
    selectionLabel_ = new QLabel(QStringLiteral("请选择一个用户"), card);
    selectionLabel_->setObjectName(QStringLiteral("muted"));
    previousButton_ = new QPushButton(QStringLiteral("上一页"), card);
    previousButton_->setObjectName(QStringLiteral("secondaryButton"));
    pageLabel_ = new QLabel(QStringLiteral("第 1 页"), card);
    pageLabel_->setObjectName(QStringLiteral("muted"));
    nextButton_ = new QPushButton(QStringLiteral("下一页"), card);
    nextButton_->setObjectName(QStringLiteral("secondaryButton"));
    statusButton_ = new QPushButton(QStringLiteral("冻结 / 解冻"), card);
    statusButton_->setObjectName(QStringLiteral("dangerButton"));
    statusButton_->setEnabled(false);
    footer->addWidget(selectionLabel_, 1);
    footer->addWidget(previousButton_);
    footer->addWidget(pageLabel_);
    footer->addWidget(nextButton_);
    footer->addSpacing(12);
    footer->addWidget(statusButton_);
    cardLayout->addLayout(footer);
    layout->addWidget(card, 1);

    connect(searchButton_, &QPushButton::clicked, this, &UserManagementPage::search);
    connect(phoneEdit_, &QLineEdit::returnPressed, this, &UserManagementPage::search);
    connect(resetButton_, &QPushButton::clicked, this, &UserManagementPage::resetSearch);
    connect(refreshButton_, &QPushButton::clicked, this, &UserManagementPage::refresh);
    connect(previousButton_, &QPushButton::clicked, this, [this] { loadPage(query_.page - 1); });
    connect(nextButton_, &QPushButton::clicked, this, [this] { loadPage(query_.page + 1); });
    connect(statusButton_, &QPushButton::clicked,
            this, &UserManagementPage::changeSelectedUserStatus);
    connect(table_, &EntityTableView::retryRequested, this, &UserManagementPage::refresh);
    connect(table_->tableView()->selectionModel(),
            &QItemSelectionModel::currentRowChanged,
            this, &UserManagementPage::updateSelection);
}

void UserManagementPage::refresh() { loadPage(query_.page); }

void UserManagementPage::search()
{
    UserListQuery next = query_;
    next.page = 1;
    next.phoneKeyword = phoneEdit_->text();
    next.status = statusFilter_->currentData().toInt();
    next.activityFilter = activityFilter_->currentData().toString();
    QString error;
    if (!next.validate(&error)) {
        QMessageBox::warning(this, QStringLiteral("查询条件无效"), error);
        return;
    }
    query_ = next;
    loadPage(1);
}

void UserManagementPage::resetSearch()
{
    phoneEdit_->clear();
    statusFilter_->setCurrentIndex(0);
    activityFilter_->setCurrentIndex(0);
    query_.phoneKeyword.clear();
    query_.status = -1;
    query_.activityFilter = QStringLiteral("ALL");
    query_.page = 1;
    loadPage(1);
}

void UserManagementPage::loadPage(int page)
{
    if (page < 1 || api_->isUserListInFlight() || api_->isUserStatusUpdateInFlight()) return;
    UserListQuery requested = query_;
    requested.page = page;
    setLoading(true);
    table_->setState(EntityTableView::State::Loading,
                     QStringLiteral("正在从服务器查询用户…"));
    const bool started = api_->requestUsers(
        requested, [this, requested](std::optional<UserListPage> result,
                                    const QString& error) {
            setLoading(false);
            if (!result) {
                usersById_.clear();
                selectionLabel_->setText(QStringLiteral("列表不可用，请重新加载"));
                statusButton_->setEnabled(false);
                previousButton_->setEnabled(false);
                nextButton_->setEnabled(false);
                table_->setState(EntityTableView::State::Error, error);
                return;
            }
            query_ = requested;
            if (result->users.isEmpty() && result->total > 0
                && result->page > 1) {
                loadPage(result->page - 1);
                return;
            }
            showPage(*result);
        });
    if (!started) setLoading(false);
}

void UserManagementPage::showPage(const UserListPage& page)
{
    usersById_.clear();
    QList<EntityTableView::Row> rows;
    for (const AdminUser& user : page.users) {
        usersById_.insert(user.userId, user);
        EntityTableView::Row row;
        row.entityId = user.userId;
        row.cells = {QString::number(user.userId), user.phone, user.nickname,
                     user.formattedBalance(), user.statusLabel(),
                     user.activityLabel(), user.currentDeviceLabel(),
                     user.formattedCreatedAt()};
        row.sortValues = {user.userId, user.phone, user.nickname, user.balance,
                          user.status, user.activityStatus,
                          user.currentDeviceLabel(), user.createdAtEpochMs};
        const QColor accountColor = user.status == protocol::UserStatusFrozen
            ? QColor(QStringLiteral("#C43742")) : QColor(QStringLiteral("#14865A"));
        const QColor activityColor = user.activeOrder
            ? QColor(QStringLiteral("#1769E8")) : QColor(QStringLiteral("#718399"));
        row.styles.append({4, accountColor, {}, true});
        row.styles.append({5, activityColor, {}, true});
        rows.append(row);
    }
    total_ = page.total;
    table_->setRows(rows);
    if (rows.isEmpty()) {
        table_->setState(EntityTableView::State::Empty,
                         QStringLiteral("当前服务端查询条件下没有用户"));
    }
    totalLabel_->setText(QStringLiteral("匹配 %1 位用户").arg(total_));
    const qint64 pageCount = qMax<qint64>(1, (total_ + query_.pageSize - 1) / query_.pageSize);
    pageLabel_->setText(QStringLiteral("第 %1 / %2 页").arg(query_.page).arg(pageCount));
    previousButton_->setEnabled(query_.page > 1);
    nextButton_->setEnabled(static_cast<qint64>(query_.page) * query_.pageSize < total_);
    selectionLabel_->setText(QStringLiteral("请选择一个用户"));
    statusButton_->setText(QStringLiteral("冻结 / 解冻"));
    statusButton_->setEnabled(false);
}

void UserManagementPage::updateSelection(const QModelIndex& current)
{
    const qint64 userId = table_->entityIdAt(current).toLongLong();
    const auto found = usersById_.constFind(userId);
    if (found == usersById_.cend()) {
        selectionLabel_->setText(QStringLiteral("请选择一个用户"));
        statusButton_->setEnabled(false);
        return;
    }
    const AdminUser& user = found.value();
    selectionLabel_->setText(user.activeOrder
        ? QStringLiteral("已选择：%1 · %2（ID %3）｜%4 · 订单 %5 · %6")
              .arg(user.nickname, user.phone).arg(user.userId)
              .arg(user.activeOrder->statusLabel())
              .arg(user.activeOrder->orderId)
              .arg(user.activeOrder->deviceLabel())
        : QStringLiteral("已选择：%1 · %2（ID %3）｜当前未使用")
              .arg(user.nickname, user.phone).arg(user.userId));
    const bool freezing = user.status == protocol::UserStatusNormal;
    statusButton_->setText(freezing ? QStringLiteral("冻结用户")
                                    : QStringLiteral("解冻用户"));
    statusButton_->setObjectName(freezing ? QStringLiteral("dangerButton")
                                          : QStringLiteral("primaryButton"));
    statusButton_->style()->unpolish(statusButton_);
    statusButton_->style()->polish(statusButton_);
    statusButton_->setEnabled(!api_->isUserStatusUpdateInFlight());
}

void UserManagementPage::changeSelectedUserStatus()
{
    const qint64 userId = selectedUserId();
    const auto found = usersById_.constFind(userId);
    if (found == usersById_.cend()) return;
    const AdminUser user = found.value();
    if (!confirmActiveFreeze(this, user)) return;
    QString reason;
    if (!promptReason(this, user, &reason)) return;
    UserStatusUpdateRequest request;
    request.userId = user.userId;
    request.expectedStatus = user.status;
    request.targetStatus = user.status == protocol::UserStatusNormal
        ? protocol::UserStatusFrozen : protocol::UserStatusNormal;
    request.reason = reason;
    setLoading(true);
    const bool started = api_->updateUserStatus(
        request, [this, request](std::optional<UserStatusUpdateResult> result,
                                const QString& error) {
            setLoading(false);
            if (!result) {
                showStatusResult(
                    this, false, QStringLiteral("状态变更未执行"),
                    QStringLiteral("服务端未接受本次状态变更：\n%1\n\n"
                                   "账号状态没有在客户端本地修改，列表将重新查询。")
                        .arg(error));
                // 拒绝也可能源于并发状态变化，因此重新查询当前页。
                loadPage(query_.page);
                return;
            }
            showStatusResult(
                this, true, QStringLiteral("操作成功"),
                result->status == protocol::UserStatusFrozen
                    ? QStringLiteral("用户已冻结。请使用用户端验证受限业务返回 1002。")
                    : QStringLiteral("用户已解冻，列表将从服务端重新加载。"));
            loadPage(query_.page);
        });
    if (!started) setLoading(false);
}

qint64 UserManagementPage::selectedUserId() const
{
    return table_->entityIdAt(table_->tableView()->currentIndex()).toLongLong();
}

void UserManagementPage::setLoading(bool loading)
{
    phoneEdit_->setEnabled(!loading);
    statusFilter_->setEnabled(!loading);
    activityFilter_->setEnabled(!loading);
    searchButton_->setEnabled(!loading);
    resetButton_->setEnabled(!loading);
    refreshButton_->setEnabled(!loading);
    previousButton_->setEnabled(!loading && query_.page > 1);
    nextButton_->setEnabled(!loading
        && static_cast<qint64>(query_.page) * query_.pageSize < total_);
    statusButton_->setEnabled(!loading && usersById_.contains(selectedUserId()));
}
