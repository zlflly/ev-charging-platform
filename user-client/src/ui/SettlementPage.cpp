#include "ui/SettlementPage.h"

#include "net/NetworkClient.h"
#include "protocol/Protocol.h"
#include "session/Session.h"
#include "ui/theme/Theme.h"

#include <QFrame>
#include <QIcon>
#include <QJsonObject>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>

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
    card->setObjectName(QStringLiteral("SettlementCard"));
    card->setStyleSheet(QStringLiteral(
        "QFrame#SettlementCard { background-color: %1; border: 1px solid %2;"
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
        const QPixmap icon = tintedSvgPixmap(
            QStringLiteral(":/resources/icons/back.svg"), QSize(26, 26),
            theme::textPrimary());
        if (!icon.isNull()) {
            painter.drawPixmap((width() - icon.width()) / 2,
                               (height() - icon.height()) / 2, icon);
        }
    }
};

class CheckIcon : public QFrame
{
public:
    explicit CheckIcon(QWidget* parent = nullptr) : QFrame(parent)
    {
        setStyleSheet(QStringLiteral("background: transparent; border: none;"));
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(QPen(theme::success(), 2.4, Qt::SolidLine,
                            Qt::RoundCap, Qt::RoundJoin));
        QColor fill = theme::success();
        fill.setAlpha(34);
        painter.setBrush(fill);
        painter.drawEllipse(rect().adjusted(2, 2, -3, -3));
        painter.drawLine(QPointF(width() * .29, height() * .52),
                         QPointF(width() * .45, height() * .68));
        painter.drawLine(QPointF(width() * .45, height() * .68),
                         QPointF(width() * .73, height() * .35));
    }
};

} // namespace

SettlementPage::SettlementPage(NetworkClient* networkClient, QWidget* parent)
    : QWidget(parent)
    , networkClient_(networkClient)
{
    setObjectName(QStringLiteral("SettlementPage"));
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumSize(theme::loginCanvasWidth, theme::loginCanvasHeight);
    buildUi();
}

void SettlementPage::buildUi()
{
    backButton_ = new BackButton(this);
    backButton_->setAccessibleName(QStringLiteral("返回充电页"));
    connect(backButton_, &QPushButton::clicked,
            this, &SettlementPage::onBackClicked);

    titleLabel_ = new QLabel(QStringLiteral("充电结算"), this);
    setLabelStyle(titleLabel_, 24, theme::textPrimary(), true);

    successIcon_ = new CheckIcon(this);
    stateLabel_ = new QLabel(QStringLiteral("充电已结束，请确认费用"), this);
    setLabelStyle(stateLabel_, 15, theme::textSecondary());

    summaryCard_ = new QFrame(this);
    styleCard(summaryCard_);
    stationLabel_ = new QLabel(summaryCard_);
    chargerLabel_ = new QLabel(summaryCard_);
    setLabelStyle(stationLabel_, 21, theme::textPrimary(), true);
    setLabelStyle(chargerLabel_, 15, theme::textSecondary());

    metricsCard_ = new QFrame(this);
    styleCard(metricsCard_);
    energyLabel_ = new QLabel(metricsCard_);
    durationLabel_ = new QLabel(metricsCard_);
    setLabelStyle(energyLabel_, 17, theme::textPrimary(), true);
    setLabelStyle(durationLabel_, 17, theme::textPrimary(), true);

    feeCard_ = new QFrame(this);
    styleCard(feeCard_);
    electricityLabel_ = new QLabel(feeCard_);
    serviceLabel_ = new QLabel(feeCard_);
    totalLabel_ = new QLabel(feeCard_);
    setLabelStyle(electricityLabel_, 15, theme::textSecondary());
    setLabelStyle(serviceLabel_, 15, theme::textSecondary());
    setLabelStyle(totalLabel_, 22, theme::primaryBlue(), true);

    balanceCard_ = new QFrame(this);
    styleCard(balanceCard_);
    balanceLabel_ = new QLabel(balanceCard_);
    setLabelStyle(balanceLabel_, 17, theme::success(), true);

    primaryButton_ = new QPushButton(QStringLiteral("确认结算"), this);
    primaryButton_->setObjectName(QStringLiteral("SettlementPrimary"));
    primaryButton_->setCursor(Qt::PointingHandCursor);
    primaryButton_->setStyleSheet(QStringLiteral(
        "QPushButton#SettlementPrimary { color: white; background-color: %1;"
        " border: 1px solid %2; border-radius: 24px; font-size: 22px;"
        " font-weight: 600; }"
        "QPushButton#SettlementPrimary:hover { background-color: %3; }"
        "QPushButton#SettlementPrimary:disabled { background-color: %4;"
        " color: %5; border-color: transparent; }")
        .arg(cssColor(theme::primaryBlue()), cssColor(QColor(0x55, 0xB2, 0xFF)),
             cssColor(theme::primaryBlueHover()), cssColor(QColor(0x12, 0x33, 0x66)),
             cssColor(theme::textMuted())));
    connect(primaryButton_, &QPushButton::clicked,
            this, &SettlementPage::onPrimaryClicked);

    hintLabel_ = new QLabel(this);
    hintLabel_->setAlignment(Qt::AlignCenter);
    hintLabel_->setWordWrap(true);
    setLabelStyle(hintLabel_, 13, theme::textSecondary());

    render();
}

void SettlementPage::openOrder(const OrderInfo& order)
{
    order_ = order;
    settled_ = false;
    requestInFlight_ = false;
    render();
}

bool SettlementPage::previewMode() const
{
    return qEnvironmentVariableIsSet("EV_HOME_PREVIEW") ||
           qEnvironmentVariableIsSet("EV_DETAIL_PREVIEW") ||
           qEnvironmentVariableIsSet("EV_CHARGING_PREVIEW") ||
           qEnvironmentVariableIsSet("EV_SETTLEMENT_PREVIEW");
}

double SettlementPage::displayBalance() const
{
    if (previewMode() && !Session::instance().isLoggedIn()) {
        return previewBalance_;
    }
    return Session::instance().balance();
}

void SettlementPage::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.fillRect(rect(), theme::background());
    const QPixmap background(QStringLiteral(":/resources/images/charging_bg.png"));
    if (!background.isNull()) {
        const QPixmap scaled = background.scaled(
            size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        painter.setOpacity(0.48);
        painter.drawPixmap((width() - scaled.width()) / 2,
                           (height() - scaled.height()) / 2, scaled);
        painter.setOpacity(1.0);
    }
}

void SettlementPage::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    layoutUi();
}

void SettlementPage::layoutUi()
{
    if (width() <= 0 || height() <= 0) return;
    const qreal sx = static_cast<qreal>(width()) / theme::loginCanvasWidth;
    const qreal sy = static_cast<qreal>(height()) / theme::loginCanvasHeight;
    auto X = [sx](qreal value) { return qRound(value * sx); };
    auto Y = [sy](qreal value) { return qRound(value * sy); };

    backButton_->setGeometry(X(14), Y(13), X(38), Y(38));
    titleLabel_->setGeometry(X(68), Y(13), X(240), Y(36));
    successIcon_->setGeometry(X(385), Y(15), X(32), Y(32));
    stateLabel_->setGeometry(X(68), Y(50), X(330), Y(25));

    summaryCard_->setGeometry(X(20), Y(84), width() - X(40), Y(145));
    successIcon_->raise();
    stationLabel_->setGeometry(X(18), Y(20), width() - X(76), Y(31));
    chargerLabel_->setGeometry(X(18), Y(62), width() - X(36), Y(28));

    metricsCard_->setGeometry(X(20), Y(243), width() - X(40), Y(118));
    const int half = metricsCard_->width() / 2;
    energyLabel_->setGeometry(X(12), Y(35), half - X(20), Y(42));
    durationLabel_->setGeometry(half + X(8), Y(35), half - X(20), Y(42));

    feeCard_->setGeometry(X(20), Y(374), width() - X(40), Y(143));
    electricityLabel_->setGeometry(X(18), Y(17), width() - X(76), Y(27));
    serviceLabel_->setGeometry(X(18), Y(52), width() - X(76), Y(27));
    totalLabel_->setGeometry(X(18), Y(95), width() - X(36), Y(36));

    balanceCard_->setGeometry(X(20), Y(531), width() - X(40), Y(74));
    balanceLabel_->setGeometry(X(18), Y(17), width() - X(36), Y(38));

    primaryButton_->setGeometry(X(20), Y(650), width() - X(40), Y(64));
    hintLabel_->setGeometry(X(28), Y(724), width() - X(56), Y(48));
}

void SettlementPage::render()
{
    const QString stationName = order_.stationName.isEmpty()
        ? QStringLiteral("东软软件园充电站") : order_.stationName;
    const QString chargerCode = order_.chargerCode.isEmpty()
        ? QStringLiteral("充电桩") : order_.chargerCode;
    const double energy = qMax(0.0, order_.energyKwh);
    const qint64 durationMs = order_.durationMs();
    const double total = qMax(0.0, order_.amount > 0.0
        ? order_.amount : order_.estimatedAmount);
    const double electricity = order_.electricityFee > 0.0
        ? order_.electricityFee : total * 0.70;
    const double service = order_.serviceFee > 0.0
        ? order_.serviceFee : qMax(0.0, total - electricity);

    stateLabel_->setText(settled_ ? QStringLiteral("结算完成，感谢使用东软充电")
                                 : QStringLiteral("充电已结束，请确认费用"));
    stationLabel_->setText(stationName);
    chargerLabel_->setText(QStringLiteral("%1 · %2kW · %3")
        .arg(chargerCode)
        .arg(order_.powerKw, 0, 'f', 0)
        .arg(order_.chargerType == protocol::ChargerTypeFast
                 ? QStringLiteral("快充") : QStringLiteral("慢充")));
    energyLabel_->setText(QStringLiteral("已充电量  %1 度")
        .arg(energy, 0, 'f', 2));
    const qint64 seconds = durationMs / 1000;
    durationLabel_->setText(QStringLiteral("充电时长  %1:%2:%3")
        .arg(seconds / 3600)
        .arg((seconds % 3600) / 60, 2, 10, QLatin1Char('0'))
        .arg(seconds % 60, 2, 10, QLatin1Char('0')));
    electricityLabel_->setText(QStringLiteral("电费  ¥%1").arg(electricity, 0, 'f', 2));
    serviceLabel_->setText(QStringLiteral("服务费  ¥%1").arg(service, 0, 'f', 2));
    totalLabel_->setText(QStringLiteral("应付合计  ¥%1").arg(total, 0, 'f', 2));
    balanceLabel_->setText(QStringLiteral("结算前余额  ¥%1")
        .arg(displayBalance(), 0, 'f', 2));

    primaryButton_->setEnabled(!requestInFlight_);
    primaryButton_->setText(settled_ ? QStringLiteral("返回首页") : QStringLiteral("确认结算"));
    hintLabel_->setText(settled_ ? QStringLiteral("费用已记录，可在订单页面查看")
                                 : QStringLiteral("确认后将从账户余额扣除本次费用"));
}

void SettlementPage::onBackClicked()
{
    emit backRequested();
}

void SettlementPage::onPrimaryClicked()
{
    if (settled_) {
        emit homeRequested();
        return;
    }
    settleOrder();
}

void SettlementPage::settleOrder()
{
    if (requestInFlight_) return;
    const double total = qMax(0.0, order_.amount > 0.0
        ? order_.amount : order_.estimatedAmount);
    if (previewMode()) {
        if (total > previewBalance_) {
            showError(QStringLiteral("余额不足，还需充值 %1 元")
                .arg(total - previewBalance_, 0, 'f', 2));
            return;
        }
        previewBalance_ -= total;
        settled_ = true;
        if (Session::instance().isLoggedIn()) {
            Session::instance().setBalance(Session::instance().balance() - total);
        }
        render();
        return;
    }
    if (!networkClient_ || order_.orderId <= 0) {
        showError(QStringLiteral("订单信息无效，请返回后重试"));
        return;
    }
    requestInFlight_ = true;
    primaryButton_->setText(QStringLiteral("结算中..."));
    primaryButton_->setEnabled(false);
    QJsonObject data;
    data.insert(QStringLiteral("orderId"), order_.orderId);
    networkClient_->sendRequest(
        QString::fromUtf8(protocol::action::kOrderSettle), data,
        [this](const protocol::Response& response) {
            requestInFlight_ = false;
            if (!response.isOk()) {
                if (response.code == protocol::CodeBalanceInsufficient) {
                    const double deficit = response.data
                        .value(QStringLiteral("deficit")).toDouble();
                    showError(deficit > 0.0
                                  ? QStringLiteral("余额不足，还需充值 ¥%1；订单仍保留，可充值后重试")
                                        .arg(deficit, 0, 'f', 2)
                                  : QStringLiteral("余额不足，订单仍保留，可充值后重试"));
                } else {
                    showError(protocol::describeError(response.code,
                                                      response.message));
                }
                return;
            }
            if (response.data.contains(QStringLiteral("balance"))) {
                Session::instance().setBalance(
                    response.data.value(QStringLiteral("balance")).toDouble());
            }
            settled_ = true;
            render();
        });
}

void SettlementPage::showError(const QString& message)
{
    hintLabel_->setText(message);
    hintLabel_->setStyleSheet(QStringLiteral(
        "color: %1; background: transparent;").arg(cssColor(theme::danger())));
    primaryButton_->setEnabled(true);
}
