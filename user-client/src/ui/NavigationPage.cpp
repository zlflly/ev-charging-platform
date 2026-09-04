#include "ui/NavigationPage.h"

#include "config/AppConfig.h"
#include "geo/Geocoder.h"
#include "net/NetworkClient.h"
#include "session/Session.h"
#include "ui/SvgIcon.h"
#include "ui/theme/Theme.h"

#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QFile>
#include <QFrame>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QPushButton>
#include <QResizeEvent>
#include <QTimer>
#include <QUrl>
#include <QWebEnginePage>
#include <QWebEngineView>
#include <QMouseEvent>
#include <QtMath>

#include <cmath>
#include <limits>
#include <QStringList>

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
    return svg::tintedPixmap(resourcePath, size, color);
}

void setLabelStyle(QLabel* label, int pixelSize, const QColor& color,
                   bool bold = false, Qt::Alignment alignment = Qt::AlignLeft | Qt::AlignVCenter)
{
    QFont font(QStringLiteral("Microsoft YaHei UI"));
    font.setPixelSize(pixelSize);
    font.setWeight(bold ? QFont::DemiBold : QFont::Normal);
    label->setFont(font);
    label->setAlignment(alignment);
    label->setStyleSheet(QStringLiteral(
        "color: %1; background: transparent;").arg(cssColor(color)));
}

void stylePanel(QFrame* panel, int radius = 18, int alpha = 235)
{
    panel->setObjectName(QStringLiteral("NavigationPanel"));
    panel->setStyleSheet(QStringLiteral(
        "QFrame#NavigationPanel { background-color: %1; border: 1px solid %2;"
        " border-radius: %3px; }")
        .arg(cssRgba(theme::cardFill(), alpha), cssColor(theme::cardBorder()))
        .arg(radius));
}

void stylePrimaryButton(QPushButton* button, int fontSize = 22)
{
    button->setCursor(Qt::PointingHandCursor);
    button->setStyleSheet(QStringLiteral(
        "QPushButton { color: white; background-color: %1; border: 1px solid %2;"
        " border-radius: 24px; font-size: %3px; font-weight: 600; }"
        "QPushButton:hover { background-color: %4; }"
        "QPushButton:pressed { background-color: %5; }"
        "QPushButton:disabled { color: %6; background-color: %7; border-color: transparent; }")
        .arg(cssColor(theme::primaryBlue()), cssColor(QColor(0x58, 0xB6, 0xFF))
             , QString::number(fontSize), cssColor(theme::primaryBlueHover()),
             cssColor(theme::primaryBluePressed()), cssColor(theme::textMuted()),
             cssColor(QColor(0x12, 0x2B, 0x53))));
}

void styleSecondaryButton(QPushButton* button, int fontSize = 18)
{
    button->setCursor(Qt::PointingHandCursor);
    button->setStyleSheet(QStringLiteral(
        "QPushButton { color: %1; background-color: %2; border: 1px solid %3;"
        " border-radius: 20px; font-size: %4px; font-weight: 600; }"
        "QPushButton:hover { border-color: %5; background-color: %6; }")
        .arg(cssColor(theme::textPrimary()), cssRgba(theme::cardFill(), 210),
             cssColor(theme::cardBorder()), QString::number(fontSize),
             cssColor(theme::primaryBlue()), cssRgba(theme::primaryBlue(), 32)));
}

void styleLocationEdit(QLineEdit* edit)
{
    QFont font(QStringLiteral("Microsoft YaHei UI"));
    font.setPixelSize(15);
    font.setWeight(QFont::DemiBold);
    edit->setFont(font);
    edit->setFrame(false);
    edit->setStyleSheet(QStringLiteral(
        "QLineEdit { color: %1; background: %2; border: 1px solid %3;"
        " border-radius: 10px; padding: 0 10px; }"
        "QLineEdit:focus { border-color: %4; background: %5; }")
        .arg(cssColor(theme::textPrimary()), cssRgba(theme::inputFill(), 220),
             cssColor(theme::inputBorder()), cssColor(theme::primaryBlue()),
             cssRgba(theme::inputFill(), 245)));
}

class NavIconButton : public QPushButton
{
public:
    enum class Kind { Back, Close, Share, Locate, ZoomIn, ZoomOut, Layers, Swap };

    NavIconButton(Kind kind, QWidget* parent = nullptr)
        : QPushButton(parent), kind_(kind)
    {
        setCursor(Qt::PointingHandCursor);
        setStyleSheet(QStringLiteral("background: transparent; border: none;"));
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const bool mapButton = kind_ == Kind::Locate || kind_ == Kind::ZoomIn ||
                               kind_ == Kind::ZoomOut || kind_ == Kind::Layers;
        if (mapButton) {
            QColor fill = theme::cardFill();
            fill.setAlpha(225);
            painter.setBrush(fill);
            painter.setPen(QPen(theme::cardBorder(), 1));
            painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 13, 13);
        }

        const QColor color = kind_ == Kind::Close ? theme::textPrimary()
                                                   : theme::textPrimary();

        QString iconPath;
        switch (kind_) {
        case Kind::Back: iconPath = QStringLiteral(":/resources/icons/back.svg"); break;
        case Kind::Locate: iconPath = QStringLiteral(":/resources/icons/locate.svg"); break;
        case Kind::ZoomIn: iconPath = QStringLiteral(":/resources/icons/plus.svg"); break;
        case Kind::ZoomOut: iconPath = QStringLiteral(":/resources/icons/minus.svg"); break;
        case Kind::Layers: iconPath = QStringLiteral(":/resources/icons/overview.svg"); break;
        default: break;
        }
        if (!iconPath.isEmpty()) {
            const QPixmap icon = tintedSvgPixmap(iconPath,
                                                 QSize(qMin(width(), height()) - 12,
                                                       qMin(width(), height()) - 12),
                                                 color);
            if (!icon.isNull()) {
                painter.drawPixmap((width() - icon.width()) / 2,
                                   (height() - icon.height()) / 2, icon);
                return;
            }
        }

        painter.setPen(QPen(color, 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);
        const qreal cx = width() / 2.0;
        const qreal cy = height() / 2.0;

        switch (kind_) {
        case Kind::Back:
            painter.drawLine(QPointF(25, cy), QPointF(10, cy));
            painter.drawLine(QPointF(10, cy), QPointF(21, cy - 11));
            painter.drawLine(QPointF(10, cy), QPointF(21, cy + 11));
            break;
        case Kind::Close:
            painter.drawLine(QPointF(10, 10), QPointF(width() - 10, height() - 10));
            painter.drawLine(QPointF(width() - 10, 10), QPointF(10, height() - 10));
            break;
        case Kind::Share:
            painter.drawRoundedRect(QRectF(7, 10, 22, 21), 4, 4);
            painter.drawLine(QPointF(17, 17), QPointF(30, 6));
            painter.drawLine(QPointF(30, 6), QPointF(30, 15));
            painter.drawLine(QPointF(30, 6), QPointF(21, 6));
            break;
        case Kind::Locate:
            painter.drawEllipse(QPointF(cx, cy), 9, 9);
            painter.drawEllipse(QPointF(cx, cy), 3, 3);
            painter.drawLine(QPointF(cx, 6), QPointF(cx, 13));
            painter.drawLine(QPointF(cx, height() - 6), QPointF(cx, height() - 13));
            painter.drawLine(QPointF(6, cy), QPointF(13, cy));
            painter.drawLine(QPointF(width() - 6, cy), QPointF(width() - 13, cy));
            break;
        case Kind::ZoomIn:
            painter.drawLine(QPointF(cx - 8, cy), QPointF(cx + 8, cy));
            painter.drawLine(QPointF(cx, cy - 8), QPointF(cx, cy + 8));
            break;
        case Kind::ZoomOut:
            painter.drawLine(QPointF(cx - 8, cy), QPointF(cx + 8, cy));
            break;
        case Kind::Layers:
            for (int offset : { -5, 0, 5 }) {
                QPainterPath layer;
                layer.moveTo(cx - 13, cy + offset);
                layer.lineTo(cx, cy + offset - 7);
                layer.lineTo(cx + 13, cy + offset);
                layer.lineTo(cx, cy + offset + 7);
                layer.closeSubpath();
                painter.drawPath(layer);
            }
            break;
        case Kind::Swap:
            painter.drawLine(QPointF(cx - 7, 8), QPointF(cx - 7, height() - 8));
            painter.drawLine(QPointF(cx - 7, 8), QPointF(cx - 12, 13));
            painter.drawLine(QPointF(cx - 7, 8), QPointF(cx - 2, 13));
            painter.drawLine(QPointF(cx + 7, height() - 8), QPointF(cx + 7, 8));
            painter.drawLine(QPointF(cx + 7, height() - 8), QPointF(cx + 2, height() - 13));
            painter.drawLine(QPointF(cx + 7, height() - 8), QPointF(cx + 12, height() - 13));
            break;
        }
    }

private:
    Kind kind_;
};

class TurnArrowWidget : public QFrame
{
public:
    explicit TurnArrowWidget(QWidget* parent = nullptr) : QFrame(parent)
    {
        setStyleSheet(QStringLiteral("background: transparent; border: none;"));
    }

    void setInstruction(const QString& instruction)
    {
        instruction_ = instruction;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        if (instruction_.contains(QStringLiteral("到达"))) {
            painter.setPen(QPen(theme::success(), 3));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(rect().adjusted(10, 10, -10, -10));
            painter.setPen(QPen(theme::success(), 4, Qt::SolidLine,
                                Qt::RoundCap, Qt::RoundJoin));
            painter.drawLine(QPointF(width() * .31, height() * .51),
                             QPointF(width() * .45, height() * .66));
            painter.drawLine(QPointF(width() * .45, height() * .66),
                             QPointF(width() * .72, height() * .36));
            return;
        }

        const qreal centerX = width() * 0.50;
        const qreal centerY = height() * 0.36;
        const qreal leftX = width() * 0.28;
        const qreal rightX = width() * 0.82;
        painter.setPen(QPen(theme::textPrimary(), 9, Qt::SolidLine,
                            Qt::RoundCap, Qt::RoundJoin));
        if (instruction_.contains(QStringLiteral("直行")) ||
            instruction_.contains(QStringLiteral("沿")) ||
            instruction_.contains(QStringLiteral("行驶"))) {
            painter.drawLine(QPointF(centerX, height() * .82),
                             QPointF(centerX, height() * .24));
            painter.setPen(Qt::NoPen);
            painter.setBrush(theme::textPrimary());
            QPolygonF arrow;
            arrow << QPointF(centerX, height() * .10)
                  << QPointF(centerX - 13, height() * .30)
                  << QPointF(centerX + 13, height() * .30);
            painter.drawPolygon(arrow);
            return;
        }

        const bool leftTurn = instruction_.contains(QStringLiteral("左转")) ||
                              instruction_.contains(QStringLiteral("向左"));
        const qreal turnX = leftTurn ? rightX : leftX;
        const qreal exitX = leftTurn ? leftX : rightX;
        painter.drawLine(QPointF(turnX, height() * 0.82),
                         QPointF(turnX, centerY));
        painter.drawLine(QPointF(turnX, centerY), QPointF(exitX, centerY));
        painter.setPen(Qt::NoPen);
        painter.setBrush(theme::textPrimary());
        QPolygonF arrow;
        const qreal direction = leftTurn ? -1.0 : 1.0;
        arrow << QPointF(exitX + direction * 13, centerY)
              << QPointF(exitX - direction * 3, centerY - 13)
              << QPointF(exitX - direction * 3, centerY + 13);
        painter.drawPolygon(arrow);
    }

private:
    QString instruction_;
};

class ArrivalCard : public QFrame
{
public:
    explicit ArrivalCard(QWidget* parent = nullptr) : QFrame(parent)
    {
        setObjectName(QStringLiteral("ArrivalCard"));
        setStyleSheet(QStringLiteral(
            "QFrame#ArrivalCard { border: 1px solid %1; border-radius: 18px; }")
            .arg(cssColor(theme::cardBorder())));
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QPixmap source(QStringLiteral(":/resources/images/navigation_arrival_generated.png"));
        if (!source.isNull()) {
            const QPixmap scaled = source.scaled(size(), Qt::KeepAspectRatioByExpanding,
                                                 Qt::SmoothTransformation);
            painter.drawPixmap((width() - scaled.width()) / 2,
                               (height() - scaled.height()) / 2, scaled);
        } else {
            painter.fillRect(rect(), theme::cardFill());
        }
        QLinearGradient overlay(0, 0, 0, height());
        overlay.setColorAt(0.0, QColor(4, 13, 27, 125));
        overlay.setColorAt(0.45, QColor(4, 13, 27, 130));
        overlay.setColorAt(1.0, QColor(4, 13, 27, 248));
        painter.fillRect(rect(), overlay);
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
        painter.setPen(Qt::NoPen);
        painter.setBrush(theme::success());
        painter.drawEllipse(rect().adjusted(2, 2, -2, -2));
        painter.setPen(QPen(theme::background(), 3, Qt::SolidLine,
                            Qt::RoundCap, Qt::RoundJoin));
        painter.drawLine(QPointF(width() * .27, height() * .53),
                         QPointF(width() * .44, height() * .70));
        painter.drawLine(QPointF(width() * .44, height() * .70),
                         QPointF(width() * .76, height() * .34));
    }
};

class StepCard : public QFrame
{
public:
    explicit StepCard(QWidget* parent = nullptr) : QFrame(parent)
    {
        setObjectName(QStringLiteral("NavigationStepCard"));
        setStyleSheet(QStringLiteral(
            "QFrame#NavigationStepCard { background: %1; border: 1px solid %2;"
            " border-radius: 13px; }")
            .arg(cssRgba(theme::cardFill(), 205), cssColor(theme::cardBorder())));
    }
};

} // namespace

NavigationPage::NavigationPage(NetworkClient* networkClient, QWidget* parent)
    : QWidget(parent)
    , networkClient_(networkClient)
    , routePlanner_(new RoutePlanner(this))
    , geocoder_(new Geocoder(this))
{
    setObjectName(QStringLiteral("NavigationPage"));
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QStringLiteral(
        "#NavigationPage { background-color: %1; }").arg(cssColor(theme::background())));
    setMinimumSize(theme::loginCanvasWidth, theme::loginCanvasHeight);
    buildUi();

    navigationTimer_ = new QTimer(this);
    navigationTimer_->setInterval(1000);
    connect(navigationTimer_, &QTimer::timeout,
            this, &NavigationPage::onNavigationTick);
    connect(routePlanner_, &RoutePlanner::routeReady,
            this, &NavigationPage::onRouteReady);
    connect(routePlanner_, &RoutePlanner::error,
            this, &NavigationPage::onRouteError);
    connect(geocoder_, &Geocoder::geocoded,
            this, &NavigationPage::onGeocoded);
    connect(geocoder_, &Geocoder::error,
            this, &NavigationPage::onGeocoderError);
}

void NavigationPage::buildUi()
{
    topBackButton_ = new NavIconButton(NavIconButton::Kind::Back, this);
    topBackButton_->setAccessibleName(QStringLiteral("返回路线规划"));
    connect(topBackButton_, &QPushButton::clicked,
            this, &NavigationPage::onTopBackClicked);

    closeButton_ = new NavIconButton(NavIconButton::Kind::Close, this);
    closeButton_->setAccessibleName(QStringLiteral("退出导航"));
    connect(closeButton_, &QPushButton::clicked,
            this, &NavigationPage::onTopBackClicked);

    shareButton_ = new NavIconButton(NavIconButton::Kind::Share, this);
    shareButton_->setAccessibleName(QStringLiteral("分享路线"));
    connect(shareButton_, &QPushButton::clicked,
            this, &NavigationPage::onShareClicked);

    pageTitle_ = new QLabel(QStringLiteral("路线规划"), this);
    setLabelStyle(pageTitle_, 24, theme::textPrimary(), true, Qt::AlignCenter);

    routeSummaryCard_ = new QFrame(this);
    stylePanel(routeSummaryCard_, 18, 230);
    originCaptionLabel_ = new QLabel(QStringLiteral("当前位置"), routeSummaryCard_);
    destinationCaptionLabel_ = new QLabel(QStringLiteral("充电站"), routeSummaryCard_);
    setLabelStyle(originCaptionLabel_, 14, theme::textSecondary(), true,
                  Qt::AlignLeft | Qt::AlignVCenter);
    setLabelStyle(destinationCaptionLabel_, 14, theme::textSecondary(), true,
                  Qt::AlignLeft | Qt::AlignVCenter);
    originEdit_ = new QLineEdit(routeSummaryCard_);
    destinationEdit_ = new QLineEdit(routeSummaryCard_);
    originEdit_->setPlaceholderText(QStringLiteral("输入当前位置或地址"));
    destinationEdit_->setPlaceholderText(QStringLiteral("输入充电站或地址"));
    originEdit_->setAccessibleName(QStringLiteral("当前位置"));
    destinationEdit_->setAccessibleName(QStringLiteral("充电站"));
    styleLocationEdit(originEdit_);
    styleLocationEdit(destinationEdit_);
    connect(originEdit_, &QLineEdit::returnPressed,
            this, &NavigationPage::onOriginReturnPressed);
    connect(destinationEdit_, &QLineEdit::returnPressed,
            this, &NavigationPage::onDestinationReturnPressed);
    routeSummaryMetaLabel_ = new QLabel(routeSummaryCard_);
    setLabelStyle(routeSummaryMetaLabel_, 13, theme::textSecondary(), false,
                  Qt::AlignRight | Qt::AlignVCenter);
    routeSummaryMetaLabel_->setVisible(false);
    swapRouteButton_ = new NavIconButton(NavIconButton::Kind::Swap, routeSummaryCard_);
    swapRouteButton_->setToolTip(QStringLiteral("交换起终点"));
    connect(swapRouteButton_, &QPushButton::clicked, this, [this] {
        onSwapLocationsClicked();
    });

    mapFrame_ = new QFrame(this);
    mapFrame_->setObjectName(QStringLiteral("NavigationMapFrame"));
    mapFrame_->setStyleSheet(QStringLiteral(
        "QFrame#NavigationMapFrame { background: %1; border: 1px solid %2;"
        " border-radius: 18px; }")
        .arg(cssColor(theme::inputFill()), cssColor(theme::cardBorder())));
    mapView_ = new QWebEngineView(mapFrame_);
    mapView_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    mapView_->setStyleSheet(QStringLiteral("background: transparent; border: none;"));
    mapView_->setVisible(false);
    mapStateLabel_ = new QLabel(QStringLiteral("正在加载高德路线地图..."), mapFrame_);
    setLabelStyle(mapStateLabel_, 14, theme::textSecondary(), false, Qt::AlignCenter);
    mapStateLabel_->setWordWrap(true);

    recenterButton_ = new NavIconButton(NavIconButton::Kind::Locate, mapFrame_);
    zoomInButton_ = new NavIconButton(NavIconButton::Kind::ZoomIn, mapFrame_);
    zoomOutButton_ = new NavIconButton(NavIconButton::Kind::ZoomOut, mapFrame_);
    layersButton_ = new NavIconButton(NavIconButton::Kind::Layers, mapFrame_);
    recenterButton_->setAccessibleName(QStringLiteral("回到当前位置"));
    zoomInButton_->setAccessibleName(QStringLiteral("放大地图"));
    zoomOutButton_->setAccessibleName(QStringLiteral("缩小地图"));
    layersButton_->setAccessibleName(QStringLiteral("地图图层"));
    connect(recenterButton_, &QPushButton::clicked,
            this, &NavigationPage::onRecenterClicked);
    connect(zoomInButton_, &QPushButton::clicked,
            this, &NavigationPage::onZoomInClicked);
    connect(zoomOutButton_, &QPushButton::clicked,
            this, &NavigationPage::onZoomOutClicked);
    connect(layersButton_, &QPushButton::clicked,
            this, &NavigationPage::onLayersClicked);

    routeOptionsCard_ = new QFrame(this);
    stylePanel(routeOptionsCard_, 18, 230);
    routeOptionsHintLabel_ = new QLabel(QStringLiteral("路线选择"), routeOptionsCard_);
    setLabelStyle(routeOptionsHintLabel_, 13, theme::textSecondary());
    for (int index = 0; index < 3; ++index) {
        auto* option = new QPushButton(routeOptionsCard_);
        option->setCheckable(true);
        option->setCursor(Qt::PointingHandCursor);
        routeOptionButtons_.append(option);
        connect(option, &QPushButton::clicked,
                this, &NavigationPage::onRouteOptionClicked);
        auto* time = new QLabel(option);
        auto* meta = new QLabel(option);
        routeOptionTimeLabels_.append(time);
        routeOptionMetaLabels_.append(meta);
        setLabelStyle(time, 21, theme::textPrimary(), true);
        setLabelStyle(meta, 12, theme::textSecondary());
    }

    returnStationButton_ = new QPushButton(QStringLiteral("返回站点"), this);
    styleSecondaryButton(returnStationButton_);
    connect(returnStationButton_, &QPushButton::clicked,
            this, &NavigationPage::backRequested);
    startNavigationButton_ = new QPushButton(QStringLiteral("开始导航"), this);
    stylePrimaryButton(startNavigationButton_);
    connect(startNavigationButton_, &QPushButton::clicked,
            this, &NavigationPage::onStartNavigationClicked);
    planningHintLabel_ = new QLabel(QStringLiteral("电量充足，预计到达后剩余电量约 42%"), this);
    setLabelStyle(planningHintLabel_, 13, theme::textSecondary(), false, Qt::AlignCenter);

    guidanceCard_ = new QFrame(this);
    stylePanel(guidanceCard_, 18, 238);
    turnArrowWidget_ = new TurnArrowWidget(guidanceCard_);
    guidanceDistanceLabel_ = new QLabel(guidanceCard_);
    guidanceRoadLabel_ = new QLabel(guidanceCard_);
    setLabelStyle(guidanceDistanceLabel_, 17, theme::textPrimary(), false,
                  Qt::AlignLeft | Qt::AlignVCenter);
    setLabelStyle(guidanceRoadLabel_, 15, theme::textPrimary(), true);
    guidanceRoadLabel_->setWordWrap(true);
    guidanceDistanceLabel_->setTextFormat(Qt::RichText);

    navigationBottomCard_ = new QFrame(this);
    stylePanel(navigationBottomCard_, 18, 238);
    remainingDistanceLabel_ = new QLabel(navigationBottomCard_);
    remainingEtaLabel_ = new QLabel(navigationBottomCard_);
    arrivalBatteryLabel_ = new QLabel(navigationBottomCard_);
    for (QLabel* label : {remainingDistanceLabel_, remainingEtaLabel_, arrivalBatteryLabel_}) {
        setLabelStyle(label, 18, theme::textPrimary(), true, Qt::AlignCenter);
        label->setTextFormat(Qt::RichText);
    }
    stopNavigationButton_ = new QPushButton(QStringLiteral("结束导航"), navigationBottomCard_);
    stopNavigationButton_->setCursor(Qt::PointingHandCursor);
    stopNavigationButton_->setStyleSheet(QStringLiteral(
        "QPushButton { color: %1; background: transparent; border: 1px solid %2;"
        " border-radius: 22px; font-size: 20px; font-weight: 600; }"
        "QPushButton:hover { background: %3; }")
        .arg(cssColor(theme::danger()), cssColor(theme::primaryBlue()),
             cssRgba(theme::primaryBlue(), 28)));
    connect(stopNavigationButton_, &QPushButton::clicked,
            this, &NavigationPage::onStopNavigationClicked);

    finishedCard_ = new ArrivalCard(this);
    finishedStatusIcon_ = new CheckIcon(finishedCard_);
    finishedStatusLabel_ = new QLabel(QStringLiteral("已到达"), finishedCard_);
    finishedStationLabel_ = new QLabel(finishedCard_);
    finishedParkingLabel_ = new QLabel(QStringLiteral("已到达目的地"), finishedCard_);
    finishedThanksLabel_ = new QLabel(QStringLiteral("感谢使用，祝您充电顺利！"), finishedCard_);
    finishedDurationLabel_ = new QLabel(finishedCard_);
    finishedDistanceLabel_ = new QLabel(finishedCard_);
    finishedParkingMetaLabel_ = new QLabel(finishedCard_);
    setLabelStyle(finishedStatusLabel_, 29, theme::success(), true);
    setLabelStyle(finishedStationLabel_, 23, theme::textPrimary(), true);
    setLabelStyle(finishedParkingLabel_, 16, theme::textPrimary());
    setLabelStyle(finishedThanksLabel_, 14, theme::textSecondary());
    for (QLabel* label : {finishedDurationLabel_, finishedDistanceLabel_, finishedParkingMetaLabel_}) {
        setLabelStyle(label, 16, theme::textPrimary(), false, Qt::AlignCenter);
    }

    finishNextStepsCard_ = new QFrame(this);
    stylePanel(finishNextStepsCard_, 18, 230);
    nextStepsTitleLabel_ = new QLabel(QStringLiteral("到站后下一步"), finishNextStepsCard_);
    setLabelStyle(nextStepsTitleLabel_, 19, theme::textPrimary(), true);
    const QList<QPair<QString, QString>> steps = {
        {QStringLiteral("查看桩位状态"), QStringLiteral("查看充电桩实时状态\n空闲 / 使用中")},
        {QStringLiteral("选择空闲充电桩"), QStringLiteral("选择合适的空闲桩位\n快速开始充电")},
        {QStringLiteral("开始充电"), QStringLiteral("连接充电枪\n开始充电并支付")},
    };
    for (const auto& step : steps) {
        auto* card = new StepCard(finishNextStepsCard_);
        auto* title = new QLabel(step.first, card);
        auto* detail = new QLabel(step.second, card);
        setLabelStyle(title, 14, theme::textPrimary(), true, Qt::AlignCenter);
        setLabelStyle(detail, 12, theme::textSecondary(), false, Qt::AlignCenter);
        detail->setWordWrap(true);
        finishStepCards_.append(card);
        finishStepTitles_.append(title);
        finishStepDetails_.append(detail);
    }
    finishBackDetailButton_ = new QPushButton(QStringLiteral("返回站点详情"), finishNextStepsCard_);
    finishChargerListButton_ = new QPushButton(QStringLiteral("查看充电桩列表"), finishNextStepsCard_);
    styleSecondaryButton(finishBackDetailButton_, 16);
    styleSecondaryButton(finishChargerListButton_, 16);
    connect(finishBackDetailButton_, &QPushButton::clicked,
            this, &NavigationPage::backToStationRequested);
    connect(finishChargerListButton_, &QPushButton::clicked,
            this, &NavigationPage::backToStationRequested);
    chooseChargerButton_ = new QPushButton(QStringLiteral("去选桩"), this);
    stylePrimaryButton(chooseChargerButton_, 21);
    connect(chooseChargerButton_, &QPushButton::clicked,
            this, &NavigationPage::backToStationRequested);
    finishedHintLabel_ = new QLabel(QStringLiteral("导航已结束，可继续进入充电流程"), this);
    setLabelStyle(finishedHintLabel_, 13, theme::textSecondary(), false, Qt::AlignCenter);

    renderState();
}

void NavigationPage::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.fillRect(rect(), theme::background());
}

void NavigationPage::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    layoutUi();
}

void NavigationPage::layoutUi()
{
    if (width() <= 0 || height() <= 0) return;
    const qreal sx = static_cast<qreal>(width()) / theme::loginCanvasWidth;
    const qreal sy = static_cast<qreal>(height()) / theme::loginCanvasHeight;
    auto X = [sx](qreal value) { return qRound(value * sx); };
    auto Y = [sy](qreal value) { return qRound(value * sy); };

    topBackButton_->setGeometry(X(14), Y(13), X(38), Y(38));
    closeButton_->setGeometry(X(14), Y(13), X(38), Y(38));
    shareButton_->setGeometry(X(419), Y(13), X(38), Y(38));
    pageTitle_->setGeometry(X(92), Y(13), X(287), Y(36));

    routeSummaryCard_->setGeometry(X(15), Y(84), width() - X(30), Y(112));
    originCaptionLabel_->setGeometry(X(12), Y(12), X(60), Y(34));
    destinationCaptionLabel_->setGeometry(X(12), Y(58), X(60), Y(34));
    const int inputLeft = X(78);
    const int inputWidth = routeSummaryCard_->width() - inputLeft - X(56);
    originEdit_->setGeometry(inputLeft, Y(12), inputWidth, Y(34));
    destinationEdit_->setGeometry(inputLeft, Y(58), inputWidth, Y(34));
    routeSummaryMetaLabel_->setGeometry(0, 0, 0, 0);
    swapRouteButton_->setGeometry(routeSummaryCard_->width() - X(48), Y(29), X(34), Y(54));

    mapFrame_->setGeometry(state_ == Navigating ? 0 : X(15),
                           state_ == Navigating ? Y(160) : Y(207),
                           state_ == Navigating ? width() : width() - X(30),
                           state_ == Navigating ? Y(475) : Y(328));
    const QRect mapContent = mapFrame_->rect().adjusted(1, 1, -1, -1);
    mapView_->setGeometry(mapContent);
    mapStateLabel_->setGeometry(X(24), mapFrame_->height() / 2 - Y(25),
                                mapFrame_->width() - X(48), Y(50));
    const int toolRight = mapFrame_->width() - X(58);
    const int toolWidth = X(42);
    const int toolHeight = Y(42);
    const int toolGap = Y(8);
    const int firstToolY = qMax(Y(8), mapFrame_->height()
        - toolHeight * 4 - toolGap * 3 - Y(12));
    recenterButton_->setGeometry(toolRight, firstToolY, toolWidth, toolHeight);
    zoomInButton_->setGeometry(toolRight, firstToolY + toolHeight + toolGap,
                                toolWidth, toolHeight);
    zoomOutButton_->setGeometry(toolRight, firstToolY + (toolHeight + toolGap) * 2,
                                toolWidth, toolHeight);
    layersButton_->setGeometry(toolRight, firstToolY + (toolHeight + toolGap) * 3,
                               toolWidth, toolHeight);

    routeOptionsCard_->setGeometry(X(15), Y(545), width() - X(30), Y(164));
    routeOptionsHintLabel_->setGeometry(X(18), Y(9), X(150), Y(22));
    const int optionWidth = (routeOptionsCard_->width() - X(30)) / 3;
    for (int index = 0; index < routeOptionButtons_.size(); ++index) {
        const int left = X(9) + index * optionWidth;
        const int actualWidth = index == 2
            ? routeOptionsCard_->width() - X(9) - left : optionWidth - X(7);
        routeOptionButtons_[index]->setGeometry(left, Y(34), actualWidth, Y(94));
        routeOptionTimeLabels_[index]->setGeometry(X(10), Y(12), actualWidth - X(20), Y(30));
        routeOptionMetaLabels_[index]->setGeometry(X(10), Y(49), actualWidth - X(20), Y(25));
    }

    returnStationButton_->setGeometry(X(20), Y(724), X(145), Y(55));
    startNavigationButton_->setGeometry(X(176), Y(724), width() - X(196), Y(55));
    planningHintLabel_->setGeometry(X(24), Y(790), width() - X(48), Y(23));

    guidanceCard_->setGeometry(X(15), Y(45), width() - X(30), Y(110));
    turnArrowWidget_->setGeometry(X(25), Y(18), X(62), Y(68));
    guidanceDistanceLabel_->setGeometry(X(96), Y(15), width() - X(116), Y(40));
    guidanceRoadLabel_->setGeometry(X(96), Y(52), width() - X(116), Y(43));

    navigationBottomCard_->setGeometry(X(15), Y(640), width() - X(30), Y(154));
    remainingDistanceLabel_->setGeometry(X(8), Y(12), navigationBottomCard_->width() / 3 - X(8), Y(74));
    remainingEtaLabel_->setGeometry(navigationBottomCard_->width() / 3, Y(12),
                                    navigationBottomCard_->width() / 3, Y(74));
    arrivalBatteryLabel_->setGeometry(navigationBottomCard_->width() * 2 / 3, Y(12),
                                      navigationBottomCard_->width() / 3 - X(8), Y(74));
    stopNavigationButton_->setGeometry(X(15), Y(100), navigationBottomCard_->width() - X(30), Y(49));

    finishedCard_->setGeometry(X(15), Y(84), width() - X(30), Y(376));
    finishedStatusIcon_->setGeometry(X(18), Y(24), X(31), Y(31));
    finishedStatusLabel_->setGeometry(X(57), Y(17), X(130), Y(44));
    finishedStationLabel_->setGeometry(X(18), Y(75), finishedCard_->width() - X(36), Y(39));
    finishedParkingLabel_->setGeometry(X(18), Y(127), X(220), Y(28));
    finishedThanksLabel_->setGeometry(X(18), Y(161), X(300), Y(25));
    finishedDurationLabel_->setGeometry(X(15), Y(257), X(126), Y(48));
    finishedDistanceLabel_->setGeometry(X(157), Y(257), X(126), Y(48));
    finishedParkingMetaLabel_->setGeometry(X(299), Y(257), X(126), Y(48));

    finishNextStepsCard_->setGeometry(X(15), Y(475), width() - X(30), Y(329));
    nextStepsTitleLabel_->setGeometry(X(18), Y(17), X(240), Y(30));
    const int stepGap = X(10);
    const int stepWidth = (finishNextStepsCard_->width() - X(24) - stepGap * 2) / 3;
    for (int index = 0; index < finishStepCards_.size(); ++index) {
        const int left = X(12) + index * (stepWidth + stepGap);
        finishStepCards_[index]->setGeometry(left, Y(59), stepWidth, Y(142));
        finishStepTitles_[index]->setGeometry(X(5), Y(12), stepWidth - X(10), Y(29));
        finishStepDetails_[index]->setGeometry(X(6), Y(78), stepWidth - X(12), Y(48));
    }
    finishBackDetailButton_->setGeometry(X(12), Y(218),
                                         (finishNextStepsCard_->width() - X(30)) / 2, Y(45));
    finishChargerListButton_->setGeometry(
        X(18) + (finishNextStepsCard_->width() - X(30)) / 2, Y(218),
        (finishNextStepsCard_->width() - X(30)) / 2, Y(45));
    chooseChargerButton_->setGeometry(X(25), Y(748), width() - X(50), Y(54));
    finishedHintLabel_->setGeometry(X(24), Y(808), width() - X(48), Y(23));
}

void NavigationPage::openRoute(double destinationLatitude,
                               double destinationLongitude,
                               const QString& destinationName,
                               bool walking)
{
    Q_UNUSED(walking);
    navigationTimer_->stop();
    const Session& session = Session::instance();
    originLatitude_ = session.hasLocation() ? session.latitude() : 0.0;
    originLongitude_ = session.hasLocation() ? session.longitude() : 0.0;
    originName_ = currentLocationLabel();
    destinationLatitude_ = destinationLatitude;
    destinationLongitude_ = destinationLongitude;
    destinationName_ = destinationName;
    originEdit_->setText(originName_);
    destinationEdit_->setText(destinationName_);
    selectedRouteOption_ = 0;
    navigationStartedAtMs_ = 0;
    simulatedElapsedSeconds_ = 0;
    activeStepIndex_ = 0;
    navigationProgress_ = 0.0;
    currentPosition_ = QPointF();
    positionDriven_ = false;
    geocodingField_ = LocationField::None;
    route_ = RouteResult{};
    state_ = Planning;
    routeLoading_ = true;
    renderState();
    requestRoute();
}

QString NavigationPage::currentLocationLabel() const
{
    const QString label = Session::instance().locationLabel().trimmed();
    return label.isEmpty() ? QStringLiteral("当前位置") : label;
}

void NavigationPage::onOriginReturnPressed()
{
    resolveLocation(true);
}

void NavigationPage::onDestinationReturnPressed()
{
    resolveLocation(false);
}

void NavigationPage::resolveLocation(bool originField)
{
    QLineEdit* const edit = originField ? originEdit_ : destinationEdit_;
    const QString address = edit->text().trimmed();
    if (address.isEmpty()) {
        planningHintLabel_->setStyleSheet(QStringLiteral(
            "color: %1; background: transparent;").arg(cssColor(theme::danger())));
        planningHintLabel_->setText(originField
            ? QStringLiteral("请输入起点地址后按回车。")
            : QStringLiteral("请输入充电站地址后按回车。"));
        return;
    }

    if (geocoder_->isBusy()) {
        planningHintLabel_->setText(QStringLiteral("正在解析地址，请稍候…"));
        return;
    }

    const bool alreadyResolved = originField
        ? Session::instance().hasLocation() && address == currentLocationLabel()
        : destinationLatitude_ != 0.0 && destinationLongitude_ != 0.0
            && address == destinationName_;
    if (alreadyResolved) {
        replanFromCurrentInputs();
        return;
    }

    geocodingField_ = originField ? LocationField::Origin : LocationField::Destination;
    routeLoading_ = true;
    route_ = RouteResult{};
    state_ = Planning;
    renderState();
    planningHintLabel_->setText(QStringLiteral("正在解析地址并刷新路线…"));
    geocoder_->geocode(address);
}

void NavigationPage::onGeocoded(double latitude, double longitude,
                                const QString& formattedAddress)
{
    const LocationField field = geocodingField_;
    geocodingField_ = LocationField::None;
    const QString displayAddress = formattedAddress.trimmed().isEmpty()
        ? (field == LocationField::Origin ? originEdit_->text().trimmed()
                                          : destinationEdit_->text().trimmed())
        : formattedAddress.trimmed();

    if (field == LocationField::Origin) {
        originLatitude_ = latitude;
        originLongitude_ = longitude;
        originName_ = displayAddress;
        originEdit_->setText(displayAddress);
    } else if (field == LocationField::Destination) {
        destinationLatitude_ = latitude;
        destinationLongitude_ = longitude;
        destinationName_ = displayAddress;
        destinationEdit_->setText(displayAddress);
    } else {
        return;
    }
    replanFromCurrentInputs();
}

void NavigationPage::onGeocoderError(const QString& message)
{
    geocodingField_ = LocationField::None;
    routeLoading_ = false;
    state_ = Planning;
    renderState();
    planningHintLabel_->setStyleSheet(QStringLiteral(
        "color: %1; background: transparent;").arg(cssColor(theme::danger())));
    planningHintLabel_->setText(message);
}

void NavigationPage::replanFromCurrentInputs()
{
    navigationTimer_->stop();
    activeStepIndex_ = 0;
    navigationProgress_ = 0.0;
    currentPosition_ = QPointF();
    positionDriven_ = false;
    route_ = RouteResult{};
    routeLoading_ = true;
    state_ = Planning;
    renderState();
    requestRoute();
}

void NavigationPage::onSwapLocationsClicked()
{
    if (geocoder_->isBusy() || routeLoading_) {
        planningHintLabel_->setText(QStringLiteral("当前正在更新路线，请稍候…"));
        return;
    }
    if (!Session::instance().hasLocation() ||
        (destinationLatitude_ == 0.0 && destinationLongitude_ == 0.0)) {
        planningHintLabel_->setText(QStringLiteral("起点或充电站坐标尚未准备好。"));
        return;
    }

    const double oldOriginLatitude = originLatitude_;
    const double oldOriginLongitude = originLongitude_;
    const QString oldOriginText = originEdit_->text().trimmed();
    const QString oldDestinationText = destinationEdit_->text().trimmed();
    const double oldDestinationLatitude = destinationLatitude_;
    const double oldDestinationLongitude = destinationLongitude_;

    originLatitude_ = oldDestinationLatitude;
    originLongitude_ = oldDestinationLongitude;
    originName_ = oldDestinationText;
    destinationLatitude_ = oldOriginLatitude;
    destinationLongitude_ = oldOriginLongitude;
    destinationName_ = oldOriginText;
    originEdit_->setText(oldDestinationText);
    destinationEdit_->setText(oldOriginText);
    replanFromCurrentInputs();
}

void NavigationPage::requestRoute()
{
    if ((originLatitude_ == 0.0 && originLongitude_ == 0.0) ||
        (destinationLatitude_ == 0.0 && destinationLongitude_ == 0.0)) {
        onRouteError(QStringLiteral("请先在首页搜索并确认当前位置。"));
        return;
    }
    if (routePlanner_->isBusy()) {
        return;
    }
    routePlanner_->plan(originLatitude_, originLongitude_, destinationLatitude_,
                        destinationLongitude_, false);
}

void NavigationPage::onRouteReady(const RouteResult& result)
{
    if (!result.valid()) {
        onRouteError(QStringLiteral("没有找到可行路线，请更换出行方式后重试。"));
        return;
    }
    route_ = result;
    activeStepIndex_ = 0;
    navigationProgress_ = 0.0;
    currentPosition_ = (originLatitude_ != 0.0 || originLongitude_ != 0.0)
        ? QPointF(originLongitude_, originLatitude_)
        : QPointF();
    positionDriven_ = false;
    routeLoading_ = false;
    state_ = Planning;
    renderState();
    if (qEnvironmentVariableIsSet("EV_NAVIGATION_ACTIVE_PREVIEW")) {
        QTimer::singleShot(0, this, [this] { onStartNavigationClicked(); });
    } else if (qEnvironmentVariableIsSet("EV_NAVIGATION_FINISHED_PREVIEW")) {
        QTimer::singleShot(0, this, [this] {
            onStartNavigationClicked();
            QTimer::singleShot(1200, this, [this] { onStopNavigationClicked(); });
        });
    }
}

void NavigationPage::onRouteError(const QString& message)
{
    routeLoading_ = false;
    state_ = Error;
    renderState();
    planningHintLabel_->setText(message);
    planningHintLabel_->setStyleSheet(QStringLiteral(
        "color: %1; background: transparent;").arg(cssColor(theme::danger())));
}

void NavigationPage::renderState()
{
    const bool planning = state_ == Planning || state_ == Error;
    const bool navigating = state_ == Navigating;
    const bool finished = state_ == Finished;
    setAllStateWidgetsVisible(planning, navigating, finished);
    layoutUi();

    if (state_ == Planning) renderPlanning();
    else if (state_ == Navigating) renderNavigating();
    else if (state_ == Finished) renderFinished();
    else if (state_ == Error) renderError(planningHintLabel_->text());
}

void NavigationPage::setAllStateWidgetsVisible(bool planVisible,
                                                bool navigationVisible,
                                                bool finishedVisible)
{
    topBackButton_->setVisible(planVisible || finishedVisible);
    closeButton_->setVisible(navigationVisible);
    shareButton_->setVisible(planVisible || finishedVisible);
    routeSummaryCard_->setVisible(planVisible);
    originCaptionLabel_->setVisible(planVisible);
    destinationCaptionLabel_->setVisible(planVisible);
    originEdit_->setVisible(planVisible);
    destinationEdit_->setVisible(planVisible);
    routeSummaryMetaLabel_->setVisible(planVisible);
    routeSummaryMetaLabel_->setVisible(false);
    routeOptionsCard_->setVisible(planVisible);
    returnStationButton_->setVisible(planVisible);
    startNavigationButton_->setVisible(planVisible);
    planningHintLabel_->setVisible(planVisible);
    guidanceCard_->setVisible(navigationVisible);
    navigationBottomCard_->setVisible(navigationVisible);
    finishedCard_->setVisible(finishedVisible);
    finishNextStepsCard_->setVisible(finishedVisible);
    chooseChargerButton_->setVisible(finishedVisible);
    finishedHintLabel_->setVisible(finishedVisible);
    setMapVisible(planVisible || navigationVisible);
}

void NavigationPage::renderPlanning()
{
    pageTitle_->setText(QStringLiteral("路线规划"));
    if (originEdit_->text().trimmed().isEmpty()) {
        originEdit_->setText(originName_.isEmpty() ? currentLocationLabel() : originName_);
    }
    if (destinationEdit_->text().trimmed().isEmpty()) {
        destinationEdit_->setText(destinationName_);
    }
    routeSummaryMetaLabel_->setText(routeLoading_ ? QStringLiteral("规划中...")
                                                  : formatDistance(route_.distanceMeters));
    planningHintLabel_->setStyleSheet(QStringLiteral(
        "color: %1; background: transparent;").arg(cssColor(theme::textSecondary())));
    planningHintLabel_->setText(routeLoading_
        ? QStringLiteral("正在为你规划更顺畅的路线...")
        : QStringLiteral("电量充足，预计到达后剩余电量约 42%"));
    startNavigationButton_->setEnabled(!routeLoading_ && route_.valid());
    startNavigationButton_->setText(routeLoading_ ? QStringLiteral("规划中...")
                                                   : QStringLiteral("开始导航"));
    returnStationButton_->setEnabled(!routeLoading_);
    updatePlanningMetrics();
    loadMap();
    if (!routeLoading_) {
        runMapJavaScript(QStringLiteral(
            "window.__navigationMap && window.__navigationMap.setProgress(0);"));
    }
}

void NavigationPage::renderNavigating()
{
    pageTitle_->setText(QStringLiteral("导航中"));
    updateNavigationReadout();
    loadMap();
}

void NavigationPage::renderFinished()
{
    pageTitle_->setText(QStringLiteral("导航已结束"));
    finishedStationLabel_->setText(destinationName_);
    finishedDurationLabel_->setText(QStringLiteral("%1\n总用时")
        .arg(formatDuration(simulatedElapsedSeconds_ > 0 ? simulatedElapsedSeconds_
                                                         : route_.durationSeconds)));
    finishedDistanceLabel_->setText(QStringLiteral("%1\n行驶里程")
        .arg(formatDistance(route_.distanceMeters)));
    finishedParkingMetaLabel_->setText(QStringLiteral("P　地面停车场入口\n当前停车位置"));
}

void NavigationPage::renderError(const QString& message)
{
    Q_UNUSED(message);
    pageTitle_->setText(QStringLiteral("路线规划"));
    if (originEdit_->text().trimmed().isEmpty()) {
        originEdit_->setText(originName_.isEmpty() ? currentLocationLabel() : originName_);
    }
    if (destinationEdit_->text().trimmed().isEmpty()) {
        destinationEdit_->setText(destinationName_);
    }
    routeSummaryMetaLabel_->setText(QStringLiteral("--"));
    startNavigationButton_->setEnabled(false);
    routeOptionsCard_->setVisible(false);
    loadMap();
}

void NavigationPage::updatePlanningMetrics()
{
    if (routeLoading_ || !route_.valid()) {
        for (int index = 0; index < routeOptionButtons_.size(); ++index) {
            routeOptionButtons_[index]->setText(index == 0
                ? QStringLiteral("推荐")
                : index == 1 ? QStringLiteral("最快") : QStringLiteral("少红绿灯"));
            routeOptionTimeLabels_[index]->setText(QStringLiteral("--"));
            routeOptionMetaLabels_[index]->setText(QStringLiteral("等待高德路线"));
        }
        routeOptionsHintLabel_->setText(QStringLiteral("推荐路线"));
        return;
    }

    const qint64 baseDuration = route_.durationSeconds;
    const double baseDistance = route_.distanceMeters;
    const QList<QString> tags = {QStringLiteral("推荐"), QStringLiteral("最快"), QStringLiteral("少红绿灯")};
    for (int index = 0; index < 3; ++index) {
        const qint64 duration = qMax<qint64>(60, baseDuration + (index == 0 ? 0 : index == 1 ? -120 : 180));
        const double distance = qMax(100.0, baseDistance + (index == 0 ? 0.0 : index == 1 ? 400.0 : -300.0));
        routeOptionButtons_[index]->setText(tags[index]);
        routeOptionTimeLabels_[index]->setText(formatDuration(duration));
        routeOptionMetaLabels_[index]->setText(QStringLiteral("%1　|　%2")
            .arg(formatDistance(distance), formatEta(duration)));
        const bool selected = index == selectedRouteOption_;
        routeOptionButtons_[index]->setChecked(selected);
        routeOptionButtons_[index]->setStyleSheet(QStringLiteral(
            "QPushButton { color: %1; background: %2; border: 1px solid %3;"
            " border-radius: 14px; font-size: 13px; font-weight: 600; text-align: left;"
            " padding: 8px; }"
            "QPushButton:hover { border-color: %4; }")
            .arg(cssColor(selected ? theme::textPrimary() : theme::textSecondary()),
                 selected ? cssRgba(theme::primaryBlue(), 48) : cssRgba(theme::inputFill(), 230),
                 cssColor(selected ? theme::primaryBlue() : theme::cardBorder()),
                 cssColor(theme::primaryBlue())));
    }
    routeOptionsHintLabel_->setText(routeLoading_ ? QStringLiteral("路线选择")
                                                   : QStringLiteral("推荐路线"));
}

void NavigationPage::updateNavigationReadout()
{
    if (route_.durationSeconds <= 0 || route_.distanceMeters <= 0) return;
    const qint64 elapsed = navigationStartedAtMs_ > 0
        ? qMax<qint64>(0, (QDateTime::currentMSecsSinceEpoch() - navigationStartedAtMs_) / 1000)
        : simulatedElapsedSeconds_;
    if (!positionDriven_) {
        simulatedElapsedSeconds_ = qMin(route_.durationSeconds, elapsed);
        navigationProgress_ = route_.durationSeconds > 0
            ? qBound(0.0, static_cast<double>(simulatedElapsedSeconds_) /
                              route_.durationSeconds, 0.99)
            : 0.0;
    }
    const qint64 remainingSeconds = qMax<qint64>(0, route_.durationSeconds - simulatedElapsedSeconds_);
    const double progress = navigationProgress_;
    const double remainingDistance = route_.distanceMeters * (1.0 - progress);

    remainingDistanceLabel_->setText(QStringLiteral(
        "<span style='color:%1;font-size:13px;'>剩余距离</span><br>"
        "<span style='font-size:25px;font-weight:700;'>%2</span>")
        .arg(cssColor(theme::textSecondary()), formatDistance(remainingDistance)));
    remainingEtaLabel_->setText(QStringLiteral(
        "<span style='color:%1;font-size:13px;'>预计到达</span><br>"
        "<span style='font-size:25px;font-weight:700;'>%2</span>")
        .arg(cssColor(theme::textSecondary()), formatEta(remainingSeconds)));
    arrivalBatteryLabel_->setText(QStringLiteral(
        "<span style='color:%1;font-size:13px;'>到达后剩余电量</span><br>"
        "<span style='color:%2;font-size:25px;font-weight:700;'>42%</span>")
        .arg(cssColor(theme::textSecondary()), cssColor(theme::success())));
    updateGuidanceForProgress(progress);
    if (positionDriven_) {
        runMapJavaScript(QStringLiteral(
            "window.__navigationMap && window.__navigationMap.setUserPosition(%1,%2);")
            .arg(currentPosition_.x(), 0, 'f', 7)
            .arg(currentPosition_.y(), 0, 'f', 7));
    } else {
        runMapJavaScript(QStringLiteral(
            "window.__navigationMap && window.__navigationMap.setProgress(%1);")
            .arg(progress, 0, 'f', 5));
    }
}

double NavigationPage::routeProgressForPosition(const QPointF& position) const
{
    if (route_.path.size() < 2) {
        return 0.0;
    }

    const double earthScale = qCos(qDegreesToRadians(position.y()));
    double totalLength = 0.0;
    double closestLength = 0.0;
    double closestDistance = std::numeric_limits<double>::max();
    for (int index = 1; index < route_.path.size(); ++index) {
        const QPointF& start = route_.path.at(index - 1);
        const QPointF& end = route_.path.at(index);
        const double dx = (end.x() - start.x()) * earthScale;
        const double dy = end.y() - start.y();
        const double segmentLength = std::hypot(dx, dy);
        if (segmentLength < 1e-12) {
            continue;
        }

        const double px = (position.x() - start.x()) * earthScale;
        const double py = position.y() - start.y();
        const double ratio = qBound(0.0, (px * dx + py * dy) /
                                           (segmentLength * segmentLength), 1.0);
        const double projectedX = start.x() + (end.x() - start.x()) * ratio;
        const double projectedY = start.y() + (end.y() - start.y()) * ratio;
        const double distanceX = (position.x() - projectedX) * earthScale;
        const double distanceY = position.y() - projectedY;
        const double distance = distanceX * distanceX + distanceY * distanceY;
        if (distance < closestDistance) {
            closestDistance = distance;
            closestLength = totalLength + segmentLength * ratio;
        }
        totalLength += segmentLength;
    }
    return totalLength > 0.0 ? qBound(0.0, closestLength / totalLength, 1.0) : 0.0;
}

void NavigationPage::updateGuidanceForProgress(double progress, int preferredStep)
{
    if (!route_.valid()) {
        return;
    }
    navigationProgress_ = qBound(0.0, progress, 1.0);
    if (route_.steps.isEmpty()) {
        const int distance = qRound(route_.distanceMeters * (1.0 - navigationProgress_));
        guidanceDistanceLabel_->setText(QStringLiteral(
            "前方 <b style='color:%1;font-size:31px;'>%2</b> 米　沿路线行驶")
            .arg(cssColor(theme::primaryBlue())).arg(qMax(0, distance)));
        guidanceRoadLabel_->setText(QStringLiteral("沿规划路线行驶"));
        static_cast<TurnArrowWidget*>(turnArrowWidget_)->setInstruction(
            QStringLiteral("直行"));
        return;
    }

    const double travelled = route_.distanceMeters * navigationProgress_;
    int stepIndex = preferredStep;
    if (stepIndex < 0 || stepIndex >= route_.steps.size()) {
        double stepStart = 0.0;
        stepIndex = route_.steps.size() - 1;
        for (int index = 0; index < route_.steps.size(); ++index) {
            const double stepLength = qMax(0.0, route_.steps.at(index).distanceMeters);
            if (travelled <= stepStart + stepLength || index == route_.steps.size() - 1) {
                stepIndex = index;
                break;
            }
            stepStart += stepLength;
        }
    }
    activeStepIndex_ = stepIndex;

    double stepStart = 0.0;
    for (int index = 0; index < stepIndex; ++index) {
        stepStart += qMax(0.0, route_.steps.at(index).distanceMeters);
    }
    const RouteStep& step = route_.steps.at(stepIndex);
    const double stepLength = qMax(0.0, step.distanceMeters);
    const int distanceAhead = navigationProgress_ >= 0.995
        ? 0
        : qRound(stepLength > 0.0
                    ? qMax(0.0, stepStart + stepLength - travelled)
                    : route_.distanceMeters * (1.0 - navigationProgress_));
    const QString instruction = navigationProgress_ >= 0.995
        ? QStringLiteral("已到达目的地")
        : (step.instruction.trimmed().isEmpty()
               ? QStringLiteral("沿路线行驶") : step.instruction.trimmed());
    const QString road = navigationProgress_ >= 0.995
        ? destinationName_
        : (step.roadName.trimmed().isEmpty() ? instruction : step.roadName.trimmed());
    QString maneuver = QStringLiteral("沿路线行驶");
    const QStringList maneuverWords = {
        QStringLiteral("左转"), QStringLiteral("右转"), QStringLiteral("掉头"),
        QStringLiteral("直行"), QStringLiteral("靠左"), QStringLiteral("靠右"),
        QStringLiteral("到达")
    };
    for (const QString& word : maneuverWords) {
        if (instruction.contains(word)) {
            maneuver = word;
            break;
        }
    }

    guidanceDistanceLabel_->setText(QStringLiteral(
        "前方 <b style='color:%1;font-size:31px;'>%2</b> 米　%3")
        .arg(cssColor(theme::primaryBlue()))
        .arg(distanceAhead)
        .arg(maneuver.toHtmlEscaped()));
    const QString roadInstruction = road == instruction || road.isEmpty()
        ? maneuver
        : QStringLiteral("%1 · %2").arg(road, maneuver);
    guidanceRoadLabel_->setText(roadInstruction);
    static_cast<TurnArrowWidget*>(turnArrowWidget_)->setInstruction(maneuver);
}

void NavigationPage::updateUserPosition(double latitude, double longitude)
{
    if (state_ != Navigating || !route_.valid()) {
        return;
    }
    currentPosition_ = QPointF(longitude, latitude);
    positionDriven_ = true;
    navigationProgress_ = routeProgressForPosition(currentPosition_);
    simulatedElapsedSeconds_ = qRound64(
        route_.durationSeconds * navigationProgress_);
    updateNavigationReadout();
    runMapJavaScript(QStringLiteral(
        "window.__navigationMap && window.__navigationMap.setUserPosition(%1,%2);")
        .arg(longitude, 0, 'f', 7).arg(latitude, 0, 'f', 7));
}

QString NavigationPage::formatDistance(double meters) const
{
    if (meters <= 0.0) return QStringLiteral("--");
    if (meters < 1000.0) return QStringLiteral("%1 米").arg(qRound(meters));
    return QStringLiteral("%1 公里").arg(meters / 1000.0, 0, 'f', 1);
}

QString NavigationPage::formatDuration(qint64 seconds) const
{
    if (seconds <= 0) return QStringLiteral("0 分钟");
    const qint64 minutes = qMax<qint64>(1, qRound64(seconds / 60.0));
    if (minutes >= 60) {
        return QStringLiteral("%1小时%2分钟").arg(minutes / 60).arg(minutes % 60);
    }
    return QStringLiteral("%1 分钟").arg(minutes);
}

QString NavigationPage::formatEta(qint64 secondsFromNow) const
{
    const QTime time = QDateTime::currentDateTime().addSecs(
        qMax<qint64>(0, secondsFromNow)).time();
    return time.toString(QStringLiteral("H:mm"));
}

void NavigationPage::setMapVisible(bool visible)
{
    mapFrame_->setVisible(visible);
    recenterButton_->setVisible(visible);
    zoomInButton_->setVisible(visible);
    zoomOutButton_->setVisible(visible);
    layersButton_->setVisible(visible);
}

QString NavigationPage::buildNavigationMapDocument() const
{
    QFile templateFile(QStringLiteral(":/resources/navigation-map.html"));
    if (!templateFile.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    QString document = QString::fromUtf8(templateFile.readAll());
    QString routePath = QStringLiteral("[");
    for (int index = 0; index < route_.path.size(); ++index) {
        if (index > 0) routePath += QLatin1Char(',');
        routePath += QStringLiteral("[%1,%2]")
            .arg(route_.path.at(index).x(), 0, 'f', 7)
            .arg(route_.path.at(index).y(), 0, 'f', 7);
    }
    routePath += QLatin1Char(']');
    QJsonArray nameArray;
    nameArray.append(destinationName_);
    const QString nameJson = QString::fromUtf8(
        QJsonDocument(nameArray).toJson(QJsonDocument::Compact));
    document.replace(QStringLiteral("__ORIGIN_LNG__"),
                     QString::number(originLongitude_, 'f', 7));
    document.replace(QStringLiteral("__ORIGIN_LAT__"),
                     QString::number(originLatitude_, 'f', 7));
    document.replace(QStringLiteral("__DESTINATION_LNG__"),
                     QString::number(destinationLongitude_, 'f', 7));
    document.replace(QStringLiteral("__DESTINATION_LAT__"),
                     QString::number(destinationLatitude_, 'f', 7));
    document.replace(QStringLiteral("__ROUTE_PATH__"), routePath);
    document.replace(QStringLiteral("__DESTINATION_NAME__"), nameJson);
    document.replace(QStringLiteral("__DISTANCE_TEXT__"),
                     formatDistance(route_.distanceMeters));
    document.replace(QStringLiteral("__ETA_TEXT__"),
                     QStringLiteral("约%1到达").arg(formatDuration(route_.durationSeconds)));
    document.replace(QStringLiteral("__JS_API_KEY__"),
                     QString::fromLocal8Bit(qgetenv(appConfig::kAmapJsApiKeyEnvironment)));
    document.replace(QStringLiteral("__JS_API_SECRET__"),
                     QString::fromLocal8Bit(qgetenv(appConfig::kAmapJsApiSecretEnvironment)));
    return document;
}

void NavigationPage::loadMap()
{
    const bool offscreen = qgetenv("QT_QPA_PLATFORM").trimmed()
        == QByteArrayLiteral("offscreen");
    const bool hasJsKey = !qgetenv(appConfig::kAmapJsApiKeyEnvironment).trimmed().isEmpty();
    if (routeLoading_) {
        mapView_->setVisible(false);
        mapStateLabel_->setVisible(true);
        mapStateLabel_->setText(QStringLiteral("正在加载高德路线地图..."));
        return;
    }
    if (!hasJsKey || offscreen || route_.path.isEmpty()) {
        mapView_->setVisible(false);
        mapStateLabel_->setVisible(true);
        mapStateLabel_->setText(hasJsKey
            ? QStringLiteral("地图预览需要在桌面窗口中打开")
            : QStringLiteral("高德地图 key 未配置，无法显示路线地图"));
        return;
    }
    const QString document = buildNavigationMapDocument();
    if (document.isEmpty()) return;
    mapStateLabel_->setVisible(false);
    mapView_->setVisible(true);
    mapView_->raise();
    recenterButton_->raise();
    zoomInButton_->raise();
    zoomOutButton_->raise();
    layersButton_->raise();
    mapView_->setHtml(document, QUrl(QStringLiteral("https://webapi.amap.com/")));
}

void NavigationPage::runMapJavaScript(const QString& script)
{
    if (mapView_->isVisible() && mapView_->page()) {
        mapView_->page()->runJavaScript(script);
    }
}

void NavigationPage::onTopBackClicked()
{
    navigationTimer_->stop();
    if (state_ == Navigating || state_ == Finished) {
        state_ = Planning;
        routeLoading_ = false;
        renderState();
        return;
    }
    emit backRequested();
}

void NavigationPage::onShareClicked()
{
    if (QClipboard* clipboard = QApplication::clipboard()) {
        clipboard->setText(QStringLiteral("前往 %1，距离 %2，预计%3到达")
                           .arg(destinationName_, formatDistance(route_.distanceMeters),
                                formatDuration(route_.durationSeconds)));
    }
    planningHintLabel_->setText(QStringLiteral("路线信息已复制，可分享给好友"));
}

void NavigationPage::onRouteOptionClicked()
{
    auto* button = qobject_cast<QPushButton*>(sender());
    const int index = routeOptionButtons_.indexOf(button);
    if (index < 0) return;
    selectedRouteOption_ = index;
    updatePlanningMetrics();
}

void NavigationPage::onStartNavigationClicked()
{
    if (routeLoading_ || !route_.valid()) return;
    state_ = Navigating;
    navigationStartedAtMs_ = QDateTime::currentMSecsSinceEpoch();
    simulatedElapsedSeconds_ = 0;
    activeStepIndex_ = 0;
    navigationProgress_ = 0.0;
    positionDriven_ = false;
    currentPosition_ = (originLatitude_ != 0.0 || originLongitude_ != 0.0)
        ? QPointF(originLongitude_, originLatitude_)
        : QPointF();
    navigationTimer_->start();
    renderState();
}

void NavigationPage::onStopNavigationClicked()
{
    simulatedElapsedSeconds_ = navigationStartedAtMs_ > 0
        ? qMax<qint64>(0, (QDateTime::currentMSecsSinceEpoch() - navigationStartedAtMs_) / 1000)
        : simulatedElapsedSeconds_;
    navigationTimer_->stop();
    state_ = Finished;
    renderState();
}

void NavigationPage::onRecenterClicked()
{
    runMapJavaScript(QStringLiteral(
        "window.__navigationMap && window.__navigationMap.recenter();"));
}

void NavigationPage::onZoomInClicked()
{
    runMapJavaScript(QStringLiteral(
        "window.__navigationMap && window.__navigationMap.zoomIn();"));
}

void NavigationPage::onZoomOutClicked()
{
    runMapJavaScript(QStringLiteral(
        "window.__navigationMap && window.__navigationMap.zoomOut();"));
}

void NavigationPage::onLayersClicked()
{
    runMapJavaScript(QStringLiteral(
        "window.__navigationMap && window.__navigationMap.fit();"));
    if (planningHintLabel_->isVisible()) {
        planningHintLabel_->setText(QStringLiteral("已全览整条路线"));
    }
}

void NavigationPage::onNavigationTick()
{
    if (state_ != Navigating) return;
    updateNavigationReadout();
}
