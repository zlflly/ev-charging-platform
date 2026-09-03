#include "ui/ChargingPage.h"

#include "net/NetworkClient.h"
#include "protocol/Protocol.h"
#include "session/Session.h"
#include "ui/ChargingHeroWidget.h"
#include "ui/theme/Theme.h"

#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QFrame>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPushButton>
#include <QTimer>

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

void setLabelStyle(QLabel* label, int pixelSize, const QColor& color,
                   bool bold = false)
{
    QFont font(QStringLiteral("Microsoft YaHei UI"));
    font.setPixelSize(pixelSize);
    font.setWeight(bold ? QFont::DemiBold : QFont::Normal);
    label->setFont(font);
    label->setStyleSheet(QStringLiteral(
        "color: %1; background: transparent;").arg(cssColor(color)));
}

void styleCard(QFrame* card)
{
    card->setObjectName(QStringLiteral("ChargingCard"));
    card->setStyleSheet(QStringLiteral(
        "QFrame#ChargingCard { background-color: %1; border: 1px solid %2;"
        " border-radius: 18px; }")
        .arg(cssRgba(theme::cardFill(), 235), cssColor(theme::cardBorder())));
}

class BackButton : public QPushButton
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
        painter.setPen(QPen(theme::textPrimary(), 2.8, Qt::SolidLine,
                            Qt::RoundCap, Qt::RoundJoin));
        const qreal cy = height() / 2.0;
        painter.drawLine(QPointF(25, cy), QPointF(10, cy));
        painter.drawLine(QPointF(10, cy), QPointF(21, cy - 11));
        painter.drawLine(QPointF(10, cy), QPointF(21, cy + 11));
    }
};

class CopyButton : public QPushButton
{
public:
    explicit CopyButton(QWidget* parent = nullptr) : QPushButton(parent)
    {
        setCursor(Qt::PointingHandCursor);
        setStyleSheet(QStringLiteral("background: transparent; border: none;"));
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(QPen(theme::textSecondary(), 1.5, Qt::SolidLine,
                            Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(QRectF(5, 4, 11, 14), 2, 2);
        painter.drawRoundedRect(QRectF(9, 7, 11, 14), 2, 2);
    }
};

class FeeDetailsButton : public QPushButton
{
public:
    explicit FeeDetailsButton(QWidget* parent = nullptr) : QPushButton(parent)
    {
        setCursor(Qt::PointingHandCursor);
        setStyleSheet(QStringLiteral("background: transparent; border: none;"));
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(QPen(theme::cardBorder(), 1));
        QColor fill = theme::cardFill();
        fill.setAlpha(235);
        painter.setBrush(fill);
        painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 17, 17);

        const QPointF center(28, height() / 2.0);
        painter.setPen(QPen(theme::textPrimary(), 1.7));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(center, 10, 10);
        QFont currencyFont(QStringLiteral("Microsoft YaHei UI"));
        currencyFont.setPixelSize(14);
        currencyFont.setWeight(QFont::DemiBold);
        painter.setFont(currencyFont);
        painter.drawText(QRectF(19, height() / 2.0 - 10, 18, 20),
                         Qt::AlignCenter, QStringLiteral("¥"));

        QFont textFont(QStringLiteral("Microsoft YaHei UI"));
        textFont.setPixelSize(17);
        textFont.setWeight(QFont::DemiBold);
        painter.setFont(textFont);
        painter.drawText(QRectF(53, 0, width() - 100, height()),
                         Qt::AlignVCenter | Qt::AlignLeft, text());

        painter.setPen(QPen(theme::textSecondary(), 2.0, Qt::SolidLine,
                            Qt::RoundCap, Qt::RoundJoin));
        painter.drawLine(QPointF(width() - 28, height() / 2.0 - 6),
                         QPointF(width() - 22, height() / 2.0));
        painter.drawLine(QPointF(width() - 22, height() / 2.0),
                         QPointF(width() - 28, height() / 2.0 + 6));
    }
};

} // namespace

ChargingPage::ChargingPage(NetworkClient* networkClient, QWidget* parent)
    : QWidget(parent)
    , networkClient_(networkClient)
{
    setObjectName(QStringLiteral("ChargingPage"));
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumSize(theme::loginCanvasWidth, theme::loginCanvasHeight);
    buildUi();

    ticker_ = new QTimer(this);
    // 弧线需要连续刷新；服务端订单遥测仍按约 3 秒一次轮询。
    ticker_->setInterval(250);
    connect(ticker_, &QTimer::timeout, this, &ChargingPage::onChargingTick);
}

void ChargingPage::buildUi()
{
    backButton_ = new BackButton(this);
    backButton_->setAccessibleName(QStringLiteral("返回站点详情"));
    connect(backButton_, &QPushButton::clicked,
            this, &ChargingPage::onBackClicked);

    titleLabel_ = new QLabel(QStringLiteral("充电准备"), this);
    titleLabel_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    setLabelStyle(titleLabel_, 24, theme::textPrimary(), true);

    orderLabel_ = new QLabel(QStringLiteral("订单号：未生成"), this);
    setLabelStyle(orderLabel_, 15, theme::textSecondary());
    copyOrderButton_ = new CopyButton(this);
    copyOrderButton_->setEnabled(false);
    copyOrderButton_->setToolTip(QStringLiteral("复制订单号"));
    connect(copyOrderButton_, &QPushButton::clicked,
            this, &ChargingPage::onCopyOrderClicked);

    heroWidget_ = new ChargingHeroWidget(this);
    backButton_->raise();
    titleLabel_->raise();
    orderLabel_->raise();
    copyOrderButton_->raise();

    metricsCard_ = new QFrame(this);
    styleCard(metricsCard_);
    energyTitleLabel_ = new QLabel(QStringLiteral("已充电量"), metricsCard_);
    energyValueLabel_ = new QLabel(metricsCard_);
    durationTitleLabel_ = new QLabel(QStringLiteral("充电时长"), metricsCard_);
    durationValueLabel_ = new QLabel(metricsCard_);
    powerTitleLabel_ = new QLabel(QStringLiteral("当前功率"), metricsCard_);
    powerValueLabel_ = new QLabel(metricsCard_);
    for (QLabel* label : {energyTitleLabel_, durationTitleLabel_, powerTitleLabel_}) {
        label->setAlignment(Qt::AlignCenter);
        setLabelStyle(label, 15, theme::textSecondary());
    }
    for (QLabel* label : {energyValueLabel_, durationValueLabel_, powerValueLabel_}) {
        label->setAlignment(Qt::AlignCenter);
        setLabelStyle(label, 21, theme::textPrimary(), true);
    }

    costCard_ = new QFrame(this);
    styleCard(costCard_);
    estimatedTitleLabel_ = new QLabel(QStringLiteral("预计费用"), costCard_);
    estimatedValueLabel_ = new QLabel(costCard_);
    balanceTitleLabel_ = new QLabel(QStringLiteral("账户余额"), costCard_);
    balanceValueLabel_ = new QLabel(costCard_);
    for (QLabel* label : {estimatedTitleLabel_, balanceTitleLabel_}) {
        setLabelStyle(label, 15, theme::textSecondary());
    }
    setLabelStyle(estimatedValueLabel_, 25, theme::textPrimary(), true);
    setLabelStyle(balanceValueLabel_, 25, theme::success(), true);

    feeDetailsButton_ = new FeeDetailsButton(this);
    feeDetailsButton_->setText(QStringLiteral("费用明细"));
    connect(feeDetailsButton_, &QPushButton::clicked,
            this, &ChargingPage::onFeeDetailsClicked);

    actionButton_ = new QPushButton(QStringLiteral("预约充电"), this);
    actionButton_->setObjectName(QStringLiteral("ChargingAction"));
    actionButton_->setCursor(Qt::PointingHandCursor);
    actionButton_->setStyleSheet(QStringLiteral(
        "QPushButton#ChargingAction { color: white; background-color: %1;"
        " border: 1px solid %2; border-radius: 24px; font-size: 22px;"
        " font-weight: 600; }"
        "QPushButton#ChargingAction:hover { background-color: %3; }"
        "QPushButton#ChargingAction:pressed { background-color: %4; }"
        "QPushButton#ChargingAction:disabled { background-color: %5;"
        " color: %6; border-color: transparent; }")
        .arg(cssColor(theme::primaryBlue()), cssColor(QColor(0x55, 0xB2, 0xFF)),
             cssColor(theme::primaryBlueHover()), cssColor(theme::primaryBluePressed()),
             cssColor(QColor(0x12, 0x33, 0x66)), cssColor(theme::textMuted())));
    connect(actionButton_, &QPushButton::clicked,
            this, &ChargingPage::onActionClicked);

    hintLabel_ = new QLabel(this);
    hintLabel_->setAlignment(Qt::AlignCenter);
    hintLabel_->setWordWrap(true);
    setLabelStyle(hintLabel_, 13, theme::textSecondary());

    updateBalanceLabel();
    renderInitialState();
}

void ChargingPage::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.fillRect(rect(), theme::background());

    const QPixmap background(QStringLiteral(":/resources/images/charging_bg.png"));
    if (!background.isNull()) {
        const QPixmap scaled = background.scaled(
            size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        const int x = (width() - scaled.width()) / 2;
        const int y = (height() - scaled.height()) / 2 + qRound(46.0 *
                                                                 height() /
                                                                 theme::loginCanvasHeight);
        painter.drawPixmap(x, y, scaled);
    }
}

void ChargingPage::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    layoutUi();
}

void ChargingPage::layoutUi()
{
    if (width() <= 0 || height() <= 0) return;
    const qreal sx = static_cast<qreal>(width()) / theme::loginCanvasWidth;
    const qreal sy = static_cast<qreal>(height()) / theme::loginCanvasHeight;
    auto X = [sx](qreal value) { return qRound(value * sx); };
    auto Y = [sy](qreal value) { return qRound(value * sy); };

    backButton_->setGeometry(X(14), Y(13), X(38), Y(38));
    titleLabel_->setGeometry(X(68), Y(13), X(220), Y(36));
    orderLabel_->setGeometry(X(68), Y(48), X(250), Y(26));
    copyOrderButton_->setGeometry(X(331), Y(49), X(27), Y(27));

    // 背景车身下移后，把环整体上移，使环底部落在车身中心线附近。
    heroWidget_->setGeometry(0, Y(74), width(), Y(315));

    metricsCard_->setGeometry(X(20), Y(390), width() - X(40), Y(128));
    const int metricWidth = metricsCard_->width() / 3;
    const QList<QPair<QLabel*, QLabel*>> metrics = {
        {energyTitleLabel_, energyValueLabel_},
        {durationTitleLabel_, durationValueLabel_},
        {powerTitleLabel_, powerValueLabel_},
    };
    for (int index = 0; index < metrics.size(); ++index) {
        const int left = index * metricWidth;
        metrics[index].first->setGeometry(left, Y(15), metricWidth, Y(24));
        metrics[index].second->setGeometry(left, Y(52), metricWidth, Y(43));
    }

    costCard_->setGeometry(X(20), Y(529), width() - X(40), Y(88));
    const int costWidth = costCard_->width();
    estimatedTitleLabel_->setGeometry(X(18), Y(13), X(170), Y(23));
    estimatedValueLabel_->setGeometry(X(18), Y(39), X(190), Y(38));
    balanceTitleLabel_->setGeometry(costWidth - X(182), Y(13), X(150), Y(23));
    balanceValueLabel_->setGeometry(costWidth - X(182), Y(39), X(170), Y(38));

    feeDetailsButton_->setGeometry(X(20), Y(627), width() - X(40), Y(54));
    actionButton_->setGeometry(X(20), Y(700), width() - X(40), Y(64));
    hintLabel_->setGeometry(X(28), Y(790), width() - X(56), Y(32));
}

void ChargingPage::openWithCharger(const ChargerInfo& charger,
                                   const QString& stationName,
                                   double pricePerKwh)
{
    ticker_->stop();
    pendingCharger_ = charger;
    pendingStationName_ = stationName;
    pendingPricePerKwh_ = pricePerKwh;
    currentOrder_ = OrderInfo{};
    hasOrder_ = false;
    state_ = Initial;
    requestInFlight_ = false;
    pollInFlight_ = false;
    serverEnergyKnown_ = false;
    pollTicks_ = 0;
    renderInitialState();

    if (previewMode()) {
        return;
    }
    requestActiveOrder();
}

bool ChargingPage::previewMode() const
{
    return qEnvironmentVariableIsSet("EV_HOME_PREVIEW") ||
           qEnvironmentVariableIsSet("EV_DETAIL_PREVIEW") ||
           qEnvironmentVariableIsSet("EV_CHARGING_PREVIEW") ||
           qEnvironmentVariableIsSet("EV_SETTLEMENT_PREVIEW");
}

double ChargingPage::displayBalance() const
{
    if (previewMode() && !Session::instance().isLoggedIn()) {
        return previewBalance_;
    }
    return Session::instance().balance();
}

void ChargingPage::requestActiveOrder()
{
    if (!networkClient_) {
        renderErrorState(QStringLiteral("网络模块未初始化"));
        return;
    }
    setRequestInFlight(true);
    actionButton_->setVisible(false);
    hintLabel_->setText(QStringLiteral("正在检查未完成订单..."));
    networkClient_->sendRequest(
        QString::fromUtf8(protocol::action::kOrderActive), {},
        [this](const protocol::Response& response) {
            setRequestInFlight(false);
            if (!response.isOk()) {
                renderErrorState(protocol::describeError(response.code,
                                                         response.message));
                return;
            }
            const OrderInfo order = OrderInfo::fromJson(
                response.data.value(QStringLiteral("order")).toObject());
            if (!order.valid()) {
                currentOrder_ = OrderInfo{};
                hasOrder_ = false;
                state_ = Initial;
                renderInitialState();
                return;
            }
            currentOrder_ = order;
            hasOrder_ = true;
            serverEnergyKnown_ = order.energyKwh > 0.0;
            state_ = order.statusEnum() == OrderInfo::StatusReserved
                ? Reserved : order.statusEnum() == OrderInfo::StatusCharging
                ? Charging : Error;
            if (state_ == Error) {
                renderErrorState(QStringLiteral("当前订单状态暂不支持在此页面继续操作"));
            } else {
                renderState();
            }
        });
}

void ChargingPage::requestOrderStatus()
{
    if (!networkClient_ || pollInFlight_ || requestInFlight_ || !hasOrder_ ||
        state_ != Charging) {
        return;
    }
    pollInFlight_ = true;
    networkClient_->sendRequest(
        QString::fromUtf8(protocol::action::kOrderStatus), {},
        [this](const protocol::Response& response) {
            pollInFlight_ = false;
            if (!response.isOk()) return;
            const OrderInfo updated = OrderInfo::fromJson(
                response.data.value(QStringLiteral("order")).toObject());
            if (!updated.valid()) return;
            currentOrder_ = updated;
            serverEnergyKnown_ = updated.energyKwh > 0.0;
            if (updated.statusEnum() == OrderInfo::StatusWaitSettlement) {
                ticker_->stop();
                emit settlementRequested(currentOrder_);
                return;
            }
            updateChargingReadout();
        });
}

void ChargingPage::onActionClicked()
{
    if (requestInFlight_) return;
    if (state_ == Error) {
        requestActiveOrder();
        return;
    }
    if (!hasOrder_) {
        if (previewMode()) {
            setOrderFromPending();
            state_ = Reserved;
            hasOrder_ = true;
            renderReservedState();
        } else {
            reserveCharger();
        }
        return;
    }
    if (state_ == Reserved) {
        startCharging();
    } else if (state_ == Charging) {
        stopCharging();
    }
}

void ChargingPage::reserveCharger()
{
    if (!networkClient_ || !pendingCharger_.valid()) return;
    setRequestInFlight(true);
    actionButton_->setText(QStringLiteral("预约中..."));
    QJsonObject data;
    data.insert(QStringLiteral("chargerId"), pendingCharger_.chargerId);
    networkClient_->sendRequest(
        QString::fromUtf8(protocol::action::kOrderReserve), data,
        [this](const protocol::Response& response) {
            setRequestInFlight(false);
            if (!response.isOk()) {
                renderErrorState(protocol::describeError(response.code,
                                                         response.message));
                return;
            }
            currentOrder_ = OrderInfo::fromJson(
                response.data.value(QStringLiteral("order")).toObject());
            hasOrder_ = currentOrder_.valid();
            serverEnergyKnown_ = currentOrder_.energyKwh > 0.0;
            state_ = Reserved;
            renderReservedState();
        });
}

void ChargingPage::startCharging()
{
    if (previewMode()) {
        currentOrder_.status = QStringLiteral("CHARGING");
        currentOrder_.startTimeMs = QDateTime::currentMSecsSinceEpoch();
        currentOrder_.powerKw = pendingCharger_.type == protocol::ChargerTypeFast
            ? 42.0 : qMin(7.0, pendingCharger_.powerKw);
        serverEnergyKnown_ = false;
        state_ = Charging;
        renderChargingState();
        return;
    }
    if (!networkClient_) return;
    setRequestInFlight(true);
    actionButton_->setText(QStringLiteral("开始中..."));
    QJsonObject data;
    data.insert(QStringLiteral("orderId"), currentOrder_.orderId);
    networkClient_->sendRequest(
        QString::fromUtf8(protocol::action::kOrderStart), data,
        [this](const protocol::Response& response) {
            setRequestInFlight(false);
            if (!response.isOk()) {
                renderErrorState(protocol::describeError(response.code,
                                                         response.message));
                return;
            }
            currentOrder_ = OrderInfo::fromJson(
                response.data.value(QStringLiteral("order")).toObject());
            state_ = Charging;
            hasOrder_ = currentOrder_.valid();
            serverEnergyKnown_ = currentOrder_.energyKwh > 0.0;
            renderChargingState();
        });
}

void ChargingPage::stopCharging()
{
    if (previewMode()) {
        updateChargingReadout();
        currentOrder_.status = QStringLiteral("WAIT_SETTLEMENT");
        currentOrder_.stopTimeMs = QDateTime::currentMSecsSinceEpoch();
        currentOrder_.amount = currentOrder_.estimatedAmount;
        ticker_->stop();
        emit settlementRequested(currentOrder_);
        return;
    }
    if (!networkClient_) return;
    setRequestInFlight(true);
    actionButton_->setText(QStringLiteral("结束中..."));
    QJsonObject data;
    data.insert(QStringLiteral("orderId"), currentOrder_.orderId);
    networkClient_->sendRequest(
        QString::fromUtf8(protocol::action::kOrderStop), data,
        [this](const protocol::Response& response) {
            setRequestInFlight(false);
            if (!response.isOk()) {
                renderErrorState(protocol::describeError(response.code,
                                                         response.message));
                return;
            }
            const OrderInfo order = OrderInfo::fromJson(
                response.data.value(QStringLiteral("order")).toObject());
            ticker_->stop();
            emit settlementRequested(order.valid() ? order : currentOrder_);
        });
}

void ChargingPage::renderState()
{
    switch (state_) {
    case Initial:  renderInitialState(); break;
    case Reserved: renderReservedState(); break;
    case Charging: renderChargingState(); break;
    case Error:    break;
    }
}

void ChargingPage::renderInitialState()
{
    titleLabel_->setText(QStringLiteral("充电准备"));
    orderLabel_->setText(QStringLiteral("订单号：未生成"));
    copyOrderButton_->setEnabled(false);
    heroWidget_->setProgress(0);
    heroWidget_->setElapsedText(QStringLiteral("00:00:00"));
    heroWidget_->setSubtitleVisible(false);
    energyValueLabel_->setText(QStringLiteral("0.00 度"));
    durationValueLabel_->setText(QStringLiteral("00:00:00"));
    powerValueLabel_->setText(QStringLiteral("0.0 kW"));
    estimatedValueLabel_->setText(QStringLiteral("¥0.00"));
    updateBalanceLabel();
    actionButton_->setVisible(true);
    actionButton_->setEnabled(!requestInFlight_);
    actionButton_->setText(QStringLiteral("预约充电"));
    setLabelStyle(hintLabel_, 13, theme::textSecondary());
    hintLabel_->setText(QStringLiteral("预约成功后才会开始计费"));
}

void ChargingPage::renderReservedState()
{
    titleLabel_->setText(QStringLiteral("待开始"));
    orderLabel_->setText(QStringLiteral("订单号：%1").arg(currentOrder_.orderId));
    copyOrderButton_->setEnabled(currentOrder_.orderId > 0);
    heroWidget_->setProgress(0);
    heroWidget_->setElapsedText(QStringLiteral("00:00:00"));
    heroWidget_->setSubtitleVisible(false);
    energyValueLabel_->setText(QStringLiteral("0.00 度"));
    durationValueLabel_->setText(QStringLiteral("00:00:00"));
    powerValueLabel_->setText(QStringLiteral("0.0 kW"));
    estimatedValueLabel_->setText(QStringLiteral("¥0.00"));
    updateBalanceLabel();
    actionButton_->setVisible(true);
    actionButton_->setEnabled(!requestInFlight_);
    actionButton_->setText(QStringLiteral("开始充电"));
    setLabelStyle(hintLabel_, 13, theme::textSecondary());
    hintLabel_->setText(QStringLiteral("充电桩已为您保留"));
}

void ChargingPage::renderChargingState()
{
    titleLabel_->setText(QStringLiteral("充电中"));
    orderLabel_->setText(QStringLiteral("订单号：%1").arg(currentOrder_.orderId));
    copyOrderButton_->setEnabled(currentOrder_.orderId > 0);
    heroWidget_->setSubtitleVisible(true);
    actionButton_->setVisible(true);
    actionButton_->setEnabled(!requestInFlight_);
    actionButton_->setText(QStringLiteral("结束充电"));
    setLabelStyle(hintLabel_, 13, theme::textSecondary());
    hintLabel_->setText(QStringLiteral("充电数据实时更新，最终费用以结算为准"));
    ticker_->start();
    updateChargingReadout();
}

void ChargingPage::renderErrorState(const QString& message)
{
    state_ = Error;
    ticker_->stop();
    titleLabel_->setText(QStringLiteral("充电"));
    actionButton_->setVisible(true);
    actionButton_->setEnabled(true);
    actionButton_->setText(QStringLiteral("重试"));
    setLabelStyle(hintLabel_, 13, theme::danger());
    hintLabel_->setText(message);
}

void ChargingPage::updateChargingReadout()
{
    if (state_ != Charging || !hasOrder_) return;
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 elapsedMs = currentOrder_.startTimeMs > 0
        ? qMax<qint64>(0, now - currentOrder_.startTimeMs) : 0;
    const double power = currentOrder_.powerKw > 0.0 ? currentOrder_.powerKw : 42.0;
    const double fallbackEnergy = power * static_cast<double>(elapsedMs) / 3600000.0;
    const bool useServerTelemetry = !previewMode() && serverEnergyKnown_;
    const double energy = useServerTelemetry ? currentOrder_.energyKwh : fallbackEnergy;
    const double price = currentOrder_.pricePerKwh > 0.0
        ? currentOrder_.pricePerKwh : pendingPricePerKwh_;
    const double estimated = useServerTelemetry && currentOrder_.estimatedAmount > 0.0
        ? currentOrder_.estimatedAmount : energy * price;
    heroWidget_->setProgress(progressForEnergy(energy));
    heroWidget_->setElapsedText(formatDuration(elapsedMs));
    energyValueLabel_->setText(QStringLiteral("%1 度").arg(energy, 0, 'f', 2));
    durationValueLabel_->setText(formatDuration(elapsedMs));
    powerValueLabel_->setText(QStringLiteral("%1 kW").arg(power, 0, 'f', 1));
    estimatedValueLabel_->setText(QStringLiteral("¥%1").arg(estimated, 0, 'f', 2));
    if (previewMode()) {
        currentOrder_.energyKwh = energy;
        currentOrder_.estimatedAmount = estimated;
    }
    updateBalanceLabel();
}

QString ChargingPage::formatDuration(qint64 milliseconds) const
{
    const qint64 totalSeconds = milliseconds / 1000;
    return QStringLiteral("%1:%2:%3")
        .arg(totalSeconds / 3600)
        .arg((totalSeconds % 3600) / 60, 2, 10, QLatin1Char('0'))
        .arg(totalSeconds % 60, 2, 10, QLatin1Char('0'));
}

double ChargingPage::progressForEnergy(double energyKwh) const
{
    // 服务端若提供目标电量/电池容量，用实际已充电量计算；否则使用
    // 34.3 度作为预览标定值，使设计稿中的 12.35 度约落在 36%。
    // 这只是展示进度，费用和订单结算仍以服务端返回值为准。
    const double targetEnergy = currentOrder_.targetEnergyKwh > 0.0
        ? currentOrder_.targetEnergyKwh : 34.3;
    return qBound(0.0, energyKwh / targetEnergy * 100.0, 99.0);
}

void ChargingPage::updateBalanceLabel()
{
    if (balanceValueLabel_) {
        balanceValueLabel_->setText(QStringLiteral("¥%1")
            .arg(displayBalance(), 0, 'f', 2));
    }
}

void ChargingPage::setRequestInFlight(bool inFlight)
{
    requestInFlight_ = inFlight;
    if (actionButton_ && actionButton_->isVisible()) {
        actionButton_->setEnabled(!inFlight);
    }
}

void ChargingPage::onChargingTick()
{
    ++pollTicks_;
    updateChargingReadout();
    if (pollTicks_ >= 12) {
        pollTicks_ = 0;
        requestOrderStatus();
    }
}

void ChargingPage::onBackClicked()
{
    ticker_->stop();
    emit backRequested();
}

void ChargingPage::onCopyOrderClicked()
{
    if (currentOrder_.orderId <= 0) return;
    if (QClipboard* clipboard = QApplication::clipboard()) {
        clipboard->setText(QString::number(currentOrder_.orderId));
    }
}

void ChargingPage::onFeeDetailsClicked()
{
    const double price = currentOrder_.pricePerKwh > 0.0
        ? currentOrder_.pricePerKwh : pendingPricePerKwh_;
    QMessageBox::information(
        this, QStringLiteral("费用明细"),
        QStringLiteral("站点：%1\n桩号：%2\n单价：%3 元/度\n已充电量：%4 度\n"
                       "当前预估：%5 元\n最终金额以结算时服务端计算为准")
            .arg(pendingStationName_, pendingCharger_.code)
            .arg(price, 0, 'f', 2)
            .arg(currentOrder_.energyKwh, 0, 'f', 2)
            .arg(currentOrder_.estimatedAmount, 0, 'f', 2));
}

void ChargingPage::setOrderFromPending()
{
    currentOrder_ = OrderInfo{};
    currentOrder_.orderId = QDateTime::currentMSecsSinceEpoch() % 1000000000;
    currentOrder_.status = QStringLiteral("RESERVED");
    currentOrder_.chargerId = pendingCharger_.chargerId;
    currentOrder_.chargerCode = pendingCharger_.code;
    currentOrder_.chargerType = pendingCharger_.type;
    currentOrder_.stationName = pendingStationName_;
    currentOrder_.pricePerKwh = pendingPricePerKwh_;
    currentOrder_.powerKw = pendingCharger_.powerKw;
}
