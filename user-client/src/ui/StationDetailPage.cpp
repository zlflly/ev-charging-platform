#include "ui/StationDetailPage.h"

#include "net/NetworkClient.h"
#include "protocol/Protocol.h"
#include "session/Session.h"
#include "ui/ChargerCard.h"
#include "ui/theme/Theme.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QStyleOption>
#include <QVBoxLayout>
#include <QtMath>
#include <cmath>

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

QPainterPath starPath(const QPointF& center, qreal outerRadius, qreal innerRadius)
{
    constexpr qreal kPi = 3.14159265358979323846;
    QPainterPath path;
    for (int index = 0; index < 10; ++index) {
        const qreal angle = -kPi / 2.0 + index * kPi / 5.0;
        const qreal radius = index % 2 == 0 ? outerRadius : innerRadius;
        const QPointF point(center.x() + std::cos(angle) * radius,
                            center.y() + std::sin(angle) * radius);
        if (index == 0) path.moveTo(point);
        else path.lineTo(point);
    }
    path.closeSubpath();
    return path;
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

class PinIcon : public QFrame
{
public:
    explicit PinIcon(QWidget* parent = nullptr) : QFrame(parent)
    {
        setStyleSheet(QStringLiteral("background: transparent; border: none;"));
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QPixmap icon = tintedSvgPixmap(
            QStringLiteral(":/resources/icons/station.svg"), QSize(25, 25),
            theme::textPrimary());
        if (!icon.isNull()) {
            painter.drawPixmap((width() - icon.width()) / 2,
                               (height() - icon.height()) / 2, icon);
            return;
        }
        const qreal cx = width() / 2.0;
        QPainterPath pin;
        pin.moveTo(cx, height() - 2);
        pin.cubicTo(cx - 3, height() - 7, 2, height() * 0.53,
                    2, height() * 0.36);
        pin.cubicTo(2, 4, 7, 1, cx, 1);
        pin.cubicTo(width() - 7, 1, width() - 2, 4,
                    width() - 2, height() * 0.36);
        pin.cubicTo(width() - 2, height() * 0.53, cx + 3, height() - 7,
                    cx, height() - 2);
        painter.setPen(Qt::NoPen);
        painter.setBrush(theme::textPrimary());
        painter.drawPath(pin);
        painter.setBrush(theme::background());
        painter.drawEllipse(QPointF(cx, height() * 0.34), 3.8, 3.8);
    }
};

class DirectionIcon : public QFrame
{
public:
    explicit DirectionIcon(QWidget* parent = nullptr) : QFrame(parent)
    {
        setStyleSheet(QStringLiteral("background: transparent; border: none;"));
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QPixmap icon = tintedSvgPixmap(
            QStringLiteral(":/resources/icons/navigate.svg"), QSize(26, 26),
            theme::primaryBlue());
        if (!icon.isNull()) {
            painter.drawPixmap((width() - icon.width()) / 2,
                               (height() - icon.height()) / 2, icon);
            return;
        }
        QPainterPath arrow;
        arrow.moveTo(4, height() - 5);
        arrow.lineTo(width() - 4, 4);
        arrow.lineTo(width() - 9, height() - 3);
        arrow.lineTo(width() - 13, height() - 8);
        arrow.closeSubpath();
        painter.setPen(Qt::NoPen);
        painter.setBrush(theme::primaryBlue());
        painter.drawPath(arrow);
    }
};

class RatingIcon : public QFrame
{
public:
    explicit RatingIcon(QWidget* parent = nullptr) : QFrame(parent)
    {
        setStyleSheet(QStringLiteral("background: transparent; border: none;"));
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QPixmap icon = tintedSvgPixmap(
            QStringLiteral(":/resources/icons/favorite.svg"), QSize(26, 26),
            theme::primaryBlue());
        if (!icon.isNull()) {
            painter.drawPixmap((width() - icon.width()) / 2,
                               (height() - icon.height()) / 2, icon);
            return;
        }
        painter.setPen(Qt::NoPen);
        painter.setBrush(theme::primaryBlue());
        painter.drawPath(starPath(QPointF(width() / 2.0, height() / 2.0),
                                  qMin(width(), height()) * 0.46,
                                  qMin(width(), height()) * 0.20));
    }
};

class TagPill : public QFrame
{
public:
    TagPill(const QString& text, bool lightning, QWidget* parent = nullptr)
        : QFrame(parent), text_(text), lightning_(lightning)
    {
        setStyleSheet(QStringLiteral("background: transparent; border: none;"));
    }

    void setText(const QString& text)
    {
        text_ = text;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QColor fill = lightning_ ? QColor(0x18, 0x58, 0x42) : theme::chipBg();
        const QColor border = lightning_ ? QColor(0x1E, 0x86, 0x58) : theme::cardBorder();
        painter.setPen(QPen(border, 1));
        painter.setBrush(fill);
        painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 6, 6);

        int textLeft = 10;
        if (lightning_) {
            const QPixmap chargeIcon = tintedSvgPixmap(
                QStringLiteral(":/resources/icons/charge.svg"), QSize(18, 18),
                theme::success());
            if (!chargeIcon.isNull()) {
                painter.drawPixmap(7, (height() - chargeIcon.height()) / 2, chargeIcon);
                textLeft = 29;
            }
        }

        QFont font(QStringLiteral("Microsoft YaHei UI"));
        font.setPixelSize(15);
        font.setWeight(QFont::DemiBold);
        painter.setFont(font);
        painter.setPen(lightning_ ? theme::textPrimary()
                                  : (text_ == QStringLiteral("营业中")
                                         ? theme::success() : theme::primaryBlue()));
        painter.drawText(QRectF(textLeft, 0, width() - textLeft - 6, height()),
                         Qt::AlignVCenter | Qt::AlignLeft, text_);
    }

private:
    QString text_;
    bool lightning_ = false;
};

class ActionButton : public QPushButton
{
public:
    enum class Kind { Favorite, Navigate };

    ActionButton(Kind kind, QWidget* parent = nullptr)
        : QPushButton(parent), kind_(kind)
    {
        setCursor(Qt::PointingHandCursor);
        setStyleSheet(QStringLiteral(
            "QPushButton { color: %1; border: 1px solid %2; border-radius: 18px;"
            " background: %3; font-size: 22px; font-weight: 600; }"
            "QPushButton:hover { border-color: %4; background: %5; }")
            .arg(cssColor(theme::textPrimary()), cssColor(theme::cardBorder()),
                 cssRgba(theme::cardFill(), 230), cssColor(theme::primaryBlue()),
                 cssRgba(theme::primaryBlue(), 35)));
    }

    void setFavoriteState(bool favorite)
    {
        favorite_ = favorite;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        // 先绘制按钮外观，再统一绘制图标和文字，避免已收藏三个字被默认居中布局挤到星号上。
        QStyleOptionButton option;
        initStyleOption(&option);
        option.text.clear();
        style()->drawControl(QStyle::CE_PushButton, &option, &painter, this);

        const qreal centerY = height() / 2.0;
        const qreal iconX = width() > 200 ? width() / 2.0 - 28
                                        : width() / 2.0 - 20;

        QFont textFont(QStringLiteral("Microsoft YaHei UI"));
        textFont.setPixelSize(22);
        textFont.setWeight(QFont::DemiBold);
        painter.setFont(textFont);
        painter.setPen(kind_ == Kind::Navigate ? Qt::white : theme::textPrimary());
        painter.drawText(QRectF(iconX + 23, 0, width() - iconX - 29, height()),
                         Qt::AlignVCenter | Qt::AlignLeft, text());

        if (kind_ == Kind::Favorite) {
            const QColor starColor = favorite_ ? theme::priceAmber()
                                                : theme::textPrimary();
            const QPixmap star = tintedSvgPixmap(
                QStringLiteral(":/resources/icons/favorite.svg"), QSize(28, 28),
                starColor);
            if (!star.isNull()) {
                painter.drawPixmap(qRound(iconX - star.width() / 2.0),
                                   qRound(centerY - star.height() / 2.0), star);
            }
        } else {
            const QPixmap navigate = tintedSvgPixmap(
                QStringLiteral(":/resources/icons/navigate_button.svg"), QSize(24, 24),
                theme::textPrimary());
            if (!navigate.isNull()) {
                painter.drawPixmap(qRound(iconX - navigate.width() / 2.0),
                                   qRound(centerY - navigate.height() / 2.0), navigate);
            }
        }
    }

private:
    Kind kind_;
    bool favorite_ = false;
};

StationDetail makePreviewDetail()
{
    StationDetail detail;
    detail.station = {1, QStringLiteral("东软软件园充电站"), 1.25, 20, 8, 1.2};
    detail.address = QStringLiteral("辽宁省沈阳市浑南区新秀街2号东软软件园");
    detail.latitude = 41.7195;
    detail.longitude = 123.4312;

    for (int index = 0; index < 20; ++index) {
        ChargerInfo charger;
        charger.chargerId = index + 1;
        charger.code = QStringLiteral("A-%1").arg(index + 1, 3, 10, QChar('0'));
        charger.type = index < 12 ? protocol::ChargerTypeFast : protocol::ChargerTypeSlow;
        charger.powerKw = charger.type == protocol::ChargerTypeFast ? 120.0 : 7.0;
        charger.status = protocol::ChargerStatusCharging;
        if (index == 0 || index == 2 || index == 3 || index == 4 || index == 5 ||
            index == 6 || index == 7 || index == 8) {
            charger.status = protocol::ChargerStatusIdle;
        }
        detail.chargers.append(charger);
    }
    return detail;
}

} // namespace

StationDetailPage::StationDetailPage(NetworkClient* networkClient, QWidget* parent)
    : QWidget(parent)
    , networkClient_(networkClient)
    , previewMode_(qEnvironmentVariableIsSet("EV_HOME_PREVIEW") ||
                   qEnvironmentVariableIsSet("EV_DETAIL_PREVIEW") ||
                   !qgetenv("EV_DETAIL_SCREENSHOT_PATH").isEmpty())
{
    setObjectName(QStringLiteral("StationDetailPage"));
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QStringLiteral(
        "#StationDetailPage { background-color: %1; }")
        .arg(cssColor(theme::background())));
    setMinimumSize(360, theme::detailCanvasHeight);
    buildUi();
}

void StationDetailPage::buildUi()
{
    backButton_ = new BackButton(this);
    backButton_->setAccessibleName(QStringLiteral("返回附近充电站"));
    connect(backButton_, &QPushButton::clicked, this, &StationDetailPage::backRequested);

    pageTitle_ = new QLabel(QStringLiteral("站点详情"), this);
    pageTitle_->setAlignment(Qt::AlignCenter);
    pageTitle_->setFont(theme::brandSubtitleFont());
    pageTitle_->setStyleSheet(QStringLiteral(
        "color: %1; background: transparent;").arg(cssColor(theme::textPrimary())));

    stationInfoCard_ = new QFrame(this);
    stationInfoCard_->setObjectName(QStringLiteral("StationInfoCard"));
    stationInfoCard_->setStyleSheet(QStringLiteral(
        "QFrame#StationInfoCard { background-color: %1; border: 1px solid %2;"
        " border-radius: 17px; }")
        .arg(cssRgba(theme::cardFill(), 235), cssColor(theme::cardBorder())));

    stationNameLabel_ = new QLabel(QStringLiteral("站点详情"), stationInfoCard_);
    QFont stationNameFont(QStringLiteral("Microsoft YaHei UI"));
    stationNameFont.setPixelSize(29);
    stationNameFont.setWeight(QFont::DemiBold);
    stationNameLabel_->setFont(stationNameFont);
    stationNameLabel_->setStyleSheet(QStringLiteral(
        "color: %1; background: transparent;").arg(cssColor(theme::textPrimary())));
    stationNameLabel_->setMinimumWidth(0);

    ratingWidget_ = new RatingIcon(stationInfoCard_);
    ratingLabel_ = new QLabel(QStringLiteral("4.8"), stationInfoCard_);
    ratingLabel_->setFont(theme::buttonFont());
    ratingLabel_->setStyleSheet(QStringLiteral(
        "color: %1; background: transparent;").arg(cssColor(theme::primaryBlue())));

    primaryTag_ = new TagPill(QStringLiteral("快充为主"), true, stationInfoCard_);
    hoursTag_ = new TagPill(QStringLiteral("24小时"), false, stationInfoCard_);
    openTag_ = new TagPill(QStringLiteral("营业中"), false, stationInfoCard_);

    addressIcon_ = new PinIcon(stationInfoCard_);
    addressLabel_ = new QLabel(QStringLiteral("地址加载中..."), stationInfoCard_);
    QFont addressFont(QStringLiteral("Microsoft YaHei UI"));
    addressFont.setPixelSize(14);
    addressLabel_->setFont(addressFont);
    addressLabel_->setStyleSheet(QStringLiteral(
        "color: %1; background: transparent;").arg(cssColor(theme::textPrimary())));

    distanceIcon_ = new DirectionIcon(stationInfoCard_);
    distanceLabel_ = new QLabel(QStringLiteral("-- km"), stationInfoCard_);
    distanceLabel_->setFont(theme::inputFont());
    distanceLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    distanceLabel_->setStyleSheet(QStringLiteral(
        "color: %1; background: transparent;").arg(cssColor(theme::textPrimary())));

    statsCard_ = new QFrame(stationInfoCard_);
    statsCard_->setObjectName(QStringLiteral("StatsCard"));
    statsCard_->setStyleSheet(QStringLiteral(
        "QFrame#StatsCard { background-color: %1; border: 1px solid %2;"
        " border-radius: 17px; }")
        .arg(cssRgba(theme::inputFill(), 220), cssColor(theme::cardBorder())));

    priceLabel_ = new QLabel(QStringLiteral("¥-- /度"), statsCard_);
    priceLabel_->setTextFormat(Qt::RichText);
    billingLabel_ = new QLabel(statsCard_);
    billingLabel_->setTextFormat(Qt::RichText);
    statsHorizontalDivider_ = new QFrame(statsCard_);
    statsHorizontalDivider_->setObjectName(QStringLiteral("StatsHorizontalDivider"));
    statsHorizontalDivider_->setStyleSheet(QStringLiteral(
        "QFrame#StatsHorizontalDivider { background-color: %1; }")
        .arg(cssColor(theme::cardBorder())));

    const QList<QString> statTitles = {QStringLiteral("总桩"), QStringLiteral("空闲"),
                                       QStringLiteral("快充"), QStringLiteral("慢充")};
    const QList<QLabel**> statOutputs = {
        &totalStatLabel_, &idleStatLabel_, &fastStatLabel_, &slowStatLabel_};
    for (int index = 0; index < statTitles.size(); ++index) {
        auto* caption = new QLabel(statTitles.at(index), statsCard_);
        caption->setAlignment(Qt::AlignCenter);
        caption->setStyleSheet(QStringLiteral(
            "font-size: 16px; color: %1; background: transparent;")
            .arg(cssColor(theme::textPrimary())));
        statCaptions_.append(caption);

        *statOutputs.at(index) = new QLabel(QStringLiteral("--"), statsCard_);
        (*statOutputs.at(index))->setAlignment(Qt::AlignCenter);
        (*statOutputs.at(index))->setTextFormat(Qt::RichText);
        (*statOutputs.at(index))->setStyleSheet(QStringLiteral(
            "font-size: 28px; font-weight: 700; color: %1; background: transparent;")
            .arg(cssColor(theme::textPrimary())));
    }
    for (int index = 0; index < 3; ++index) {
        auto* divider = new QFrame(statsCard_);
        divider->setObjectName(QStringLiteral("StatsVerticalDivider"));
        divider->setStyleSheet(QStringLiteral(
            "QFrame#StatsVerticalDivider { background-color: %1; }")
            .arg(cssColor(theme::cardBorder())));
        statDividers_.append(divider);
    }

    listCard_ = new QFrame(this);
    listCard_->setObjectName(QStringLiteral("ChargerListCard"));
    listCard_->setStyleSheet(QStringLiteral(
        "QFrame#ChargerListCard { background-color: %1; border: 1px solid %2;"
        " border-radius: 17px; }")
        .arg(cssRgba(theme::cardFill(), 235), cssColor(theme::cardBorder())));

    listTitleLabel_ = new QLabel(QStringLiteral("充电桩列表"), listCard_);
    listTitleLabel_->setFont(theme::brandSubtitleFont());
    listTitleLabel_->setStyleSheet(QStringLiteral(
        "color: %1; background: transparent;").arg(cssColor(theme::textPrimary())));

    const QList<QString> filterTitles = {QStringLiteral("全部"), QStringLiteral("快充"),
                                         QStringLiteral("慢充"), QStringLiteral("空闲")};
    for (const QString& title : filterTitles) {
        auto* button = new QPushButton(title, listCard_);
        button->setCheckable(true);
        button->setCursor(Qt::PointingHandCursor);
        filterButtons_.append(button);
        connect(button, &QPushButton::clicked,
                this, &StationDetailPage::onFilterClicked);
    }

    chargerScroll_ = new QScrollArea(listCard_);
    chargerScroll_->setFrameShape(QFrame::NoFrame);
    chargerScroll_->setWidgetResizable(true);
    chargerScroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    chargerScroll_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    chargerScroll_->setStyleSheet(QStringLiteral(
        "QScrollArea { background: transparent; border: none; }"
        "QWidget { background: transparent; }"));
    chargerContainer_ = new QWidget(chargerScroll_);
    chargerListLayout_ = new QVBoxLayout(chargerContainer_);
    chargerListLayout_->setContentsMargins(0, 0, 0, 0);
    chargerListLayout_->setSpacing(6);
    chargerScroll_->setWidget(chargerContainer_);

    favoriteButton_ = new ActionButton(ActionButton::Kind::Favorite, this);
    favoriteButton_->setText(QStringLiteral("收藏"));
    connect(favoriteButton_, &QPushButton::clicked,
            this, &StationDetailPage::onFavoriteClicked);

    navigateButton_ = new ActionButton(ActionButton::Kind::Navigate, this);
    navigateButton_->setText(QStringLiteral("导航"));
    navigateButton_->setStyleSheet(QStringLiteral(
        "QPushButton { color: white; border: 1px solid %1; border-radius: 18px;"
        " background: %2; font-size: 22px; font-weight: 600; padding-left: 22px; }"
        "QPushButton:hover { background: %3; }")
        .arg(cssColor(theme::primaryBlueHover()), cssColor(theme::primaryBlue()),
             cssColor(theme::primaryBlueHover())));
    navigateButton_->setEnabled(false);
    connect(navigateButton_, &QPushButton::clicked,
            this, &StationDetailPage::onNavigateClicked);

    updateFilterButtons();
    layoutUi();
}

void StationDetailPage::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.fillRect(rect(), theme::background());
}

void StationDetailPage::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    layoutUi();
}

void StationDetailPage::layoutUi()
{
    if (width() <= 0 || height() <= 0) return;

    const qreal sx = static_cast<qreal>(width()) / theme::loginCanvasWidth;
    const qreal sy = static_cast<qreal>(height()) / theme::detailCanvasHeight;
    auto X = [sx](qreal value) { return qRound(value * sx); };
    auto Y = [sy](qreal value) { return qRound(value * sy); };

    backButton_->setGeometry(X(12), Y(theme::detailHeaderTop), X(36), Y(36));
    pageTitle_->setGeometry(X(135), Y(theme::detailHeaderTop), X(201), Y(36));

    const int side = X(14);
    const int panelWidth = width() - side * 2;
    stationInfoCard_->setGeometry(side, Y(theme::detailInfoTop), panelWidth,
                                  Y(theme::detailInfoHeight));
    const int infoWidth = stationInfoCard_->width();
    stationNameLabel_->setGeometry(X(20), Y(10), qMax(0, infoWidth - X(140)), Y(40));
    ratingWidget_->setGeometry(infoWidth - X(87), Y(20), X(25), Y(25));
    ratingLabel_->setGeometry(infoWidth - X(57), Y(17), X(44), Y(34));
    primaryTag_->setGeometry(X(18), Y(53), X(104), Y(25));
    hoursTag_->setGeometry(X(134), Y(53), X(72), Y(25));
    openTag_->setGeometry(X(218), Y(53), X(76), Y(25));

    addressIcon_->setGeometry(X(20), Y(87), X(25), Y(27));
    addressLabel_->setGeometry(X(52), Y(84), qMax(0, infoWidth - X(160)), Y(33));
    distanceIcon_->setGeometry(infoWidth - X(105), Y(88), X(25), Y(25));
    distanceLabel_->setGeometry(infoWidth - X(77), Y(84), X(62), Y(33));

    statsCard_->setGeometry(X(12), Y(120), infoWidth - X(24), Y(157));
    const int statsWidth = statsCard_->width();
    priceLabel_->setGeometry(X(16), Y(11), X(210), Y(38));
    billingLabel_->setGeometry(statsWidth - X(191), Y(14), X(175), Y(30));
    statsHorizontalDivider_->setGeometry(X(16), Y(60), statsWidth - X(32), Y(1));

    const int cellsLeft = X(16);
    const int cellsWidth = statsWidth - X(32);
    const int cellWidth = cellsWidth / 4;
    const QList<QLabel*> values = {totalStatLabel_, idleStatLabel_,
                                   fastStatLabel_, slowStatLabel_};
    for (int index = 0; index < 4; ++index) {
        const int left = cellsLeft + index * cellWidth;
        statCaptions_[index]->setGeometry(left, Y(70), cellWidth, Y(25));
        values[index]->setGeometry(left, Y(94), cellWidth, Y(47));
        if (index < statDividers_.size()) {
            statDividers_[index]->setGeometry(left + cellWidth, Y(70), X(1), Y(64));
        }
    }

    listCard_->setGeometry(side, Y(theme::detailListTop), panelWidth,
                           Y(theme::detailListHeight));
    const int listWidth = listCard_->width();
    listTitleLabel_->setGeometry(X(18), Y(8), listWidth - X(36), Y(34));
    const QList<int> filterWidths = {69, 66, 66, 66};
    int filterLeft = X(18);
    for (int index = 0; index < filterButtons_.size(); ++index) {
        filterButtons_[index]->setGeometry(filterLeft, Y(42),
                                           X(filterWidths[index]), Y(26));
        filterLeft += X(filterWidths[index] + 12);
    }
    chargerScroll_->setGeometry(X(14), Y(79), listWidth - X(28), Y(270));
    favoriteButton_->setGeometry(side, Y(theme::detailActionsTop), X(150),
                                 Y(theme::detailActionsHeight));
    navigateButton_->setGeometry(X(176), Y(theme::detailActionsTop),
                                 width() - X(176) - side, Y(theme::detailActionsHeight));
}

void StationDetailPage::openStation(qint64 stationId)
{
    currentStationId_ = stationId;
    currentStationName_ = QStringLiteral("站点 #%1").arg(stationId);
    currentAddress_.clear();
    stationLatitude_ = 0.0;
    stationLongitude_ = 0.0;
    currentPricePerKwh_ = 0.0;
    currentChargers_.clear();
    filterMode_ = FilterAll;
    updateFilterButtons();
    stationNameLabel_->setText(currentStationName_);
    addressLabel_->setText(QStringLiteral("地址加载中..."));
    distanceLabel_->setText(QStringLiteral("-- km"));
    navigateButton_->setEnabled(false);
    updateFavoriteButton();

    if (previewMode_) {
        Session::instance().setLocation(
            41.7087, 123.4312, QStringLiteral("预览定位"));
        renderDetail(previewDetail());
        return;
    }
    requestStationDetail();
}

void StationDetailPage::refresh()
{
    if (currentStationId_ > 0) requestStationDetail();
}

void StationDetailPage::requestStationDetail()
{
    if (!networkClient_) {
        showErrorState(QStringLiteral("网络模块未初始化"));
        return;
    }

    showLoadingState();
    QJsonObject data;
    data.insert(QStringLiteral("stationId"), currentStationId_);
    networkClient_->sendRequest(
        QString::fromUtf8(protocol::action::kStationDetail), data,
        [this](const protocol::Response& response) {
            if (!response.isOk()) {
                showErrorState(protocol::describeError(response.code,
                                                       response.message));
                return;
            }

            const QJsonObject payload = response.data;
            StationDetail detail;
            detail.station = StationInfo::fromJson(payload);
            detail.address = payload.value(QStringLiteral("address")).toString();
            detail.latitude = payload.value(QStringLiteral("latitude")).toDouble();
            detail.longitude = payload.value(QStringLiteral("longitude")).toDouble();
            for (const QJsonValue& value :
                 payload.value(QStringLiteral("chargers")).toArray()) {
                const ChargerInfo charger = ChargerInfo::fromJson(value.toObject());
                if (charger.valid()) detail.chargers.append(charger);
            }
            renderDetail(detail);
        });
}

void StationDetailPage::renderDetail(const StationDetail& detail)
{
    if (!detail.station.name.isEmpty()) currentStationName_ = detail.station.name;
    currentAddress_ = detail.address;
    stationNameLabel_->setText(currentStationName_);
    addressLabel_->setText(currentAddress_.isEmpty()
                                ? QStringLiteral("地址暂未提供") : currentAddress_);
    currentPricePerKwh_ = detail.station.pricePerKwh;
    stationLatitude_ = detail.latitude;
    stationLongitude_ = detail.longitude;
    currentChargers_ = detail.chargers;
    ratingLabel_->setText(QStringLiteral("4.8"));

    int fastCount = 0;
    int slowCount = 0;
    for (const ChargerInfo& charger : currentChargers_) {
        if (charger.type == protocol::ChargerTypeFast) ++fastCount;
        else ++slowCount;
    }
    static_cast<TagPill*>(primaryTag_)->setText(
        fastCount >= slowCount ? QStringLiteral("快充为主") : QStringLiteral("慢充为主"));

    const double distanceKm = computeDistanceKm();
    distanceLabel_->setText(distanceKm >= 0.0
                                ? QStringLiteral("%1 km").arg(distanceKm, 0, 'f', 1)
                                : QStringLiteral("-- km"));
    updateStats(detail);
    updateFavoriteButton();
    navigateButton_->setEnabled(
        Session::instance().hasLocation() &&
        (stationLatitude_ != 0.0 || stationLongitude_ != 0.0));
    renderChargerList();
}

void StationDetailPage::updateStats(const StationDetail& detail)
{
    priceLabel_->setText(QStringLiteral(
        "<span style='color:%1; font-size:31px; font-weight:700;'>¥%2</span>"
        "<span style='color:%3; font-size:18px; font-weight:600;'> /度</span>")
        .arg(cssColor(theme::primaryBlue()))
        .arg(detail.station.pricePerKwh, 0, 'f', 2)
        .arg(cssColor(theme::textPrimary())));
    billingLabel_->setText(QStringLiteral(
        "<span style='color:%1; font-size:14px;'>当前计费时段&nbsp;&nbsp;</span>"
        "<span style='color:%2; font-size:15px;'>00:00-24:00</span>")
        .arg(cssColor(theme::textSecondary()), cssColor(theme::primaryBlue())));

    int fastCount = 0;
    int slowCount = 0;
    int idleCount = 0;
    for (const ChargerInfo& charger : currentChargers_) {
        if (charger.type == protocol::ChargerTypeFast) ++fastCount;
        else ++slowCount;
        if (charger.isIdle()) ++idleCount;
    }
    const int total = detail.station.totalChargers > 0
        ? detail.station.totalChargers : currentChargers_.size();
    const int available = detail.station.availableChargers > 0
        ? detail.station.availableChargers : idleCount;
    const QList<QPair<QLabel*, QPair<int, QColor>>> values = {
        {totalStatLabel_, {total, theme::primaryBlue()}},
        {idleStatLabel_, {available, theme::success()}},
        {fastStatLabel_, {fastCount, theme::textPrimary()}},
        {slowStatLabel_, {slowCount, theme::textPrimary()}},
    };
    for (const auto& entry : values) {
        entry.first->setText(QStringLiteral(
            "<span style='color:%1; font-size:28px; font-weight:700;'>%2</span>"
            "<span style='color:%3; font-size:15px;'> 台</span>")
            .arg(cssColor(entry.second.second))
            .arg(entry.second.first)
            .arg(cssColor(theme::textSecondary())));
    }
}

void StationDetailPage::renderChargerList()
{
    clearChargerList();
    QList<ChargerInfo> filtered;
    for (const ChargerInfo& charger : currentChargers_) {
        if (filterMode_ == FilterFast && charger.type != protocol::ChargerTypeFast) continue;
        if (filterMode_ == FilterSlow && charger.type != protocol::ChargerTypeSlow) continue;
        if (filterMode_ == FilterIdle && !charger.isIdle()) continue;
        filtered.append(charger);
    }

    if (filtered.isEmpty()) {
        showEmptyChargersState();
        return;
    }

    int displayIndex = 1;
    for (const ChargerInfo& charger : filtered) {
        auto* card = new ChargerCard(charger, displayIndex++, chargerContainer_);
        connect(card, &ChargerCard::selected, this, [this](qint64 chargerId) {
            for (const ChargerInfo& charger : currentChargers_) {
                if (charger.chargerId == chargerId) {
                    emit chargerSelected(charger, currentStationName_, currentPricePerKwh_);
                    return;
                }
            }
        });
        chargerListLayout_->addWidget(card);
    }
    chargerListLayout_->addStretch(1);
}

void StationDetailPage::onFilterClicked()
{
    auto* button = qobject_cast<QPushButton*>(sender());
    const int index = filterButtons_.indexOf(button);
    if (index < 0) return;
    filterMode_ = index;
    updateFilterButtons();
    renderChargerList();
}

void StationDetailPage::updateFilterButtons()
{
    for (int index = 0; index < filterButtons_.size(); ++index) {
        const bool active = index == filterMode_;
        filterButtons_[index]->setChecked(active);
        filterButtons_[index]->setStyleSheet(QString(
            "QPushButton { background-color: %1; color: %2; border: 1px solid %3;"
            " border-radius: 15px; font-size: 16px; }"
            "QPushButton:hover { border-color: %4; }")
            .arg(active ? cssRgba(theme::primaryBlue(), 80)
                        : cssRgba(theme::cardFill(), 140))
            .arg(cssColor(active ? theme::textPrimary() : theme::textSecondary()))
            .arg(cssColor(active ? theme::primaryBlue() : theme::cardBorder()))
            .arg(cssColor(theme::primaryBlue())));
    }
}

void StationDetailPage::showLoadingState()
{
    clearChargerList();
    auto* label = new QLabel(QStringLiteral("正在加载站内充电桩..."), chargerContainer_);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet(QStringLiteral(
        "color: %1; background: transparent; font-size: 16px; padding: 40px 0;")
        .arg(cssColor(theme::textSecondary())));
    chargerListLayout_->addWidget(label);
    chargerListLayout_->addStretch(1);
}

void StationDetailPage::showErrorState(const QString& message)
{
    clearChargerList();
    auto* title = new QLabel(QStringLiteral("站点详情加载失败"), chargerContainer_);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(QStringLiteral(
        "color: %1; background: transparent; font-size: 17px; font-weight: 600;")
        .arg(cssColor(theme::textPrimary())));
    auto* detail = new QLabel(message, chargerContainer_);
    detail->setAlignment(Qt::AlignCenter);
    detail->setWordWrap(true);
    detail->setStyleSheet(QStringLiteral(
        "color: %1; background: transparent; font-size: 14px;")
        .arg(cssColor(theme::textSecondary())));
    auto* retry = new QPushButton(QStringLiteral("重试"), chargerContainer_);
    retry->setCursor(Qt::PointingHandCursor);
    retry->setStyleSheet(QStringLiteral(
        "QPushButton { color: %1; border: 1px solid %1; border-radius: 14px;"
        " padding: 4px 20px; background: transparent; }")
        .arg(cssColor(theme::primaryBlue())));
    connect(retry, &QPushButton::clicked,
            this, &StationDetailPage::requestStationDetail);
    chargerListLayout_->addSpacing(28);
    chargerListLayout_->addWidget(title);
    chargerListLayout_->addWidget(detail);
    chargerListLayout_->addWidget(retry, 0, Qt::AlignHCenter);
    chargerListLayout_->addStretch(1);
}

void StationDetailPage::showEmptyChargersState()
{
    clearChargerList();
    auto* label = new QLabel(QStringLiteral("没有符合筛选条件的充电桩"), chargerContainer_);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet(QStringLiteral(
        "color: %1; background: transparent; font-size: 16px; padding: 42px 0;")
        .arg(cssColor(theme::textSecondary())));
    chargerListLayout_->addWidget(label);
    chargerListLayout_->addStretch(1);
}

void StationDetailPage::clearChargerList()
{
    while (QLayoutItem* item = chargerListLayout_->takeAt(0)) {
        if (QWidget* widget = item->widget()) delete widget;
        delete item;
    }
}

double StationDetailPage::computeDistanceKm() const
{
    const Session& session = Session::instance();
    if (!session.hasLocation() ||
        (stationLatitude_ == 0.0 && stationLongitude_ == 0.0)) {
        return -1.0;
    }
    constexpr double kEarthRadiusKm = 6371.0;
    const double lat1 = qDegreesToRadians(session.latitude());
    const double lat2 = qDegreesToRadians(stationLatitude_);
    const double dLat = qDegreesToRadians(stationLatitude_ - session.latitude());
    const double dLng = qDegreesToRadians(stationLongitude_ - session.longitude());
    const double a = std::sin(dLat / 2.0) * std::sin(dLat / 2.0) +
                     std::cos(lat1) * std::cos(lat2) *
                     std::sin(dLng / 2.0) * std::sin(dLng / 2.0);
    return kEarthRadiusKm * 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
}

bool StationDetailPage::isFavorite() const
{
    if (currentStationId_ <= 0) return false;
    QSettings settings;
    return settings.value(QStringLiteral("favorites/stationIds")).toList()
        .contains(QVariant::fromValue(currentStationId_));
}

void StationDetailPage::setFavorite(bool favorite)
{
    if (currentStationId_ <= 0) return;
    QSettings settings;
    QVariantList ids = settings.value(QStringLiteral("favorites/stationIds")).toList();
    const QVariant id = QVariant::fromValue(currentStationId_);
    if (favorite) {
        if (!ids.contains(id)) ids.append(id);
    } else {
        ids.removeAll(id);
    }
    settings.setValue(QStringLiteral("favorites/stationIds"), ids);
}

void StationDetailPage::updateFavoriteButton()
{
    const bool favorite = isFavorite();
    favoriteButton_->setText(favorite ? QStringLiteral("已收藏")
                                      : QStringLiteral("收藏"));
    static_cast<ActionButton*>(favoriteButton_)->setFavoriteState(favorite);
}

void StationDetailPage::onFavoriteClicked()
{
    setFavorite(!isFavorite());
    updateFavoriteButton();
}

void StationDetailPage::onNavigateClicked()
{
    if (!Session::instance().hasLocation() ||
        (stationLatitude_ == 0.0 && stationLongitude_ == 0.0)) {
        return;
    }
    emit navigationRequested(stationLatitude_, stationLongitude_,
                             currentStationName_, false);
}

StationDetail StationDetailPage::previewDetail() const
{
    return makePreviewDetail();
}
