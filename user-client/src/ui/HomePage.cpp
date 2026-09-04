#include "ui/HomePage.h"

#include "config/AppConfig.h"
#include "geo/Geocoder.h"
#include "net/NetworkClient.h"
#include "protocol/Protocol.h"
#include "session/Session.h"
#include "ui/StationCard.h"
#include "ui/SvgIcon.h"
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
#include <QScrollArea>
#include <QShowEvent>
#include <QSizePolicy>
#include <QTimer>
#include <QVBoxLayout>
#include <QWebEngineView>
#include <QUrl>
#include <algorithm>

namespace {

// 与 WSL 版本保持一致的默认定位，只在首次进入主页且没有已有定位时使用。
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
        const QPixmap searchIcon = tintedSvgPixmap(
            QStringLiteral(":/resources/icons/search.svg"), QSize(26, 26),
            theme::textSecondary());
        if (!searchIcon.isNull()) {
            painter.drawPixmap(10, (height() - searchIcon.height()) / 2, searchIcon);
        }
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
        const QPixmap locateIcon = tintedSvgPixmap(
            QStringLiteral(":/resources/icons/locate.svg"), QSize(28, 28),
            theme::textPrimary());
        if (!locateIcon.isNull()) {
            painter.drawPixmap((width() - locateIcon.width()) / 2,
                               (height() - locateIcon.height()) / 2,
                               locateIcon);
        }
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
        QString iconPath;
        switch (kind_) {
        case Kind::Home: iconPath = QStringLiteral(":/resources/icons/home.svg"); break;
        case Kind::Station: iconPath = QStringLiteral(":/resources/icons/station.svg"); break;
        case Kind::Charge: iconPath = QStringLiteral(":/resources/icons/charge.svg"); break;
        case Kind::Order: iconPath = QStringLiteral(":/resources/icons/order.svg"); break;
        case Kind::Profile: iconPath = QStringLiteral(":/resources/icons/profile.svg"); break;
        }
        const QPixmap icon = tintedSvgPixmap(iconPath, QSize(27, 27), color);
        if (!icon.isNull()) {
            painter.drawPixmap((width() - icon.width()) / 2, 3, icon);
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
        {2001, QStringLiteral("北理良乡南门充电站"), 1.20, 4, 2, 0.2},
        {2002, QStringLiteral("良乡大学城快充站"), 1.50, 4, 2, 0.4},
        {2003, QStringLiteral("房山长阳智慧充电站"), 1.80, 4, 0, 2.6},
        {2004, QStringLiteral("良乡东路超级充电站"), 1.35, 4, 3, 0.8},
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

    stationScroll_ = new QScrollArea(this);
    stationScroll_->setObjectName(QStringLiteral("StationScroll"));
    stationScroll_->setFrameShape(QFrame::NoFrame);
    stationScroll_->setWidgetResizable(false);
    stationScroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    stationScroll_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    stationScroll_->setStyleSheet(QStringLiteral(
        "QScrollArea#StationScroll { background: transparent; border: none; }"));
    stationContainer_ = new QWidget();
    stationContainer_->setStyleSheet(QStringLiteral("background: transparent;"));
    stationScroll_->setWidget(stationContainer_);

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
        "<span style='color:%2;'>电量监控</span>"
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

    // 底部栏是页面导航入口，不再是仅用于展示的静态图标。
    connect(navButtons_[1], &QPushButton::clicked,
            this, &HomePage::stationsRequested);
    connect(navButtons_[2], &QPushButton::clicked,
            this, &HomePage::chargingRequested);
    connect(navButtons_[3], &QPushButton::clicked,
            this, &HomePage::ordersRequested);
    connect(navButtons_[4], &QPushButton::clicked,
            this, &HomePage::profileRequested);

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

    stationScroll_->setGeometry(margin, qRound(theme::homeCardsTop * sy),
                                contentWidth,
                                qRound((theme::homeFooterTop - theme::homeCardsTop - 4) * sy));

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
    if (firstShowHandled_ && !previewMode_) {
        // 从订单/个人中心返回时刷新一次，保证站点空闲数来自服务端最新状态。
        refreshStations();
        return;
    }
    firstShowHandled_ = true;

    if (previewMode_) {
        // 视觉预览不请求站点接口，但仍让配置了高德 key 的环境显示真实地图。
        Session::instance().setLocation(
            appConfig::kDefaultLocationLatitude,
            appConfig::kDefaultLocationLongitude,
            QStringLiteral("%1（预览定位）")
                .arg(QString::fromUtf8(appConfig::kDefaultLocationLabel)));
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
        appConfig::kDefaultLocationLatitude,
        appConfig::kDefaultLocationLongitude,
        QStringLiteral("%1（默认定位）")
            .arg(QString::fromUtf8(appConfig::kDefaultLocationLabel)));
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
    auto* loading = new QLabel(QStringLiteral("加载附近站点中..."), stationContainer_);
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
    auto* state = new QFrame(stationContainer_);
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
    auto* empty = new QLabel(QStringLiteral("暂无附近充电站\n请搜索其他地址后重试"), stationContainer_);
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
        auto* card = new StationCard(station, stationContainer_);
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
    const int viewportHeight = qMax(1, qRound((theme::homeFooterTop
                                               - theme::homeCardsTop - 4) * sy));
    const int itemHeight = qRound(theme::homeCardHeight * sy);
    const int itemGap = qRound(theme::homeCardGap * sy);
    const int containerHeight = qMax(viewportHeight,
                                     stationItems_.size() * itemHeight +
                                         qMax(0, stationItems_.size() - 1) * itemGap);
    stationContainer_->resize(contentWidth, containerHeight);
    int cardIndex = 0;

    for (QWidget* item : stationItems_) {
        const bool stateWidget = item->property("homeState").toBool();
        if (stateWidget) {
            item->setGeometry(0, 0, contentWidth, viewportHeight);
        } else {
            const int top = cardIndex * (itemHeight + itemGap);
            item->setGeometry(0, top, contentWidth, itemHeight);
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
