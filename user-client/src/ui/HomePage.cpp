#include "ui/HomePage.h"

#include "config/AppConfig.h"
#include "geo/Geocoder.h"
#include "net/NetworkClient.h"
#include "protocol/Protocol.h"
#include "session/Session.h"
#include "ui/StationCard.h"
#include "ui/theme/Theme.h"

#include <QFile>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QResizeEvent>
#include <QShowEvent>
#include <QSizePolicy>
#include <QTimer>
#include <QVBoxLayout>
#include <QWebEngineView>
#include <QUrl>
#include <algorithm>

namespace {

// 与 WSL 版本保持一致的默认定位，只在首次进入主页且没有已有定位时使用。
constexpr double kDefaultLatitude = 39.735678;
constexpr double kDefaultLongitude = 116.171271;
constexpr const char* kDefaultLocationLabel = "北京理工大学良乡校区";

QString cssColor(const QColor& color)
{
    return color.name(QColor::HexRgb);
}

QString cssRgba(const QColor& color, int alpha)
{
    return QStringLiteral("rgba(%1,%2,%3,%4)")
        .arg(color.red()).arg(color.green()).arg(color.blue()).arg(alpha);
}

QString readEnvironmentValue(const char* variableName)
{
    return QString::fromLocal8Bit(qgetenv(variableName)).trimmed();
}

QString formatCoordinate(double value)
{
    return QString::number(value, 'f', 6);
}

class SearchField : public QLineEdit
{
public:
    explicit SearchField(QWidget* parent = nullptr) : QLineEdit(parent) {}

protected:
    void paintEvent(QPaintEvent* event) override
    {
        QPainter backgroundPainter(this);
        backgroundPainter.setRenderHint(QPainter::Antialiasing, true);
        QColor fill = theme::cardFill();
        fill.setAlpha(245);
        backgroundPainter.setBrush(fill);
        backgroundPainter.setPen(QPen(hasFocus() ? theme::primaryBlue()
                                                  : theme::cardBorder(), 1));
        backgroundPainter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 18, 18);

        QLineEdit::paintEvent(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(QPen(theme::textSecondary(), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(QRectF(17, 10, 12, 12));
        painter.drawLine(QPointF(27, 21), QPointF(33, 27));
    }
};

class RecenterButton : public QPushButton
{
public:
    explicit RecenterButton(QWidget* parent = nullptr) : QPushButton(parent)
    {
        setCursor(Qt::PointingHandCursor);
        setStyleSheet(QStringLiteral("background: transparent; border: none;"));
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        QColor background = theme::background();
        background.setAlpha(220);
        painter.setPen(QPen(theme::textPrimary(), 1.5));
        painter.setBrush(background);
        painter.drawEllipse(rect().adjusted(1, 1, -1, -1));
        painter.setPen(QPen(theme::textPrimary(), 2));
        painter.drawEllipse(QPointF(width() / 2.0, height() / 2.0), 7, 7);
        painter.drawLine(QPointF(width() / 2.0, 6),
                         QPointF(width() / 2.0, height() - 6));
        painter.drawLine(QPointF(6, height() / 2.0),
                         QPointF(width() - 6, height() / 2.0));
    }
};

class NavButton : public QPushButton
{
public:
    enum class Kind { Home, Station, Charge, Order, Profile };

    NavButton(Kind kind, const QString& text, bool selected, QWidget* parent = nullptr)
        : QPushButton(parent), kind_(kind), text_(text), selected_(selected)
    {
        setCursor(Qt::PointingHandCursor);
        setStyleSheet(QStringLiteral("background: transparent; border: none;"));
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QColor color = selected_ ? theme::primaryBlue() : theme::textSecondary();
        painter.setPen(QPen(color, 2.1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);
        const QPointF c(width() / 2.0, 18);

        if (kind_ == Kind::Home) {
            QPainterPath home;
            home.moveTo(c.x() - 11, 17);
            home.lineTo(c.x(), 8);
            home.lineTo(c.x() + 11, 17);
            home.lineTo(c.x() + 9, 17);
            home.lineTo(c.x() + 9, 28);
            home.lineTo(c.x() - 9, 28);
            home.lineTo(c.x() - 9, 17);
            home.closeSubpath();
            painter.setBrush(color);
            painter.drawPath(home);
        } else if (kind_ == Kind::Station) {
            painter.drawEllipse(QPointF(c.x(), 15), 9, 9);
            QPainterPath pin;
            pin.moveTo(c.x(), 31);
            pin.cubicTo(c.x() - 2, 28, c.x() - 6, 23, c.x() - 6, 19);
            pin.cubicTo(c.x() - 6, 14, c.x() - 3, 11, c.x(), 11);
            pin.cubicTo(c.x() + 3, 11, c.x() + 6, 14, c.x() + 6, 19);
            pin.cubicTo(c.x() + 6, 23, c.x() + 2, 28, c.x(), 31);
            painter.drawPath(pin);
            painter.drawLine(QPointF(c.x() - 10, 31), QPointF(c.x() + 10, 31));
        } else if (kind_ == Kind::Charge) {
            painter.drawEllipse(c, 11, 11);
            QPainterPath bolt;
            bolt.moveTo(c.x() + 2, 7);
            bolt.lineTo(c.x() - 4, 19);
            bolt.lineTo(c.x(), 19);
            bolt.lineTo(c.x() - 2, 29);
            bolt.lineTo(c.x() + 6, 16);
            bolt.lineTo(c.x() + 2, 16);
            bolt.closeSubpath();
            painter.setBrush(color);
            painter.drawPath(bolt);
        } else if (kind_ == Kind::Order) {
            painter.drawRoundedRect(QRectF(c.x() - 11, 8, 20, 22), 2, 2);
            painter.drawLine(QPointF(c.x() - 6, 15), QPointF(c.x() + 4, 15));
            painter.drawLine(QPointF(c.x() - 6, 20), QPointF(c.x() + 4, 20));
            painter.drawLine(QPointF(c.x() - 6, 25), QPointF(c.x() + 1, 25));
            painter.drawLine(QPointF(c.x() + 7, 24), QPointF(c.x() + 12, 29));
        } else {
            painter.drawEllipse(QPointF(c.x(), 12), 6, 6);
            painter.drawArc(QRectF(c.x() - 11, 21, 22, 18), 20 * 16, 140 * 16);
        }

        QFont font(QStringLiteral("Microsoft YaHei UI"));
        font.setPixelSize(13);
        painter.setFont(font);
        painter.setPen(color);
        painter.drawText(QRectF(0, 36, width(), 17), Qt::AlignHCenter, text_);
    }

private:
    Kind kind_;
    QString text_;
    bool selected_;
};

QList<StationInfo> previewStations()
{
    return {
        {1, QStringLiteral("东软软件园充电站"), 1.25, 20, 8, 1.2},
        {2, QStringLiteral("长白岛万科广场充电站"), 1.35, 16, 5, 2.1},
        {3, QStringLiteral("深南商业中心充电站"), 1.28, 30, 12, 3.4},
        {4, QStringLiteral("沈阳奥体中心充电站"), 1.30, 24, 9, 4.6},
    };
}

} // namespace

HomePage::HomePage(NetworkClient* networkClient, QWidget* parent)
    : QWidget(parent)
    , networkClient_(networkClient)
    , geocoder_(new Geocoder(this))
    , previewMode_(qEnvironmentVariableIsSet("EV_HOME_PREVIEW"))
{
    setObjectName(QStringLiteral("HomePage"));
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QStringLiteral(
        "#HomePage { background-color: %1; }").arg(cssColor(theme::background())));
    setMinimumSize(360, theme::loginCanvasHeight);
    setFocusPolicy(Qt::StrongFocus);

    buildUi();

    connect(searchButton_, &QPushButton::clicked,
            this, &HomePage::onSearchClicked);
    connect(searchEdit_, &QLineEdit::returnPressed,
            this, &HomePage::onSearchClicked);
    connect(recenterButton_, &QPushButton::clicked,
            this, &HomePage::onLocateClicked);
    connect(geocoder_, &Geocoder::geocoded,
            this, &HomePage::onGeocoded);
    connect(geocoder_, &Geocoder::error,
            this, &HomePage::onGeocodeError);

    updateSortButtons();
    QTimer::singleShot(0, this, [this] { setFocus(); });
}

HomePage::~HomePage() = default;

void HomePage::buildUi()
{
    const int margin = 17;
    const int contentWidth = theme::loginCanvasWidth - margin * 2;

    titleLabel_ = new QLabel(QStringLiteral("附近充电站"), this);
    titleLabel_->setFont(theme::brandSubtitleFont());
    titleLabel_->setStyleSheet(QStringLiteral(
        "color: %1; background: transparent;").arg(cssColor(theme::primaryBlue())));

    searchEdit_ = new SearchField(this);
    searchEdit_->setObjectName(QStringLiteral("HomeSearch"));
    searchEdit_->setPlaceholderText(QStringLiteral("请输入地址 / 充电站 / 商圈"));
    searchEdit_->setFont(theme::inputFont());
    searchEdit_->setFocusPolicy(Qt::ClickFocus);
    searchEdit_->setStyleSheet(QString(
        "QLineEdit#HomeSearch { background: transparent; border: none;"
        " padding: 0 62px 0 40px; color: %1; }"
        "QLineEdit#HomeSearch::placeholder { color: %2; }")
        .arg(cssColor(theme::textPrimary()), cssColor(theme::textSecondary())));

    searchButton_ = new QPushButton(QStringLiteral("搜索"), this);
    searchButton_->setCursor(Qt::PointingHandCursor);
    searchButton_->setFont(theme::inputFont());
    searchButton_->setStyleSheet(QStringLiteral(
        "QPushButton { background: transparent; border: none; color: %1; }"
        "QPushButton:hover { color: %2; }")
        .arg(cssColor(theme::primaryBlue()), cssColor(theme::primaryBlueHover())));

    mapFrame_ = new QFrame(this);
    mapFrame_->setObjectName(QStringLiteral("MapFrame"));
    mapFrame_->setStyleSheet(QStringLiteral(
        "#MapFrame { background: %1; border: 1px solid %2; border-radius: 17px; }")
        .arg(cssColor(theme::inputFill()), cssColor(theme::cardBorder())));

    mapView_ = new QWebEngineView(mapFrame_);
    mapView_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    mapView_->setStyleSheet(QStringLiteral("background: transparent; border: none;"));
    mapView_->setVisible(false);

    recenterButton_ = new RecenterButton(mapFrame_);
    recenterButton_->setAccessibleName(QStringLiteral("回到当前位置"));
    recenterButton_->raise();

    for (const QString& text : {QStringLiteral("距离优先"),
                                QStringLiteral("价格优先"),
                                QStringLiteral("空闲优先")}) {
        auto* button = new QPushButton(text, this);
        button->setCheckable(true);
        button->setCursor(Qt::PointingHandCursor);
        button->setFont(theme::buttonFont());
        sortButtons_.append(button);
        connect(button, &QPushButton::clicked,
                this, &HomePage::onSortClicked);
    }

    footerLabel_ = new QLabel(this);
    footerLabel_->setAlignment(Qt::AlignCenter);
    footerLabel_->setTextFormat(Qt::RichText);
    footerLabel_->setFont(theme::footerFont());
    footerLabel_->setStyleSheet(QStringLiteral("background: transparent;"));
    footerLabel_->setText(QStringLiteral(
        "<span style='color:%1;'>登录即可使用 </span>"
        "<span style='color:%2;'>收藏（充电中）</span>"
        "<span style='color:%1;'> 和 </span>"
        "<span style='color:%2;'>（电量监控）</span>"
        "<span style='color:%1;'> 功能</span>")
        .arg(cssColor(theme::textSecondary()), cssColor(theme::primaryBlue())));

    bottomBar_ = new QFrame(this);
    bottomBar_->setObjectName(QStringLiteral("HomeBottomBar"));
    bottomBar_->setStyleSheet(QStringLiteral(
        "#HomeBottomBar { background-color: %1; border-top: 1px solid %2; }")
        .arg(cssRgba(theme::background(), 248), cssColor(theme::cardBorder())));

    const QList<QPair<NavButton::Kind, QString>> navItems = {
        {NavButton::Kind::Home, QStringLiteral("首页")},
        {NavButton::Kind::Station, QStringLiteral("站点")},
        {NavButton::Kind::Charge, QStringLiteral("充电")},
        {NavButton::Kind::Order, QStringLiteral("订单")},
        {NavButton::Kind::Profile, QStringLiteral("我的")},
    };
    for (const auto& item : navItems) {
        navButtons_.append(new NavButton(item.first, item.second,
                                          navButtons_.isEmpty(), bottomBar_));
    }

    Q_UNUSED(contentWidth);
}

void HomePage::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.fillRect(rect(), theme::background());
}

void HomePage::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    const qreal sx = static_cast<qreal>(width()) / theme::loginCanvasWidth;
    const qreal sy = static_cast<qreal>(height()) / theme::loginCanvasHeight;
    const int margin = qRound(17 * sx);
    const int contentWidth = width() - margin * 2;

    titleLabel_->setGeometry(margin + 4, qRound(theme::homeHeaderTop * sy),
                             qRound(220 * sx), qRound(29 * sy));

    searchEdit_->setGeometry(margin, qRound(theme::homeSearchTop * sy), contentWidth,
                             qRound(theme::homeSearchHeight * sy));
    searchButton_->setGeometry(width() - qRound(76 * sx),
                               qRound(theme::homeSearchTop * sy), qRound(62 * sx),
                               qRound(theme::homeSearchHeight * sy));

    mapFrame_->setGeometry(margin, qRound(theme::homeMapTop * sy), contentWidth,
                           qRound(theme::homeMapHeight * sy));
    const QRect mapContent = mapFrame_->rect().adjusted(1, 1, -1, -1);
    mapView_->setGeometry(mapContent);
    recenterButton_->setGeometry(mapFrame_->width() - qRound(50 * sx),
                                 mapFrame_->height() - qRound(50 * sy),
                                 qRound(40 * sx), qRound(40 * sy));
    recenterButton_->raise();

    const int sortGap = qRound(26 * sx);
    const int sortWidth = (contentWidth - sortGap * 2) / 3;
    for (int i = 0; i < sortButtons_.size(); ++i) {
        sortButtons_[i]->setGeometry(margin + i * (sortWidth + sortGap),
                                     qRound(theme::homeSortTop * sy), sortWidth,
                                     qRound(theme::homeSortHeight * sy));
    }

    layoutStationItems();

    footerLabel_->setGeometry(margin, qRound(theme::homeFooterTop * sy),
                              contentWidth, qRound(22 * sy));
    bottomBar_->setGeometry(0, qRound(theme::homeNavTop * sy), width(),
                            qRound(theme::homeNavHeight * sy));
    const int navWidth = width() / navButtons_.size();
    for (int i = 0; i < navButtons_.size(); ++i) {
        navButtons_[i]->setGeometry(i * navWidth, 0,
                                    i == navButtons_.size() - 1
                                        ? width() - i * navWidth : navWidth,
                                    bottomBar_->height());
    }
}

void HomePage::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    if (firstShowHandled_) {
        return;
    }
    firstShowHandled_ = true;

    if (previewMode_) {
        // 视觉预览不请求站点接口，但仍让配置了高德 key 的环境显示真实地图。
        Session::instance().setLocation(
            kDefaultLatitude, kDefaultLongitude,
            QStringLiteral("%1（预览定位）").arg(QString::fromUtf8(kDefaultLocationLabel)));
        refreshMap();
        stations_ = previewStations();
        applySort(sortMode_);
        return;
    }

    if (!Session::instance().hasLocation()) {
        applyDefaultLocation();
    } else {
        refreshStations();
    }
}

void HomePage::onSearchClicked()
{
    if (geocoder_->isBusy()) {
        searchEdit_->setToolTip(QStringLiteral("正在定位中，请稍候..."));
        return;
    }
    const QString address = searchEdit_->text().trimmed();
    if (address.isEmpty()) {
        searchEdit_->setToolTip(QStringLiteral("请输入要定位的地址"));
        searchEdit_->setFocus();
        return;
    }
    searchEdit_->setToolTip(QStringLiteral("正在使用高德地图定位..."));
    geocoder_->geocode(address);
}

void HomePage::onLocateClicked()
{
    applyDefaultLocation();
}

void HomePage::onGeocoded(double latitude, double longitude,
                          const QString& address)
{
    Session::instance().setLocation(
        latitude, longitude,
        QStringLiteral("%1（高德地图）").arg(address));
    searchEdit_->setToolTip(QStringLiteral("已定位：%1").arg(address));
    refreshStations();
}

void HomePage::onGeocodeError(const QString& message)
{
    searchEdit_->setToolTip(message);
}

void HomePage::applyDefaultLocation()
{
    Session::instance().setLocation(
        kDefaultLatitude, kDefaultLongitude,
        QStringLiteral("%1（默认定位）").arg(QString::fromUtf8(kDefaultLocationLabel)));
    refreshStations();
}

void HomePage::refreshMap()
{
    const Session& session = Session::instance();
    const QString jsKey = readEnvironmentValue(appConfig::kAmapJsApiKeyEnvironment);
    // QtWebEngine 的 Chromium/OpenGL 在 QT_QPA_PLATFORM=offscreen 下不稳定；
    // 无法加载时保留纯色地图容器，不再显示静态地图占位图。
    const bool offscreen = qgetenv("QT_QPA_PLATFORM").trimmed()
        == QByteArrayLiteral("offscreen");
    if (!session.hasLocation() || jsKey.isEmpty() || offscreen) {
        mapView_->setVisible(false);
        recenterButton_->raise();
        return;
    }

    const QString document = buildHomeMapDocument(session.longitude(), session.latitude());
    if (document.isEmpty()) {
        mapView_->setVisible(false);
        recenterButton_->raise();
        return;
    }

    mapView_->setVisible(true);
    mapView_->raise();
    recenterButton_->raise();
    mapView_->setHtml(document, QUrl(QStringLiteral("https://webapi.amap.com/")));
}

QString HomePage::buildHomeMapDocument(double longitude, double latitude) const
{
    QFile templateFile(QStringLiteral(":/resources/home-map.html"));
    if (!templateFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }

    QString document = QString::fromUtf8(templateFile.readAll());
    document.replace(QStringLiteral("__CENTER_LNG__"), formatCoordinate(longitude));
    document.replace(QStringLiteral("__CENTER_LAT__"), formatCoordinate(latitude));
    document.replace(QStringLiteral("__JS_API_KEY__"),
                     readEnvironmentValue(appConfig::kAmapJsApiKeyEnvironment));
    document.replace(QStringLiteral("__JS_API_SECRET__"),
                     readEnvironmentValue(appConfig::kAmapJsApiSecretEnvironment));
    return document;
}

void HomePage::refreshStations()
{
    refreshMap();
    if (!Session::instance().hasLocation()) {
        stations_.clear();
        showEmptyState();
        return;
    }
    requestNearbyStations();
}

void HomePage::requestNearbyStations()
{
    if (!networkClient_) {
        showErrorState(QStringLiteral("网络模块未初始化"));
        return;
    }

    const Session& session = Session::instance();
    const quint64 requestVersion = ++stationQueryVersion_;
    showLoadingState();

    QJsonObject data;
    data.insert(QStringLiteral("latitude"), session.latitude());
    data.insert(QStringLiteral("longitude"), session.longitude());

    networkClient_->sendRequest(
        QString::fromUtf8(protocol::action::kStationNearby), data,
        [this, requestVersion](const protocol::Response& response) {
            if (requestVersion != stationQueryVersion_) {
                return;
            }
            if (!response.isOk()) {
                showErrorState(protocol::describeError(response.code,
                                                        response.message));
                return;
            }
            stations_ = StationInfo::fromJsonArray(
                response.data.value(QStringLiteral("stations")).toArray());
            applySort(sortMode_);
        });
}

void HomePage::applySort(int sortMode)
{
    sortMode_ = sortMode;
    updateSortButtons();
    if (stations_.isEmpty()) {
        showEmptyState();
        return;
    }

    std::sort(stations_.begin(), stations_.end(),
              [sortMode](const StationInfo& left, const StationInfo& right) {
        if (sortMode == 1 && left.pricePerKwh != right.pricePerKwh) {
            return left.pricePerKwh < right.pricePerKwh;
        }
        if (sortMode == 2 && left.availableChargers != right.availableChargers) {
            return left.availableChargers > right.availableChargers;
        }
        return left.distanceKm < right.distanceKm;
    });
    renderStations();
}

void HomePage::updateSortButtons()
{
    for (int i = 0; i < sortButtons_.size(); ++i) {
        const bool active = i == sortMode_;
        sortButtons_[i]->setChecked(active);
        sortButtons_[i]->setStyleSheet(QString(
            "QPushButton { background-color: %1; color: %2; border: 1px solid %3;"
            " border-radius: 18px; }"
            "QPushButton:hover { border-color: %4; }")
            .arg(active ? cssRgba(theme::primaryBlue(), 60)
                        : cssRgba(theme::cardFill(), 220))
            .arg(cssColor(theme::textPrimary()))
            .arg(active ? cssColor(theme::primaryBlue()) : cssColor(theme::cardBorder()))
            .arg(cssColor(theme::primaryBlue())));
    }
}

void HomePage::showLoadingState()
{
    clearList();
    auto* loading = new QLabel(QStringLiteral("加载附近站点中..."), this);
    loading->setProperty("homeState", true);
    loading->setAlignment(Qt::AlignCenter);
    loading->setStyleSheet(QStringLiteral(
        "font-size: 14px; color: %1; background: transparent;")
        .arg(cssColor(theme::textSecondary())));
    stationItems_.append(loading);
    loading->show();
    layoutStationItems();
}

void HomePage::showErrorState(const QString& message)
{
    clearList();
    auto* state = new QFrame(this);
    state->setObjectName(QStringLiteral("HomeState"));
    state->setProperty("homeState", true);
    state->setStyleSheet(QStringLiteral(
        "QFrame#HomeState { background: %1; border: 1px solid %2; border-radius: 17px; }")
        .arg(cssRgba(theme::cardFill(), 235), cssColor(theme::cardBorder())));
    auto* layout = new QVBoxLayout(state);
    layout->setContentsMargins(16, 15, 16, 12);
    layout->setSpacing(7);
    auto* title = new QLabel(QStringLiteral("附近站点加载失败"), state);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(QStringLiteral(
        "font-size: 16px; font-weight: 600; color: %1; background: transparent;")
        .arg(cssColor(theme::textPrimary())));
    auto* detail = new QLabel(message, state);
    detail->setAlignment(Qt::AlignCenter);
    detail->setWordWrap(true);
    detail->setStyleSheet(QStringLiteral(
        "font-size: 13px; color: %1; background: transparent;")
        .arg(cssColor(theme::textSecondary())));
    auto* retry = new QPushButton(QStringLiteral("重试"), state);
    retry->setCursor(Qt::PointingHandCursor);
    retry->setStyleSheet(QStringLiteral(
        "QPushButton { color: %1; background: transparent; border: 1px solid %1;"
        " border-radius: 15px; padding: 3px 18px; }"
        "QPushButton:hover { background: %2; }")
        .arg(cssColor(theme::primaryBlue()), cssRgba(theme::primaryBlue(), 40)));
    connect(retry, &QPushButton::clicked, this, &HomePage::refreshStations);
    layout->addWidget(title);
    layout->addWidget(detail);
    layout->addWidget(retry, 0, Qt::AlignHCenter);
    stationItems_.append(state);
    state->show();
    layoutStationItems();
}

void HomePage::showEmptyState()
{
    clearList();
    auto* empty = new QLabel(QStringLiteral("暂无附近充电站\n请搜索其他地址后重试"), this);
    empty->setProperty("homeState", true);
    empty->setAlignment(Qt::AlignCenter);
    empty->setStyleSheet(QStringLiteral(
        "font-size: 14px; line-height: 1.6; color: %1; background: transparent;")
        .arg(cssColor(theme::textSecondary())));
    stationItems_.append(empty);
    empty->show();
    layoutStationItems();
}

void HomePage::renderStations()
{
    clearList();
    for (const StationInfo& station : stations_) {
        auto* card = new StationCard(station, this);
        connect(card, &StationCard::selected,
                this, &HomePage::stationSelected);
        stationItems_.append(card);
        card->show();
    }
    layoutStationItems();
    update();
}

void HomePage::layoutStationItems()
{
    if (width() <= 0 || height() <= 0) {
        return;
    }

    const qreal sx = static_cast<qreal>(width()) / theme::loginCanvasWidth;
    const qreal sy = static_cast<qreal>(height()) / theme::loginCanvasHeight;
    const int margin = qRound(17 * sx);
    const int contentWidth = width() - margin * 2;
    int cardIndex = 0;

    for (QWidget* item : stationItems_) {
        const bool stateWidget = item->property("homeState").toBool();
        if (stateWidget) {
            const int stateTop = qRound(theme::homeCardsTop * sy);
            const int stateHeight = qMax(qRound(theme::homeCardHeight * sy),
                                        qRound((theme::homeFooterTop
                                                - theme::homeCardsTop - 4) * sy));
            item->setGeometry(margin, stateTop, contentWidth, stateHeight);
        } else {
            const int top = qRound((theme::homeCardsTop
                                    + cardIndex * (theme::homeCardHeight
                                                   + theme::homeCardGap)) * sy);
            item->setGeometry(margin, top, contentWidth,
                              qRound(theme::homeCardHeight * sy));
            ++cardIndex;
        }
        item->show();
    }
}

void HomePage::clearList()
{
    for (QWidget* item : stationItems_) {
        // 列表是移动端直接定位布局，刷新时立即销毁旧卡片，避免旧的 loading
        // 状态在同一帧中残留并与错误/空状态重叠。
        delete item;
    }
    stationItems_.clear();
}

void HomePage::onSortClicked()
{
    auto* button = qobject_cast<QPushButton*>(sender());
    const int index = sortButtons_.indexOf(button);
    if (index >= 0) {
        applySort(index);
    }
}
