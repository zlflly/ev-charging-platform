#include "ui/OrderPage.h"

#include "net/NetworkClient.h"
#include "protocol/Protocol.h"
#include "ui/theme/Theme.h"

#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QVBoxLayout>

namespace {

QString cssColor(const QColor& color)
{
    return color.name(QColor::HexRgb);
}

QString cssRgba(const QColor& color, int alpha)
{
    return QStringLiteral("rgba(%1,%2,%3,%4)")
        .arg(color.red()).arg(color.green()).arg(color.blue()).arg(alpha);
}

QPixmap tintedSvgPixmap(const QString& resourcePath, const QSize& size,
                        const QColor& color)
{
    const QPixmap source = QIcon(resourcePath).pixmap(size);
    if (source.isNull()) return {};
    QPixmap tinted(size);
    tinted.fill(color);
    QPainter painter(&tinted);
    painter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    painter.drawPixmap(0, 0, source);
    return tinted;
}

class BackButton final : public QPushButton
{
public:
    explicit BackButton(QWidget* parent = nullptr) : QPushButton(parent)
    {
        setCursor(Qt::PointingHandCursor);
        setStyleSheet(QStringLiteral("background: transparent; border: none;"));
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QPixmap icon = tintedSvgPixmap(
            QStringLiteral(":/resources/icons/back.svg"), QSize(25, 25),
            theme::textPrimary());
        if (!icon.isNull()) {
            painter.drawPixmap((width() - icon.width()) / 2,
                               (height() - icon.height()) / 2, icon);
        }
    }
};

QString smallButtonStyle()
{
    return QString(
        "QPushButton { background: %1; color: %2; border: 1px solid %3;"
        " border-radius: 10px; padding: 0 10px; }"
        "QPushButton:hover { border-color: %4; }"
        "QPushButton:pressed { background: %5; }")
        .arg(cssRgba(theme::cardFill(), 230), cssColor(theme::textPrimary()),
             cssColor(theme::cardBorder()), cssColor(theme::primaryBlue()),
             cssRgba(theme::primaryBlue(), 48));
}

QString activeButtonStyle()
{
    return QString(
        "QPushButton { background: %1; color: white; border: 1px solid %2;"
        " border-radius: 17px; } QPushButton:hover { background: %3; }")
        .arg(cssRgba(theme::primaryBlue(), 220), cssColor(theme::primaryBlue()),
             cssColor(theme::primaryBlueHover()));
}

QString cardStyle()
{
    return QString(
        "QFrame { background: %1; border: 1px solid %2; border-radius: 15px; }")
        .arg(cssRgba(theme::cardFill(), 235), cssColor(theme::cardBorder()));
}

} // namespace

OrderPage::OrderPage(NetworkClient* networkClient, QWidget* parent)
    : QWidget(parent)
    , networkClient_(networkClient)
{
    setObjectName(QStringLiteral("OrderPage"));
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QStringLiteral("#OrderPage { background-color: %1; }")
                      .arg(cssColor(theme::background())));
    setMinimumSize(360, theme::loginCanvasHeight);
    buildUi();
}

void OrderPage::buildUi()
{
    backButton_ = new BackButton(this);
    titleLabel_ = new QLabel(QStringLiteral("我的订单"), this);
    titleLabel_->setFont(theme::buttonFont());
    titleLabel_->setAlignment(Qt::AlignCenter);
    titleLabel_->setStyleSheet(QStringLiteral("color: %1; background: transparent;")
                                   .arg(cssColor(theme::textPrimary())));

    summaryCard_ = new QFrame(this);
    summaryCard_->setStyleSheet(cardStyle());
    totalLabel_ = new QLabel(summaryCard_);
    activeLabel_ = new QLabel(summaryCard_);
    pendingLabel_ = new QLabel(summaryCard_);
    spendLabel_ = new QLabel(summaryCard_);
    const QList<QPair<QLabel*, QString>> summaryLabels = {
        {totalLabel_, QStringLiteral("全部订单")},
        {activeLabel_, QStringLiteral("进行中")},
        {pendingLabel_, QStringLiteral("待结算")},
        {spendLabel_, QStringLiteral("累计消费")},
    };
    for (const auto& item : summaryLabels) {
        item.first->setTextFormat(Qt::RichText);
        item.first->setAlignment(Qt::AlignCenter);
        item.first->setProperty("summaryTitle", item.second);
        item.first->setStyleSheet(QStringLiteral("background: transparent;"));
    }

    for (const QString& text : {QStringLiteral("全部"), QStringLiteral("进行中"),
                                QStringLiteral("待结算"), QStringLiteral("已完成"),
                                QStringLiteral("已取消")}) {
        auto* button = new QPushButton(text, this);
        button->setCheckable(true);
        button->setFont(theme::footerFont());
        button->setCursor(Qt::PointingHandCursor);
        filterButtons_.append(button);
        connect(button, &QPushButton::clicked, this, &OrderPage::onFilterClicked);
    }

    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText(QStringLiteral("搜索站点名或订单号"));
    searchEdit_->setFont(theme::footerFont());
    searchEdit_->setStyleSheet(QString(
        "QLineEdit { background: %1; border: 1px solid %2; border-radius: 12px;"
        " padding: 0 13px; color: %3; } QLineEdit:focus { border-color: %4; }"
        " QLineEdit::placeholder { color: %5; }")
        .arg(cssColor(theme::inputFill()), cssColor(theme::inputBorder()),
             cssColor(theme::textPrimary()), cssColor(theme::primaryBlue()),
             cssColor(theme::textMuted())));
    connect(searchEdit_, &QLineEdit::textChanged,
            this, &OrderPage::onSearchChanged);

    orderScroll_ = new QScrollArea(this);
    orderScroll_->setWidgetResizable(true);
    orderScroll_->setFrameShape(QFrame::NoFrame);
    orderScroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    orderScroll_->setStyleSheet(QStringLiteral("background: transparent; border: none;"));
    orderContainer_ = new QWidget();
    orderContainer_->setStyleSheet(QStringLiteral("background: transparent;"));
    orderScroll_->setWidget(orderContainer_);
    stateLabel_ = new QLabel(orderContainer_);
    stateLabel_->setAlignment(Qt::AlignCenter);
    stateLabel_->setWordWrap(true);
    stateLabel_->setFont(theme::inputFont());
    stateLabel_->setStyleSheet(QStringLiteral("color: %1; background: transparent;")
                                   .arg(cssColor(theme::textSecondary())));

    connect(backButton_, &QPushButton::clicked, this, &OrderPage::backRequested);
    filterButtons_.first()->setChecked(true);
    for (int i = 0; i < filterButtons_.size(); ++i) {
        filterButtons_[i]->setStyleSheet(i == 0 ? activeButtonStyle()
                                                : smallButtonStyle());
    }
}

void OrderPage::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.fillRect(rect(), theme::background());
}

void OrderPage::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    layoutUi();
}

void OrderPage::layoutUi()
{
    const qreal sx = static_cast<qreal>(width()) / theme::loginCanvasWidth;
    const qreal sy = static_cast<qreal>(height()) / theme::loginCanvasHeight;
    auto X = [sx](qreal value) { return qRound(value * sx); };
    auto Y = [sy](qreal value) { return qRound(value * sy); };
    const int side = X(17);
    const int contentWidth = width() - side * 2;

    backButton_->setGeometry(X(12), Y(14), X(36), Y(34));
    titleLabel_->setGeometry(X(95), Y(14), X(280), Y(34));
    summaryCard_->setGeometry(side, Y(62), contentWidth, Y(91));
    const int cellWidth = summaryCard_->width() / 4;
    const QList<QLabel*> labels = {totalLabel_, activeLabel_, pendingLabel_, spendLabel_};
    for (int i = 0; i < labels.size(); ++i) {
        labels[i]->setGeometry(i * cellWidth, Y(11),
                               i == labels.size() - 1
                                   ? summaryCard_->width() - i * cellWidth : cellWidth,
                               Y(67));
    }
    const int filterGap = X(7);
    const int filterWidth = (contentWidth - filterGap * 4) / 5;
    for (int i = 0; i < filterButtons_.size(); ++i) {
        filterButtons_[i]->setGeometry(side + i * (filterWidth + filterGap), Y(164),
                                        i == filterButtons_.size() - 1
                                            ? contentWidth - i * (filterWidth + filterGap)
                                            : filterWidth,
                                        Y(33));
    }
    searchEdit_->setGeometry(side, Y(207), contentWidth, Y(38));
    orderScroll_->setGeometry(side, Y(255), contentWidth, Y(554));
    stateLabel_->setGeometry(X(12), Y(18), orderContainer_->width() - X(24), Y(80));
}

void OrderPage::reload()
{
    orders_.clear();
    summary_ = {};
    loading_ = true;
    showState(QStringLiteral("正在加载订单..."));
    if (!networkClient_) {
        loading_ = false;
        showState(QStringLiteral("网络模块未初始化"), true);
        return;
    }
    networkClient_->sendRequest(QString::fromUtf8(protocol::action::kOrderHistory), {},
        [this](const protocol::Response& response) {
            loading_ = false;
            if (!response.isOk()) {
                showState(protocol::describeError(response.code, response.message), true);
                return;
            }
            summary_ = response.data.value(QStringLiteral("summary")).toObject();
            for (const QJsonValue& value : response.data
                     .value(QStringLiteral("orders")).toArray()) {
                const OrderInfo order = OrderInfo::fromJson(value.toObject());
                if (order.valid()) orders_.append(order);
            }
            renderSummary(summary_);
            renderOrders();
        });
}

void OrderPage::renderSummary(const QJsonObject& summary)
{
    const QColor valueColor = theme::textPrimary();
    const auto text = [this, valueColor](QLabel* label, const QString& title,
                                         const QString& value) {
        label->setText(QStringLiteral(
            "<span style='color:%1; font-size:12px;'>%2</span><br>"
            "<span style='color:%3; font-size:21px; font-weight:700;'>%4</span>")
            .arg(cssColor(theme::textSecondary()), title, cssColor(valueColor), value));
    };
    text(totalLabel_, QStringLiteral("全部订单"),
         QString::number(summary.value(QStringLiteral("total")).toInt()));
    text(activeLabel_, QStringLiteral("进行中"),
         QString::number(summary.value(QStringLiteral("charging")).toInt()));
    text(pendingLabel_, QStringLiteral("待结算"),
         QString::number(summary.value(QStringLiteral("pending")).toInt()));
    text(spendLabel_, QStringLiteral("累计消费"),
         QStringLiteral("¥%1").arg(summary.value(QStringLiteral("totalSpend")).toDouble(),
                                     0, 'f', 2));
}

bool OrderPage::matchesFilter(const OrderInfo& order) const
{
    const OrderInfo::Status status = order.statusEnum();
    if (filterMode_ == 1 && status != OrderInfo::StatusReserved &&
        status != OrderInfo::StatusCharging) return false;
    if (filterMode_ == 2 && status != OrderInfo::StatusWaitSettlement) return false;
    if (filterMode_ == 3 && status != OrderInfo::StatusFinished) return false;
    if (filterMode_ == 4 && status != OrderInfo::StatusCancelled) return false;
    if (searchText_.isEmpty()) return true;
    return order.stationName.contains(searchText_, Qt::CaseInsensitive) ||
           QString::number(order.orderId).contains(searchText_, Qt::CaseInsensitive);
}

QString OrderPage::statusText(const OrderInfo& order) const
{
    switch (order.statusEnum()) {
    case OrderInfo::StatusReserved: return QStringLiteral("待开始");
    case OrderInfo::StatusCharging: return QStringLiteral("充电中");
    case OrderInfo::StatusWaitSettlement: return QStringLiteral("待结算");
    case OrderInfo::StatusFinished: return QStringLiteral("已完成");
    case OrderInfo::StatusCancelled: return QStringLiteral("已取消");
    default: return QStringLiteral("未知状态");
    }
}

QString OrderPage::formatDate(qint64 timestampMs) const
{
    if (timestampMs <= 0) return QStringLiteral("时间未知");
    return QDateTime::fromMSecsSinceEpoch(timestampMs).toString(QStringLiteral("MM-dd hh:mm"));
}

QString OrderPage::formatDuration(qint64 milliseconds) const
{
    const qint64 seconds = qMax<qint64>(0, milliseconds) / 1000;
    return QStringLiteral("%1:%2:%3")
        .arg(seconds / 3600)
        .arg((seconds % 3600) / 60, 2, 10, QLatin1Char('0'))
        .arg(seconds % 60, 2, 10, QLatin1Char('0'));
}

void OrderPage::renderOrders()
{
    QLayout* oldLayout = orderContainer_->layout();
    if (oldLayout) {
        while (QLayoutItem* item = oldLayout->takeAt(0)) {
            if (QWidget* widget = item->widget()) widget->deleteLater();
            delete item;
        }
        delete oldLayout;
    }
    auto* list = new QVBoxLayout(orderContainer_);
    list->setContentsMargins(0, 0, 0, 12);
    list->setSpacing(9);

    int visibleCount = 0;
    for (const OrderInfo& order : orders_) {
        if (!matchesFilter(order)) continue;
        ++visibleCount;
        auto* card = new QFrame(orderContainer_);
        card->setStyleSheet(cardStyle());
        card->setMinimumHeight(105);
        auto* root = new QVBoxLayout(card);
        root->setContentsMargins(14, 10, 14, 10);
        root->setSpacing(4);

        auto* top = new QHBoxLayout();
        auto* station = new QLabel(order.stationName.isEmpty()
                                       ? QStringLiteral("充电站") : order.stationName,
                                   card);
        station->setFont(theme::fieldLabelFont());
        station->setStyleSheet(QStringLiteral("color: %1; background: transparent;")
                                   .arg(cssColor(theme::textPrimary())));
        station->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        auto* status = new QLabel(statusText(order), card);
        status->setAlignment(Qt::AlignCenter);
        status->setMinimumWidth(52);
        status->setStyleSheet(QStringLiteral(
            "color: %1; background: %2; border: 1px solid %1; border-radius: 8px; padding: 2px 6px;")
            .arg(cssColor(order.statusEnum() == OrderInfo::StatusFinished
                              ? theme::success() : order.statusEnum() == OrderInfo::StatusWaitSettlement
                              ? theme::priceAmber() : theme::linkBlue()),
                 cssRgba(theme::background(), 150)));
        top->addWidget(station);
        top->addWidget(status);
        root->addLayout(top);

        const double amount = order.amount > 0.0 ? order.amount : order.estimatedAmount;
        auto* meta = new QLabel(QStringLiteral("订单 %1   %2")
                                    .arg(order.orderId).arg(formatDate(order.createTimeMs)), card);
        meta->setStyleSheet(QStringLiteral("color: %1; background: transparent;")
                                .arg(cssColor(theme::textMuted())));
        auto* detail = new QLabel(QStringLiteral("%1 · %2 度 · ¥%3")
                                      .arg(order.chargerCode.isEmpty()
                                               ? QStringLiteral("充电桩") : order.chargerCode)
                                      .arg(order.energyKwh, 0, 'f', 2)
                                      .arg(amount, 0, 'f', 2), card);
        detail->setStyleSheet(QStringLiteral("color: %1; background: transparent;")
                                  .arg(cssColor(theme::textSecondary())));
        root->addWidget(meta);
        root->addWidget(detail);

        if (order.statusEnum() == OrderInfo::StatusReserved ||
            order.statusEnum() == OrderInfo::StatusCharging ||
            order.statusEnum() == OrderInfo::StatusWaitSettlement ||
            order.statusEnum() == OrderInfo::StatusFinished) {
            auto* action = new QPushButton(
                order.statusEnum() == OrderInfo::StatusFinished
                    ? QStringLiteral("再次导航")
                    : order.statusEnum() == OrderInfo::StatusWaitSettlement
                    ? QStringLiteral("去结算") : QStringLiteral("继续处理"), card);
            action->setFont(theme::footerFont());
            action->setCursor(Qt::PointingHandCursor);
            action->setStyleSheet(smallButtonStyle());
            root->addWidget(action, 0, Qt::AlignRight);
            connect(action, &QPushButton::clicked, this, [this, order] {
                if (order.statusEnum() == OrderInfo::StatusFinished) {
                    emit navigateAgainRequested(order);
                } else {
                    emit continueOrderRequested(order);
                }
            });
        }
        list->addWidget(card);
    }

    stateLabel_->setVisible(visibleCount == 0);
    if (visibleCount == 0) {
        stateLabel_->setText(orders_.isEmpty()
                                 ? QStringLiteral("暂无订单\n完成一次充电后，订单会显示在这里")
                                 : QStringLiteral("没有符合条件的订单"));
        stateLabel_->setStyleSheet(QStringLiteral("color: %1; background: transparent;")
                                       .arg(cssColor(theme::textSecondary())));
    }
    list->addStretch(1);
    orderContainer_->adjustSize();
}

void OrderPage::showState(const QString& text, bool error)
{
    QLayout* oldLayout = orderContainer_->layout();
    if (oldLayout) {
        while (QLayoutItem* item = oldLayout->takeAt(0)) {
            if (QWidget* widget = item->widget()) widget->deleteLater();
            delete item;
        }
        delete oldLayout;
    }
    stateLabel_->setVisible(true);
    stateLabel_->setText(text);
    stateLabel_->setStyleSheet(QStringLiteral("color: %1; background: transparent;")
                                   .arg(cssColor(error ? theme::danger()
                                                       : theme::textSecondary())));
    auto* list = new QVBoxLayout(orderContainer_);
    list->setContentsMargins(0, 0, 0, 0);
    list->addWidget(stateLabel_);
    list->addStretch(1);
}

void OrderPage::onFilterClicked()
{
    auto* button = qobject_cast<QPushButton*>(sender());
    const int index = filterButtons_.indexOf(button);
    if (index < 0) return;
    filterMode_ = index;
    for (int i = 0; i < filterButtons_.size(); ++i) {
        QSignalBlocker blocker(filterButtons_[i]);
        filterButtons_[i]->setChecked(i == filterMode_);
        filterButtons_[i]->setStyleSheet(i == filterMode_
                                             ? activeButtonStyle() : smallButtonStyle());
    }
    renderOrders();
}

void OrderPage::onSearchChanged(const QString& text)
{
    searchText_ = text.trimmed();
    if (!loading_) renderOrders();
}
