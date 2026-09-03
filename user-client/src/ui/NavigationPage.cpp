#include "ui/NavigationPage.h"

#include "config/AppConfig.h"
#include "net/NetworkClient.h"
#include "session/Session.h"
#include "ui/theme/Theme.h"

#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QFile>
#include <QFrame>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QResizeEvent>
#include <QTimer>
#include <QUrl>
#include <QWebEnginePage>
#include <QWebEngineView>
#include <QMouseEvent>
#include <QtMath>

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

class NavIconButton : public QPushButton
{
public:
    enum class Kind { Back, Close, Share, Mute, Locate, ZoomIn, ZoomOut, Layers, Swap };

    NavIconButton(Kind kind, QWidget* parent = nullptr)
        : QPushButton(parent), kind_(kind)
    {
        setCursor(Qt::PointingHandCursor);
        setStyleSheet(QStringLiteral("background: transparent; border: none;"));
    }

    void setMuted(bool muted)
    {
        muted_ = muted;
        update();
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
        case Kind::Layers: iconPath = QStringLiteral(":/resources/icons/layers.svg"); break;
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
        case Kind::Mute:
            painter.drawLine(QPointF(7, cy - 5), QPointF(13, cy - 5));
            painter.drawLine(QPointF(13, cy - 5), QPointF(19, cy - 11));
            painter.drawLine(QPointF(19, cy - 11), QPointF(19, cy + 11));
            painter.drawLine(QPointF(19, cy + 11), QPointF(13, cy + 5));
            painter.drawLine(QPointF(13, cy + 5), QPointF(7, cy + 5));
            painter.drawLine(QPointF(7, cy + 5), QPointF(7, cy - 5));
            painter.drawLine(QPointF(25, cy - 6), QPointF(34, cy + 6));
            painter.drawLine(QPointF(34, cy - 6), QPointF(25, cy + 6));
            if (!muted_) {
                painter.drawArc(QRectF(18, cy - 11, 17, 22), -55 * 16, 110 * 16);
            }
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
    bool muted_ = false;
};

class ModeButton : public QPushButton
{
public:
    enum class Kind { Car, Walk, Bike };

    ModeButton(Kind kind, const QString& label, QWidget* parent = nullptr)
        : QPushButton(parent), kind_(kind), label_(label)
    {
        setCursor(Qt::PointingHandCursor);
        setStyleSheet(QStringLiteral("background: transparent; border: none;"));
    }

    void setSelected(bool selected)
    {
        selected_ = selected;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QColor accent = selected_ ? theme::primaryBlue() : theme::textSecondary();
        QColor fill = selected_ ? theme::primaryBlue() : theme::cardFill();
        fill.setAlpha(selected_ ? 225 : 170);
        painter.setBrush(fill);
        painter.setPen(QPen(selected_ ? QColor(0x4B, 0xA9, 0xFF) : theme::cardBorder(), 1));
        painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 20, 20);

        const qreal cx = 34;
        const qreal cy = height() / 2.0;
        const QString iconPath = kind_ == Kind::Car
            ? QStringLiteral(":/resources/icons/drive.svg")
            : kind_ == Kind::Walk
                ? QStringLiteral(":/resources/icons/walk.svg")
                : QStringLiteral(":/resources/icons/riding.svg");
        const QPixmap icon = tintedSvgPixmap(iconPath, QSize(27, 27),
                                             selected_ ? Qt::white : accent);
        if (!icon.isNull()) {
            painter.drawPixmap(qRound(cx - icon.width() / 2.0),
                               qRound(cy - icon.height() / 2.0), icon);
        }

        QFont font(QStringLiteral("Microsoft YaHei UI"));
        font.setPixelSize(17);
        font.setWeight(QFont::DemiBold);
        painter.setFont(font);
        painter.setPen(selected_ ? Qt::white : theme::textPrimary());
        painter.drawText(QRectF(55, 0, width() - 59, height()),
                         Qt::AlignVCenter | Qt::AlignLeft, label_);
    }

private:
    Kind kind_;
    QString label_;
    bool selected_ = false;
};

class TurnArrowWidget : public QFrame
{
public:
    explicit TurnArrowWidget(QWidget* parent = nullptr) : QFrame(parent)
    {
        setStyleSheet(QStringLiteral("background: transparent; border: none;"));
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QPixmap icon = tintedSvgPixmap(
            QStringLiteral(":/resources/icons/turn_right.svg"), QSize(52, 52),
            theme::textPrimary());
        if (!icon.isNull()) {
            painter.drawPixmap((width() - icon.width()) / 2,
                               (height() - icon.height()) / 2, icon);
            return;
        }
        painter.setPen(QPen(theme::textPrimary(), 8, Qt::SolidLine,
                            Qt::RoundCap, Qt::RoundJoin));
        painter.drawLine(QPointF(19, height() - 10), QPointF(19, 21));
        painter.drawLine(QPointF(19, 21), QPointF(width() - 15, 21));
        painter.drawLine(QPointF(width() - 15, 21), QPointF(width() - 29, 10));
        painter.drawLine(QPointF(width() - 15, 21), QPointF(width() - 29, 32));
    }
};

class LaneGuideWidget : public QFrame
{
public:
    explicit LaneGuideWidget(QWidget* parent = nullptr) : QFrame(parent)
    {
        setStyleSheet(QStringLiteral("background: transparent; border: none;"));
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        QColor fill = theme::inputFill();
        fill.setAlpha(220);
        painter.setBrush(fill);
        painter.setPen(QPen(theme::cardBorder(), 1));
        painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 12, 12);
        const int segment = width() / 4;
        painter.setPen(QPen(theme::cardBorder(), 1, Qt::DashLine));
        for (int i = 1; i < 4; ++i) {
            painter.drawLine(QPointF(i * segment, 8), QPointF(i * segment, height() - 8));
        }
        const QPixmap straight = tintedSvgPixmap(
            QStringLiteral(":/resources/icons/straight.svg"), QSize(26, 26),
            theme::textMuted());
        const QPixmap right = tintedSvgPixmap(
            QStringLiteral(":/resources/icons/turn_right.svg"), QSize(26, 26),
            theme::primaryBlue());
        if (!straight.isNull()) {
            painter.drawPixmap(segment / 2 - straight.width() / 2,
                               (height() - straight.height()) / 2, straight);
            painter.drawPixmap(segment + segment / 2 - straight.width() / 2,
                               (height() - straight.height()) / 2, straight);
        }
        if (!right.isNull()) {
            painter.drawPixmap(segment * 3 + segment / 2 - right.width() / 2,
                               (height() - right.height()) / 2, right);
        }
    }
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
    , previewMode_(qEnvironmentVariableIsSet("EV_HOME_PREVIEW") ||
                   qEnvironmentVariableIsSet("EV_DETAIL_PREVIEW") ||
                   qEnvironmentVariableIsSet("EV_NAVIGATION_PREVIEW") ||
                   qEnvironmentVariableIsSet("EV_NAVIGATION_ACTIVE_PREVIEW") ||
                   qEnvironmentVariableIsSet("EV_NAVIGATION_FINISHED_PREVIEW") ||
                   !qgetenv("EV_NAVIGATION_SCREENSHOT_PATH").isEmpty())
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
    originLabel_ = new QLabel(this);
    destinationLabel_ = new QLabel(this);
    routeSummaryMetaLabel_ = new QLabel(this);
    setLabelStyle(originLabel_, 16, theme::textPrimary());
    setLabelStyle(destinationLabel_, 16, theme::textPrimary());
    setLabelStyle(routeSummaryMetaLabel_, 13, theme::textSecondary(), false,
                  Qt::AlignRight | Qt::AlignVCenter);
    swapRouteButton_ = new NavIconButton(NavIconButton::Kind::Swap, routeSummaryCard_);
    swapRouteButton_->setToolTip(QStringLiteral("交换起终点"));
    connect(swapRouteButton_, &QPushButton::clicked, this, [this] {
        planningHintLabel_->setText(QStringLiteral("当前仅支持从当前位置前往充电站"));
    });

    const QList<QPair<ModeButton::Kind, QString>> modes = {
        {ModeButton::Kind::Car, QStringLiteral("驾车")},
        {ModeButton::Kind::Walk, QStringLiteral("步行")},
        {ModeButton::Kind::Bike, QStringLiteral("骑行")},
    };
    for (const auto& mode : modes) {
        auto* button = new ModeButton(mode.first, mode.second, this);
        modeButtons_.append(button);
        connect(button, &QPushButton::clicked,
                this, &NavigationPage::onModeClicked);
    }

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
    guidanceTurnLabel_ = new QLabel(guidanceCard_);
    setLabelStyle(guidanceDistanceLabel_, 17, theme::textPrimary(), false,
                  Qt::AlignLeft | Qt::AlignVCenter);
    setLabelStyle(guidanceRoadLabel_, 18, theme::textPrimary(), true);
    setLabelStyle(guidanceTurnLabel_, 17, theme::textPrimary(), false);
    muteButton_ = new NavIconButton(NavIconButton::Kind::Mute, guidanceCard_);
    muteButton_->setToolTip(QStringLiteral("静音"));
    connect(muteButton_, &QPushButton::clicked, [this] {
        muted_ = !muted_;
        static_cast<NavIconButton*>(muteButton_)->setMuted(muted_);
    });
    laneGuideWidget_ = new LaneGuideWidget(guidanceCard_);
    guidanceTimeLabel_ = new QLabel(guidanceCard_);
    guidanceDistanceMetaLabel_ = new QLabel(guidanceCard_);
    guidanceEtaLabel_ = new QLabel(guidanceCard_);
    for (QLabel* label : {guidanceTimeLabel_, guidanceDistanceMetaLabel_, guidanceEtaLabel_}) {
        setLabelStyle(label, 14, theme::textPrimary(), false, Qt::AlignCenter);
    }

    navigationBottomCard_ = new QFrame(this);
    stylePanel(navigationBottomCard_, 18, 238);
    remainingDistanceLabel_ = new QLabel(navigationBottomCard_);
    remainingEtaLabel_ = new QLabel(navigationBottomCard_);
    arrivalBatteryLabel_ = new QLabel(navigationBottomCard_);
    for (QLabel* label : {remainingDistanceLabel_, remainingEtaLabel_, arrivalBatteryLabel_}) {
        setLabelStyle(label, 18, theme::textPrimary(), true, Qt::AlignCenter);
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

    routeSummaryCard_->setGeometry(X(15), Y(84), width() - X(30), Y(94));
    originLabel_->setGeometry(X(47), Y(95), width() - X(120), Y(28));
    destinationLabel_->setGeometry(X(47), Y(135), width() - X(120), Y(28));
    routeSummaryMetaLabel_->setGeometry(width() - X(145), Y(107), X(65), Y(23));
    swapRouteButton_->setGeometry(routeSummaryCard_->width() - X(48), Y(19), X(34), Y(54));

    const int modeWidth = (width() - X(30)) / 3;
    for (int index = 0; index < modeButtons_.size(); ++index) {
        modeButtons_[index]->setGeometry(X(15) + index * modeWidth, Y(187),
                                         index == 2 ? width() - X(15) - (X(15) + index * modeWidth)
                                                    : modeWidth,
                                         Y(42));
    }

    mapFrame_->setGeometry(state_ == Navigating ? 0 : X(15),
                           state_ == Navigating ? Y(248) : Y(239),
                           state_ == Navigating ? width() : width() - X(30),
                           state_ == Navigating ? Y(407) : Y(319));
    const QRect mapContent = mapFrame_->rect().adjusted(1, 1, -1, -1);
    mapView_->setGeometry(mapContent);
    mapStateLabel_->setGeometry(X(24), mapFrame_->height() / 2 - Y(25),
                                mapFrame_->width() - X(48), Y(50));
    const int toolRight = mapFrame_->width() - X(53);
    recenterButton_->setGeometry(toolRight, mapFrame_->height() - Y(56), X(42), Y(42));
    zoomInButton_->setGeometry(toolRight, mapFrame_->height() - Y(154), X(42), Y(42));
    zoomOutButton_->setGeometry(toolRight, mapFrame_->height() - Y(106), X(42), Y(42));
    layersButton_->setGeometry(toolRight, mapFrame_->height() - Y(56), X(42), Y(42));

    routeOptionsCard_->setGeometry(X(15), Y(568), width() - X(30), Y(164));
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

    returnStationButton_->setGeometry(X(20), Y(748), X(145), Y(55));
    startNavigationButton_->setGeometry(X(176), Y(748), width() - X(196), Y(55));
    planningHintLabel_->setGeometry(X(24), Y(808), width() - X(48), Y(23));

    guidanceCard_->setGeometry(X(15), Y(74), width() - X(30), Y(184));
    turnArrowWidget_->setGeometry(X(25), Y(22), X(62), Y(63));
    guidanceDistanceLabel_->setGeometry(X(96), Y(17), X(255), Y(39));
    guidanceTurnLabel_->setGeometry(X(96), Y(53), X(255), Y(25));
    guidanceRoadLabel_->setGeometry(X(96), Y(53), X(255), Y(30));
    muteButton_->setGeometry(guidanceCard_->width() - X(76), Y(17), X(56), Y(56));
    laneGuideWidget_->setGeometry(X(20), Y(111), guidanceCard_->width() - X(40), Y(38));
    guidanceTimeLabel_->setGeometry(X(21), Y(151), X(125), Y(27));
    guidanceDistanceMetaLabel_->setGeometry(X(157), Y(151), X(125), Y(27));
    guidanceEtaLabel_->setGeometry(X(294), Y(151), X(125), Y(27));

    navigationBottomCard_->setGeometry(X(15), Y(662), width() - X(30), Y(154));
    remainingDistanceLabel_->setGeometry(X(8), Y(16), navigationBottomCard_->width() / 3 - X(8), Y(34));
    remainingEtaLabel_->setGeometry(navigationBottomCard_->width() / 3, Y(16),
                                    navigationBottomCard_->width() / 3, Y(34));
    arrivalBatteryLabel_->setGeometry(navigationBottomCard_->width() * 2 / 3, Y(16),
                                      navigationBottomCard_->width() / 3 - X(8), Y(34));
    stopNavigationButton_->setGeometry(X(15), Y(83), navigationBottomCard_->width() - X(30), Y(49));

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
    navigationTimer_->stop();
    destinationLatitude_ = destinationLatitude;
    destinationLongitude_ = destinationLongitude;
    destinationName_ = destinationName;
    walking_ = walking;
    muted_ = false;
    selectedRouteOption_ = 0;
    navigationStartedAtMs_ = 0;
    simulatedElapsedSeconds_ = 0;
    route_ = RouteResult{};
    state_ = Planning;
    routeLoading_ = true;
    renderState();
    requestRoute();
}

bool NavigationPage::previewMode() const
{
    return previewMode_;
}

QString NavigationPage::currentLocationLabel() const
{
    const QString label = Session::instance().locationLabel().trimmed();
    return label.isEmpty() ? QStringLiteral("当前位置") : label;
}

void NavigationPage::requestRoute()
{
    if (!Session::instance().hasLocation()) {
        Session::instance().setLocation(41.7087, 123.4312,
                                        QStringLiteral("沈阳市和平区三好街"));
    }
    if (previewMode_) {
        onRouteReady(makePreviewRoute());
        return;
    }
    routePlanner_->plan(Session::instance().latitude(), Session::instance().longitude(),
                        destinationLatitude_, destinationLongitude_, walking_);
}

RouteResult NavigationPage::makePreviewRoute() const
{
    const double originLat = Session::instance().hasLocation()
        ? Session::instance().latitude() : 41.7087;
    const double originLng = Session::instance().hasLocation()
        ? Session::instance().longitude() : 123.4312;
    const QPointF origin(originLng, originLat);
    const QPointF destination(destinationLongitude_, destinationLatitude_);
    const double dx = destination.x() - origin.x();
    const double dy = destination.y() - origin.y();
    const double lngBend = qAbs(dx) < 0.001 ? 0.003 : dx * 0.14;
    const double latBend = qAbs(dy) < 0.001 ? 0.002 : dy * 0.16;

    RouteResult result;
    result.path = {
        origin,
        QPointF(origin.x() + dx * .08 + lngBend, origin.y() + dy * .08),
        QPointF(origin.x() + dx * .23 + lngBend, origin.y() + dy * .21 + latBend),
        QPointF(origin.x() + dx * .38, origin.y() + dy * .38 + latBend),
        QPointF(origin.x() + dx * .57 - lngBend * .5, origin.y() + dy * .55),
        QPointF(origin.x() + dx * .73, origin.y() + dy * .72 - latBend * .5),
        QPointF(destination.x() - dx * .10, destination.y() - dy * .08),
        destination,
    };
    auto distanceBetween = [](const QPointF& left, const QPointF& right) {
        constexpr double earthRadiusKm = 6371.0;
        const double lat1 = qDegreesToRadians(left.y());
        const double lat2 = qDegreesToRadians(right.y());
        const double dLat = qDegreesToRadians(right.y() - left.y());
        const double dLng = qDegreesToRadians(right.x() - left.x());
        const double a = std::sin(dLat / 2.0) * std::sin(dLat / 2.0) +
                         std::cos(lat1) * std::cos(lat2) *
                         std::sin(dLng / 2.0) * std::sin(dLng / 2.0);
        return earthRadiusKm * 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a)) * 1000.0;
    };
    for (int index = 1; index < result.path.size(); ++index) {
        result.distanceMeters += distanceBetween(result.path.at(index - 1), result.path.at(index));
    }
    result.durationSeconds = qMax<qint64>(480, qRound64(result.distanceMeters / 2.5));
    result.firstInstruction = QStringLiteral("右转进入三好街");
    result.firstRoadName = QStringLiteral("三好街");
    result.firstStepDistanceMeters = 300.0;
    return result;
}

void NavigationPage::onRouteReady(const RouteResult& result)
{
    if (!result.valid()) {
        onRouteError(QStringLiteral("没有找到可行路线，请更换出行方式后重试。"));
        return;
    }
    route_ = result;
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
    originLabel_->setVisible(planVisible);
    destinationLabel_->setVisible(planVisible);
    routeSummaryMetaLabel_->setVisible(planVisible);
    for (QPushButton* button : modeButtons_) button->setVisible(planVisible);
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
    originLabel_->setText(QStringLiteral("当前位置　%1").arg(currentLocationLabel()));
    destinationLabel_->setText(QStringLiteral("充电站　%1").arg(destinationName_));
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
    setPlanMode(walking_);
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
    const int distance = route_.firstStepDistanceMeters > 0.0
        ? qRound(route_.firstStepDistanceMeters) : 300;
    const QString road = route_.firstRoadName.isEmpty()
        ? QStringLiteral("三好街") : route_.firstRoadName;
    QString turn = route_.firstInstruction;
    if (!turn.isEmpty() && !road.isEmpty()) {
        const int roadIndex = turn.indexOf(road);
        if (roadIndex >= 0) turn = turn.left(roadIndex).trimmed();
    }
    if (turn.isEmpty()) turn = QStringLiteral("右转进入");
    guidanceDistanceLabel_->setText(QStringLiteral(
        "前方 <b style='color:#4a9aff;font-size:31px;'>%1</b> 米　%2")
        .arg(distance).arg(turn.toHtmlEscaped()));
    guidanceDistanceLabel_->setTextFormat(Qt::RichText);
    guidanceTurnLabel_->clear();
    guidanceRoadLabel_->setText(road);
    guidanceTimeLabel_->setText(QStringLiteral("%1\n剩余时间")
        .arg(formatDuration(route_.durationSeconds)));
    guidanceDistanceMetaLabel_->setText(QStringLiteral("%1\n剩余距离")
        .arg(formatDistance(route_.distanceMeters)));
    guidanceEtaLabel_->setText(QStringLiteral("%1\n预计到达")
        .arg(formatEta(route_.durationSeconds)));
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
    originLabel_->setText(QStringLiteral("当前位置　%1").arg(currentLocationLabel()));
    destinationLabel_->setText(QStringLiteral("充电站　%1").arg(destinationName_));
    routeSummaryMetaLabel_->setText(QStringLiteral("--"));
    startNavigationButton_->setEnabled(false);
    routeOptionsCard_->setVisible(false);
    loadMap();
}

void NavigationPage::updatePlanningMetrics()
{
    const qint64 baseDuration = route_.durationSeconds > 0 ? route_.durationSeconds : 1080;
    const double baseDistance = route_.distanceMeters > 0 ? route_.distanceMeters : 6800.0;
    const QList<QString> tags = {QStringLiteral("推荐"), QStringLiteral("最快"), QStringLiteral("少红绿灯")};
    const QList<QString> details = {QStringLiteral("路线畅通　| 8 个红绿灯"),
                                    QStringLiteral("更快路线　| 12 个红绿灯"),
                                    QStringLiteral("红绿灯少　| 5 个红绿灯")};
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
    simulatedElapsedSeconds_ = qMin(route_.durationSeconds, elapsed);
    const qint64 remainingSeconds = qMax<qint64>(0, route_.durationSeconds - simulatedElapsedSeconds_);
    const double progress = route_.durationSeconds > 0
        ? qBound(0.0, static_cast<double>(simulatedElapsedSeconds_) / route_.durationSeconds, 0.99)
        : 0.0;
    const double remainingDistance = route_.distanceMeters * (1.0 - progress);

    guidanceTimeLabel_->setText(QStringLiteral("%1\n剩余时间")
        .arg(formatDuration(remainingSeconds)));
    guidanceDistanceMetaLabel_->setText(QStringLiteral("%1\n剩余距离")
        .arg(formatDistance(remainingDistance)));
    guidanceEtaLabel_->setText(QStringLiteral("%1\n预计到达")
        .arg(formatEta(remainingSeconds)));
    remainingDistanceLabel_->setText(QStringLiteral("剩余　%1\n剩余距离")
        .arg(formatDistance(remainingDistance)));
    remainingEtaLabel_->setText(QStringLiteral("到达　%1\n预计到达")
        .arg(formatEta(remainingSeconds)));
    arrivalBatteryLabel_->setText(QStringLiteral("电量　42%\n到达后剩余电量"));
    runMapJavaScript(QStringLiteral(
        "window.__navigationMap && window.__navigationMap.setProgress(%1);")
        .arg(progress, 0, 'f', 5));
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

void NavigationPage::setPlanMode(bool walking)
{
    if (modeButtons_.size() != 3) return;
    static_cast<ModeButton*>(modeButtons_.at(0))->setSelected(!walking);
    static_cast<ModeButton*>(modeButtons_.at(1))->setSelected(walking);
    static_cast<ModeButton*>(modeButtons_.at(2))->setSelected(false);
}

void NavigationPage::setMapVisible(bool visible)
{
    mapFrame_->setVisible(visible);
    recenterButton_->setVisible(visible);
    zoomInButton_->setVisible(visible && state_ == Planning);
    zoomOutButton_->setVisible(visible && state_ == Planning);
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
    const Session& session = Session::instance();
    document.replace(QStringLiteral("__ORIGIN_LNG__"),
                     QString::number(session.longitude(), 'f', 7));
    document.replace(QStringLiteral("__ORIGIN_LAT__"),
                     QString::number(session.latitude(), 'f', 7));
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
            : QStringLiteral("高德地图 key 未配置，暂时无法显示路线地图"));
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

void NavigationPage::onModeClicked()
{
    auto* button = static_cast<ModeButton*>(sender());
    const int index = modeButtons_.indexOf(button);
    if (index < 0) return;
    if (index == 2) {
        planningHintLabel_->setText(QStringLiteral("骑行路线即将上线，当前使用驾车路线"));
        return;
    }
    const bool walking = index == 1;
    if (walking_ == walking && !routeLoading_) return;
    walking_ = walking;
    routeLoading_ = true;
    route_ = RouteResult{};
    renderState();
    requestRoute();
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
    planningHintLabel_->setText(QStringLiteral("地图图层已切换"));
}

void NavigationPage::onNavigationTick()
{
    if (state_ != Navigating) return;
    updateNavigationReadout();
}
