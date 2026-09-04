/*
THESIS: Turn the charging journey into one continuous city-energy line; refuse the generic dashboard card grid.
OWN-WORLD: Mineral-white ground, ink-blue type, a cobalt route line, and green nodes reserved for real availability.
STORY: Locate, choose a live node, charge, settle, and always see the server-confirmed state.
FIRST VIEWPORT: Location and search lead into a large route-map field; nearby stations continue below; navigation stays thumb-reachable.
FORM: City transit diagram, grounded direction 3, seed 5d277190.
FINISH: unreviewed and undocumented is unfinished; this build ends with the finish review, the verdict, DESIGN.md, and every shipping raster carrying its provenance.
*/

#include "ui/MobileApp.h"

#include "geo/Geocoder.h"
#include "geo/RoutePlanner.h"
#include "config/AppConfig.h"
#include "net/NetworkClient.h"
#include "protocol/Protocol.h"
#include "session/Session.h"
#include "ui/theme/Theme.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QDateTime>
#include <QDoubleValidator>
#include <QFile>
#include <QFrame>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QStackedWidget>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>
#include <QWebEnginePage>
#include <QWebEngineView>
#include <QtMath>
#include <initializer_list>

namespace {

QLabel* label(const QString& text, const char* name = nullptr)
{
    auto* result = new QLabel(text);
    if (name) result->setObjectName(QString::fromLatin1(name));
    result->setWordWrap(true);
    return result;
}

QPushButton* button(const QString& text, const char* name = "Primary")
{
    auto* result = new QPushButton(text);
    result->setObjectName(QString::fromLatin1(name));
    result->setCursor(Qt::PointingHandCursor);
    return result;
}

QFrame* surface()
{
    auto* frame = new QFrame;
    frame->setObjectName(QStringLiteral("Surface"));
    return frame;
}

QWidget* bottomActionBar(std::initializer_list<QPushButton*> actions)
{
    auto* bar = new QWidget;
    auto* layout = new QVBoxLayout(bar);
    layout->setContentsMargins(18, 8, 18, 18);
    layout->setSpacing(10);
    for (QPushButton* action : actions) layout->addWidget(action);
    return bar;
}

void clearLayout(QLayout* layout)
{
    while (QLayoutItem* item = layout->takeAt(0)) {
        if (item->widget()) item->widget()->deleteLater();
        if (item->layout()) clearLayout(item->layout());
        delete item;
    }
}

QString statusName(OrderInfo::Status status)
{
    switch (status) {
    case OrderInfo::StatusReserved: return QStringLiteral("已预约");
    case OrderInfo::StatusCharging: return QStringLiteral("充电中");
    case OrderInfo::StatusWaitSettlement: return QStringLiteral("待结算");
    case OrderInfo::StatusFinished: return QStringLiteral("已完成");
    case OrderInfo::StatusCancelled: return QStringLiteral("已取消");
    default: return QStringLiteral("暂无进行中的订单");
    }
}

QString durationText(qint64 durationMs)
{
    const qint64 minutes = qMax<qint64>(0, durationMs / 60000);
    return QStringLiteral("%1小时%2分").arg(minutes / 60).arg(minutes % 60, 2, 10, QLatin1Char('0'));
}

OrderInfo orderFromPayload(const QJsonObject& payload)
{
    const QJsonObject nested = payload.value(QStringLiteral("order")).toObject();
    return OrderInfo::fromJson(nested.isEmpty() ? payload : nested);
}

class NavButton final : public QAbstractButton
{
public:
    explicit NavButton(const QString& text, int icon, QWidget* parent = nullptr)
        : QAbstractButton(parent), icon_(icon)
    {
        setText(text);
        setCheckable(true);
        setCursor(Qt::PointingHandCursor);
        setMinimumHeight(62);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }
protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QColor color(isChecked() ? Theme::Blue : Theme::Muted);
        if (isChecked()) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(Theme::BlueSoft));
            p.drawRoundedRect(QRectF(width() / 2.0 - 23, 6, 46, 28), 14, 14);
        }
        QPen pen(color, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        const QPointF c(width() / 2.0, 20);
        if (icon_ == 0) {
            QPainterPath path; path.moveTo(c.x()-8,c.y()); path.lineTo(c.x(),c.y()-7); path.lineTo(c.x()+8,c.y());
            path.lineTo(c.x()+8,c.y()+8); path.lineTo(c.x()+2,c.y()+8); path.lineTo(c.x()+2,c.y()+2);
            path.lineTo(c.x()-2,c.y()+2); path.lineTo(c.x()-2,c.y()+8); path.lineTo(c.x()-8,c.y()+8); path.closeSubpath(); p.drawPath(path);
        } else if (icon_ == 1) {
            p.drawArc(QRectF(c.x()-8,c.y()-8,16,16), 25*16, 285*16); p.drawLine(c.x()+1,c.y()-9,c.x()-3,c.y()); p.drawLine(c.x()-3,c.y(),c.x()+2,c.y()); p.drawLine(c.x()+2,c.y(),c.x()-2,c.y()+9);
        } else if (icon_ == 2) {
            p.drawRoundedRect(QRectF(c.x()-8,c.y()-8,16,17),3,3); p.drawLine(c.x()-4,c.y()-3,c.x()+4,c.y()-3); p.drawLine(c.x()-4,c.y()+1,c.x()+2,c.y()+1);
        } else {
            p.drawEllipse(QRectF(c.x()-4,c.y()-8,8,8)); p.drawArc(QRectF(c.x()-9,c.y(),18,14),0,180*16);
        }
        p.setPen(color);
        QFont f = font(); f.setPixelSize(11); f.setWeight(isChecked() ? QFont::DemiBold : QFont::Normal); p.setFont(f);
        p.drawText(QRect(0, 39, width(), 18), Qt::AlignCenter, text());
    }
private:
    int icon_ = 0;
};

} // namespace

EnergyMapWidget::EnergyMapWidget(QWidget* parent) : QWidget(parent)
{
    setMinimumHeight(285);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void EnergyMapWidget::setStations(const QList<StationInfo>& stations)
{
    stations_ = stations;
    update();
}

void EnergyMapWidget::setCaption(const QString& caption)
{
    caption_ = caption;
    update();
}

void EnergyMapWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#E9EFED"));
    p.drawRoundedRect(rect(), 20, 20);

    QPen street(QColor("#FFFFFF"), 13, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(street);
    QPainterPath a; a.moveTo(-20, height()*0.72); a.cubicTo(width()*0.24,height()*0.48,width()*0.46,height()*0.84,width()+20,height()*0.34); p.drawPath(a);
    QPainterPath b; b.moveTo(width()*0.18,-10); b.cubicTo(width()*0.35,height()*0.3,width()*0.22,height()*0.55,width()*0.5,height()+20); p.drawPath(b);
    QPainterPath c; c.moveTo(width()*0.78,-10); c.lineTo(width()*0.62,height()+15); p.drawPath(c);

    QPen route(QColor(Theme::Blue), 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(route);
    QPainterPath line; line.moveTo(width()*0.12,height()*0.76); line.cubicTo(width()*0.35,height()*0.5,width()*0.52,height()*0.72,width()*0.82,height()*0.3); p.drawPath(line);

    const int count = qMin(3, stations_.size());
    const QPointF points[3] = {{width()*0.26,height()*0.64},{width()*0.54,height()*0.59},{width()*0.79,height()*0.34}};
    for (int i = 0; i < count; ++i) {
        p.setPen(QPen(Qt::white, 4)); p.setBrush(QColor(stations_[i].availableChargers > 0 ? Theme::Green : Theme::Amber));
        p.drawEllipse(points[i], 9, 9);
    }
    if (stations_.isEmpty()) {
        p.setPen(QColor(Theme::Muted));
        QFont f = font(); f.setPixelSize(13); p.setFont(f);
        p.drawText(rect().adjusted(24, 24, -24, -24), Qt::AlignCenter, caption_);
    }
}

AmapWidget::AmapWidget(QWidget* parent) : QWidget(parent)
{
    setMinimumHeight(285);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    const QString apiKey = qEnvironmentVariable(appConfig::kAmapJsApiKeyEnvironment);
    if (apiKey.isEmpty()) {
        fallback_ = new EnergyMapWidget;
        fallback_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        layout->addWidget(fallback_);
        return;
    }

    QFile source(QStringLiteral(":/resources/home-map.html"));
    if (!source.open(QIODevice::ReadOnly)) {
        fallback_ = new EnergyMapWidget;
        fallback_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        layout->addWidget(fallback_);
        return;
    }

    QByteArray html = source.readAll();
    html.replace("__AMAP_KEY__", apiKey.toUtf8());
    html.replace("__AMAP_SECRET__",
                 qEnvironmentVariable(appConfig::kAmapJsApiSecretEnvironment).toUtf8());
    webView_ = new QWebEngineView;
    webView_->setContextMenuPolicy(Qt::NoContextMenu);
    webView_->page()->setBackgroundColor(QColor("#E9EFED"));
    layout->addWidget(webView_);
    liveMap_ = true;
    connect(webView_, &QWebEngineView::loadFinished, this, [this](bool ok) {
        loaded_ = ok;
        if (!ok) return;
        const auto scripts = queuedScripts_;
        queuedScripts_.clear();
        for (const QString& script : scripts) runScript(script);
    });
    webView_->setHtml(QString::fromUtf8(html), QUrl(QStringLiteral("https://localhost/")));
}

void AmapWidget::runScript(const QString& script)
{
    if (!webView_) return;
    if (!loaded_) {
        queuedScripts_.append(script);
        return;
    }
    webView_->page()->runJavaScript(script);
}

void AmapWidget::setStations(const QList<StationInfo>& stations)
{
    if (fallback_) fallback_->setStations(stations);
    QJsonArray items;
    for (const StationInfo& station : stations) {
        if (qFuzzyIsNull(station.latitude) || qFuzzyIsNull(station.longitude)) continue;
        QJsonObject item;
        item.insert(QStringLiteral("name"), station.name);
        item.insert(QStringLiteral("latitude"), station.latitude);
        item.insert(QStringLiteral("longitude"), station.longitude);
        item.insert(QStringLiteral("available"), station.availableChargers);
        items.append(item);
    }
    runScript(QStringLiteral("window.setStations(%1)")
        .arg(QString::fromUtf8(QJsonDocument(items).toJson(QJsonDocument::Compact))));
}

void AmapWidget::setCenter(double latitude, double longitude, const QString& label)
{
    if (fallback_ && !label.isEmpty()) fallback_->setCaption(label);
    runScript(QStringLiteral("window.setCenter(%1,%2)")
        .arg(latitude, 0, 'f', 7).arg(longitude, 0, 'f', 7));
}

void AmapWidget::setRoute(const RouteResult& route)
{
    QJsonArray points;
    for (const QPointF& point : route.path) {
        QJsonObject value;
        value.insert(QStringLiteral("lng"), point.x());
        value.insert(QStringLiteral("lat"), point.y());
        points.append(value);
    }
    runScript(QStringLiteral("window.setRoute(%1)")
        .arg(QString::fromUtf8(QJsonDocument(points).toJson(QJsonDocument::Compact))));
}

ChargeGauge::ChargeGauge(QWidget* parent) : QWidget(parent)
{
    setMinimumSize(270, 270);
}

void ChargeGauge::setValue(double percent, const QString& centerText, const QString& caption)
{
    percent_ = qBound(0.0, percent, 100.0);
    centerText_ = centerText;
    caption_ = caption;
    update();
}

void ChargeGauge::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const qreal side = qMin(width(), height()) - 36;
    const QRectF ring((width()-side)/2, (height()-side)/2, side, side);
    QPen base(QColor("#DFE8E8"), 15, Qt::SolidLine, Qt::RoundCap); p.setPen(base); p.drawArc(ring, 225*16, -270*16);
    QPen live(QColor(Theme::Green), 15, Qt::SolidLine, Qt::RoundCap); p.setPen(live); p.drawArc(ring, 225*16, int(-270*16*percent_/100.0));
    p.setPen(QColor(Theme::Ink));
    QFont f = font(); f.setPixelSize(40); f.setWeight(QFont::Bold); p.setFont(f);
    p.drawText(rect().adjusted(0,58,0,-60), Qt::AlignCenter, centerText_);
    f.setPixelSize(12); f.setWeight(QFont::Normal); p.setFont(f); p.setPen(QColor(Theme::Muted));
    p.drawText(rect().adjusted(0,105,0,-35), Qt::AlignCenter, caption_);
}

MobileApp::MobileApp(NetworkClient* network, QWidget* parent)
    : QMainWindow(parent), network_(network), geocoder_(new Geocoder(this)), routePlanner_(new RoutePlanner(this))
{
    setWindowTitle(QStringLiteral("东软充电"));
    resize(430, 860);
    setMinimumSize(390, 720);
    setMaximumWidth(520);

    auto* root = new QWidget;
    root->setObjectName(QStringLiteral("AppRoot"));
    auto* layout = new QVBoxLayout(root);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    pages_ = new QStackedWidget;
    pages_->addWidget(buildLoginPage());
    pages_->addWidget(buildHomePage());
    pages_->addWidget(buildStationPage());
    pages_->addWidget(buildChargerPage());
    pages_->addWidget(buildNavigationPage());
    pages_->addWidget(buildChargingPage());
    pages_->addWidget(buildOrdersPage());
    pages_->addWidget(buildProfilePage());
    bottomNav_ = buildBottomNavigation();
    layout->addWidget(pages_, 1);
    layout->addWidget(bottomNav_);
    setCentralWidget(root);

    connect(network_, &NetworkClient::connected, this, [this] { updateConnectionState(true); });
    connect(network_, &NetworkClient::disconnected, this, [this] { updateConnectionState(false); });
    connect(network_, &NetworkClient::transportError, this, [this](int code, const QString& message) {
        if (pages_->currentIndex() == LoginPage) {
            setNotice(loginNotice_, protocol::describeError(code, message), true);
        }
    });
    connect(geocoder_, &Geocoder::geocoded, this, [this](double lat, double lng, const QString& address) {
        Session::instance().setLocation(lat, lng, address);
        locateButton_->setEnabled(true);
        locateButton_->setText(QStringLiteral("更新位置"));
        refreshSessionViews();
        mapWidget_->setCenter(lat, lng, address);
        requestNearbyStations();
    });
    connect(geocoder_, &Geocoder::error, this, [this](const QString& message) {
        setBusy(locateButton_, false, QStringLiteral("更新位置"));
        setNotice(homeNotice_, message, true);
    });
    connect(routePlanner_, &RoutePlanner::routeReady, this, [this](const RouteResult& route) {
        if (pages_->currentIndex() == NavigationPage) {
            navigationMap_->setRoute(route);
            navigationDistance_->setText(QStringLiteral("%1 km")
                .arg(route.distanceMeters / 1000.0, 0, 'f', 1));
            navigationDuration_->setText(QStringLiteral("%1 分钟")
                .arg(qMax<qint64>(1, route.durationSeconds / 60)));
            setNotice(navigationNotice_, route.firstInstruction);
        } else {
            setNotice(detailNotice_, QStringLiteral("%1 公里 · 约 %2 分钟")
                .arg(route.distanceMeters / 1000.0, 0, 'f', 1)
                .arg(qMax<qint64>(1, route.durationSeconds / 60)));
        }
    });
    connect(routePlanner_, &RoutePlanner::error, this, [this](const QString& message) {
        setNotice(pages_->currentIndex() == NavigationPage
                      ? navigationNotice_ : detailNotice_, message, true);
    });

    updateConnectionState(false);
    showPage(Session::instance().isLoggedIn() ? HomePage : LoginPage);
    refreshSessionViews();
    if (Session::instance().hasLocation()) {
        mapWidget_->setCenter(Session::instance().latitude(),
            Session::instance().longitude(), Session::instance().locationLabel());
    }
    const QString previewPage = qEnvironmentVariable("EV_PREVIEW_PAGE");
    if (Session::instance().isLoggedIn() && previewPage == QStringLiteral("charging")) {
        showPage(ChargingPage);
    } else if (Session::instance().isLoggedIn() && previewPage == QStringLiteral("profile")) {
        showPage(ProfilePage);
    } else if (Session::instance().isLoggedIn()
               && (previewPage == QStringLiteral("station")
                   || previewPage == QStringLiteral("charger")
                   || previewPage == QStringLiteral("navigation"))) {
        StationDetail preview;
        preview.station.stationId = 1;
        preview.station.name = QStringLiteral("北理良乡南门充电站");
        preview.station.pricePerKwh = 1.28;
        preview.station.totalChargers = 12;
        preview.station.availableChargers = 5;
        preview.address = QStringLiteral("北京市房山区良乡高教园区");
        preview.latitude = appConfig::kPreviewStationLatitude;
        preview.longitude = appConfig::kPreviewStationLongitude;
        ChargerInfo fast;
        fast.chargerId = 11;
        fast.code = QStringLiteral("A-001");
        fast.type = protocol::ChargerTypeFast;
        fast.status = protocol::ChargerStatusIdle;
        fast.powerKw = 120;
        preview.chargers.append(fast);
        ChargerInfo busy = fast;
        busy.chargerId = 12;
        busy.code = QStringLiteral("A-002");
        busy.status = protocol::ChargerStatusCharging;
        preview.chargers.append(busy);
        renderStationDetail(preview);
        if (previewPage == QStringLiteral("station")) showPage(StationPage);
        else {
            showChargerDetail(fast);
            if (previewPage == QStringLiteral("navigation")) showNavigation();
        }
    }
    auto* orderPoll = new QTimer(this);
    orderPoll->setInterval(5000);
    connect(orderPoll, &QTimer::timeout, this, [this] {
        if (pages_->currentIndex() == ChargingPage
            && activeOrder_.statusEnum() == OrderInfo::StatusCharging) {
            requestActiveOrder(ChargingPage);
        }
    });
    orderPoll->start();
    network_->connectToServer(QString::fromLatin1(protocol::kDefaultHost), protocol::kDefaultPort);
}

QWidget* MobileApp::buildLoginPage()
{
    auto* page = new QWidget; page->setObjectName(QStringLiteral("Page"));
    auto* layout = new QVBoxLayout(page); layout->setContentsMargins(28, 34, 28, 30); layout->setSpacing(16);
    auto* brand = label(QStringLiteral("NEU · CHARGE"), "Muted"); brand->setStyleSheet(QStringLiteral("color:#176CFF;font-weight:700;letter-spacing:1px"));
    connectionLabel_ = label(QStringLiteral("正在连接服务"), "StatusWarn"); connectionLabel_->setAlignment(Qt::AlignCenter); connectionLabel_->setFixedWidth(112);
    auto* top = new QHBoxLayout; top->addWidget(brand); top->addStretch(); top->addWidget(connectionLabel_); layout->addLayout(top);
    layout->addStretch(2);

    auto* marker = new QFrame; marker->setFixedSize(56, 6); marker->setStyleSheet(QStringLiteral("background:#176CFF;border-radius:3px")); layout->addWidget(marker);
    auto* title = label(QStringLiteral("从更近的能源线路出发"), "PageTitle"); title->setStyleSheet(QStringLiteral("font-size:36px")); layout->addWidget(title);
    layout->addWidget(label(QStringLiteral("手机号登录，新用户自动注册"), "Muted"));
    layout->addSpacing(30);
    phoneInput_ = new QLineEdit; phoneInput_->setPlaceholderText(QStringLiteral("请输入 11 位手机号")); phoneInput_->setMaxLength(11); phoneInput_->setInputMethodHints(Qt::ImhDigitsOnly); layout->addWidget(phoneInput_);
    loginButton_ = button(QStringLiteral("登录 / 自动注册")); layout->addWidget(loginButton_);
    loginNotice_ = label(QString(), "Muted"); loginNotice_->setMinimumHeight(36); layout->addWidget(loginNotice_);
    layout->addStretch(3);
    connect(loginButton_, &QPushButton::clicked, this, &MobileApp::attemptLogin);
    connect(phoneInput_, &QLineEdit::returnPressed, this, &MobileApp::attemptLogin);
    return page;
}

QWidget* MobileApp::buildHomePage()
{
    auto* page = new QWidget; page->setObjectName(QStringLiteral("Page"));
    auto* outer = new QVBoxLayout(page); outer->setContentsMargins(0,0,0,0);
    auto* scroll = new QScrollArea; scroll->setWidgetResizable(true); auto* body = new QWidget; auto* layout = new QVBoxLayout(body); layout->setContentsMargins(14,18,14,22); layout->setSpacing(10);
    auto* titleRow = new QHBoxLayout; auto* texts = new QVBoxLayout; texts->setSpacing(2); texts->addWidget(label(QStringLiteral("附近充电站"), "PageTitle")); locationLabel_ = label(QStringLiteral("尚未设置位置"), "Muted"); texts->addWidget(locationLabel_); titleRow->addLayout(texts); titleRow->addStretch(); auto* refresh = button(QStringLiteral("刷新"), "Quiet"); refresh->setFixedWidth(58); titleRow->addWidget(refresh); layout->addLayout(titleRow);
    auto* search = new QHBoxLayout; search->setSpacing(8); addressInput_ = new QLineEdit; addressInput_->setPlaceholderText(QStringLiteral("输入城市、区域或详细地址")); locateButton_ = button(QStringLiteral("定位"), "Primary"); locateButton_->setFixedWidth(86); search->addWidget(addressInput_); search->addWidget(locateButton_); layout->addLayout(search);
    homeNotice_ = label(QString(), "Muted"); homeNotice_->hide(); layout->addWidget(homeNotice_);
    mapWidget_ = new AmapWidget; layout->addWidget(mapWidget_);
    auto* listTitle = new QHBoxLayout; listTitle->addWidget(label(QStringLiteral("沿线站点"), "SectionTitle")); listTitle->addStretch(); layout->addLayout(listTitle);
    stationListBody_ = new QWidget; stationListLayout_ = new QVBoxLayout(stationListBody_); stationListLayout_->setContentsMargins(0,0,0,0); stationListLayout_->setSpacing(10); layout->addWidget(stationListBody_); layout->addStretch(); scroll->setWidget(body); outer->addWidget(scroll);
    connect(locateButton_, &QPushButton::clicked, this, &MobileApp::locateAddress); connect(addressInput_, &QLineEdit::returnPressed, this, &MobileApp::locateAddress); connect(refresh, &QPushButton::clicked, this, &MobileApp::requestNearbyStations);
    return page;
}

QWidget* MobileApp::buildStationPage()
{
    auto* page = new QWidget; page->setObjectName(QStringLiteral("Page")); auto* outer = new QVBoxLayout(page); outer->setContentsMargins(0,0,0,0);
    auto* scroll = new QScrollArea; scroll->setWidgetResizable(true); auto* body = new QWidget; auto* layout = new QVBoxLayout(body); layout->setContentsMargins(18,18,18,12); layout->setSpacing(12);
    auto* back = button(QStringLiteral("返回"), "Quiet"); back->setFixedWidth(72); layout->addWidget(back,0,Qt::AlignLeft);
    auto* stationPlate = new QFrame; stationPlate->setObjectName(QStringLiteral("SoftSurface")); auto* plateLayout = new QVBoxLayout(stationPlate); plateLayout->setContentsMargins(18,18,18,18); plateLayout->setSpacing(7);
    auto* heading = new QHBoxLayout; detailName_ = label(QStringLiteral("充电站"), "PageTitle"); detailName_->setStyleSheet(QStringLiteral("font-size:24px")); heading->addWidget(detailName_, 1); detailMeta_ = label(QStringLiteral("读取中"), "StatusInfo"); detailMeta_->setAlignment(Qt::AlignCenter); detailMeta_->setFixedWidth(96); heading->addWidget(detailMeta_); plateLayout->addLayout(heading);
    detailAddress_ = label(QString(), "Muted"); plateLayout->addWidget(detailAddress_); plateLayout->addSpacing(12);
    auto* metricLayout = new QHBoxLayout; auto* priceBox = new QVBoxLayout; priceBox->addWidget(label(QStringLiteral("电价"), "Muted")); detailPrice_ = label(QStringLiteral("¥ —"), "Amount"); priceBox->addWidget(detailPrice_); metricLayout->addLayout(priceBox); metricLayout->addStretch();
    auto* availableBox = new QVBoxLayout; availableBox->addWidget(label(QStringLiteral("当前空闲"), "Muted")); detailAvailability_ = label(QStringLiteral("—"), "Metric"); detailAvailability_->setStyleSheet(QStringLiteral("color:#149B68")); availableBox->addWidget(detailAvailability_); metricLayout->addLayout(availableBox); plateLayout->addLayout(metricLayout); layout->addWidget(stationPlate);
    detailNotice_ = label(QString(), "Muted"); layout->addWidget(detailNotice_); layout->addSpacing(8); layout->addWidget(label(QStringLiteral("充电桩"), "SectionTitle")); chargerListBody_ = new QWidget; chargerListLayout_ = new QVBoxLayout(chargerListBody_); chargerListLayout_->setContentsMargins(0,0,0,0); chargerListLayout_->setSpacing(10); layout->addWidget(chargerListBody_); layout->addStretch(); scroll->setWidget(body); outer->addWidget(scroll,1);
    auto* route = button(QStringLiteral("导航到充电站"), "Primary"); outer->addWidget(bottomActionBar({route}));
    connect(back,&QPushButton::clicked,this,[this]{showPage(HomePage);});
    connect(route,&QPushButton::clicked,this,[this]{ currentCharger_ = ChargerInfo{}; showNavigation(); });
    return page;
}

QWidget* MobileApp::buildChargerPage()
{
    auto* page = new QWidget; page->setObjectName(QStringLiteral("Page")); auto* outer = new QVBoxLayout(page); outer->setContentsMargins(0,0,0,0); outer->setSpacing(0);
    auto* body = new QWidget; auto* layout = new QVBoxLayout(body); layout->setContentsMargins(18,18,18,0); layout->setSpacing(14); outer->addWidget(body,1);
    auto* back = button(QStringLiteral("返回"), "Quiet"); back->setFixedWidth(72); layout->addWidget(back,0,Qt::AlignLeft);
    auto* powerPlate = new QFrame; powerPlate->setObjectName(QStringLiteral("SoftSurface")); auto* powerLayout = new QVBoxLayout(powerPlate); powerLayout->setContentsMargins(22,22,22,22);
    auto* identity = new QHBoxLayout; chargerCode_ = label(QStringLiteral("充电桩"), "PageTitle"); identity->addWidget(chargerCode_,1); chargerStatus_ = label(QStringLiteral("读取中"), "StatusInfo"); chargerStatus_->setAlignment(Qt::AlignCenter); chargerStatus_->setFixedWidth(92); identity->addWidget(chargerStatus_); powerLayout->addLayout(identity);
    powerLayout->addSpacing(14);
    powerLayout->addWidget(label(QStringLiteral("额定功率"), "Muted")); chargerPower_ = label(QStringLiteral("— kW"), "Metric"); chargerPower_->setStyleSheet(QStringLiteral("font-size:48px")); powerLayout->addWidget(chargerPower_); chargerMeta_ = label(QString(), "Muted"); powerLayout->addWidget(chargerMeta_); layout->addWidget(powerPlate);

    auto* guide = new QFrame; guide->setObjectName(QStringLiteral("Surface")); auto* guideLayout = new QVBoxLayout(guide); guideLayout->setContentsMargins(18,16,18,16); guideLayout->setSpacing(7);
    guideLayout->addWidget(label(QStringLiteral("到站后"), "SectionTitle")); guideLayout->addWidget(label(QStringLiteral("确认桩编号 → 插枪 → 开始充电"), "Muted")); layout->addWidget(guide);
    chargerNotice_ = label(QString(), "Muted"); layout->addWidget(chargerNotice_); layout->addStretch();
    auto* navigate = button(QStringLiteral("导航到充电桩"), "Secondary"); reserveButton_ = button(QStringLiteral("预约充电桩"), "Primary"); outer->addWidget(bottomActionBar({navigate,reserveButton_}));
    connect(back,&QPushButton::clicked,this,[this]{showPage(StationPage);});
    connect(navigate,&QPushButton::clicked,this,&MobileApp::showNavigation);
    connect(reserveButton_,&QPushButton::clicked,this,[this]{reserveCharger(currentCharger_);});
    return page;
}

QWidget* MobileApp::buildNavigationPage()
{
    auto* page = new QWidget; page->setObjectName(QStringLiteral("Page"));
    auto* layout = new QVBoxLayout(page); layout->setContentsMargins(0,0,0,0); layout->setSpacing(0);
    auto* top = new QWidget; auto* topLayout = new QHBoxLayout(top); topLayout->setContentsMargins(14,14,14,10); auto* back = button(QStringLiteral("返回"), "Quiet"); back->setFixedWidth(72); topLayout->addWidget(back); topLayout->addStretch(); topLayout->addWidget(label(QStringLiteral("路线导航"), "SectionTitle")); topLayout->addStretch(); auto* spacer = new QWidget; spacer->setFixedWidth(72); topLayout->addWidget(spacer); layout->addWidget(top);
    navigationMap_ = new AmapWidget; navigationMap_->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding); layout->addWidget(navigationMap_,1);
    auto* panel = new QFrame; panel->setObjectName(QStringLiteral("Surface")); auto* panelLayout = new QVBoxLayout(panel); panelLayout->setContentsMargins(18,18,18,18); panelLayout->setSpacing(10);
    navigationTarget_ = label(QStringLiteral("正在准备终点"), "SectionTitle"); panelLayout->addWidget(navigationTarget_); auto* metrics = new QHBoxLayout; navigationDuration_ = label(QStringLiteral("— 分钟"), "Metric"); navigationDuration_->setStyleSheet(QStringLiteral("font-size:32px")); navigationDistance_ = label(QStringLiteral("— km"), "Amount"); metrics->addWidget(navigationDuration_); metrics->addStretch(); metrics->addWidget(navigationDistance_,0,Qt::AlignBottom); panelLayout->addLayout(metrics); navigationNotice_ = label(QStringLiteral("正在规划路线…"), "Muted"); panelLayout->addWidget(navigationNotice_); navigationAction_ = button(QStringLiteral("开始导航"), "Primary"); panelLayout->addWidget(navigationAction_); layout->addWidget(panel);
    connect(back,&QPushButton::clicked,this,[this]{showPage(ChargerPage);});
    connect(navigationAction_,&QPushButton::clicked,this,[this]{
        if (navigationAction_->text() == QStringLiteral("开始导航")) {
            navigationAction_->setText(QStringLiteral("结束导航"));
            navigationAction_->setObjectName(QStringLiteral("Danger"));
            navigationAction_->style()->unpolish(navigationAction_); navigationAction_->style()->polish(navigationAction_);
            setNotice(navigationNotice_, QStringLiteral("导航已开始"));
        } else {
            navigationAction_->setText(QStringLiteral("开始导航"));
            navigationAction_->setObjectName(QStringLiteral("Primary"));
            navigationAction_->style()->unpolish(navigationAction_); navigationAction_->style()->polish(navigationAction_);
            showPage(ChargerPage);
        }
    });
    return page;
}

QWidget* MobileApp::buildChargingPage()
{
    auto* page = new QWidget; page->setObjectName(QStringLiteral("Page")); auto* layout = new QVBoxLayout(page); layout->setContentsMargins(22,24,22,28); layout->setSpacing(12);
    auto* top = new QHBoxLayout; top->addWidget(label(QStringLiteral("充电"), "PageTitle")); top->addStretch(); auto* refresh = button(QStringLiteral("同步"), "Quiet"); refresh->setFixedWidth(58); top->addWidget(refresh); layout->addLayout(top);
    chargingStatus_ = label(QStringLiteral("正在同步"), "StatusInfo"); chargingStatus_->setAlignment(Qt::AlignCenter); chargingStatus_->setFixedWidth(112); layout->addWidget(chargingStatus_,0,Qt::AlignHCenter);
    gauge_ = new ChargeGauge; layout->addWidget(gauge_,0,Qt::AlignHCenter); chargingStation_ = label(QStringLiteral("暂无订单"), "SectionTitle"); chargingStation_->setAlignment(Qt::AlignCenter); layout->addWidget(chargingStation_); chargingMetrics_ = label(QStringLiteral("请选择空闲充电桩"), "Muted"); chargingMetrics_->setAlignment(Qt::AlignCenter); layout->addWidget(chargingMetrics_);
    auto* amountSurface = surface(); auto* amountLayout = new QHBoxLayout(amountSurface); amountLayout->setContentsMargins(18,15,18,15); amountLayout->addWidget(label(QStringLiteral("费用"), "Muted")); amountLayout->addStretch(); chargingAmount_ = label(QStringLiteral("¥ —"), "Amount"); amountLayout->addWidget(chargingAmount_); layout->addWidget(amountSurface);
    chargingNotice_ = label(QString(), "Muted"); layout->addWidget(chargingNotice_); layout->addStretch(); chargingAction_ = button(QStringLiteral("查看附近站点"), "Primary"); layout->addWidget(chargingAction_);
    connect(refresh,&QPushButton::clicked,this,[this]{requestActiveOrder(ChargingPage);}); connect(chargingAction_,&QPushButton::clicked,this,[this]{ const auto s=activeOrder_.statusEnum(); if(!activeOrder_.valid()){showPage(HomePage);} else if(s==OrderInfo::StatusReserved){performOrderAction(QString::fromLatin1(protocol::action::kOrderStart),chargingAction_);} else if(s==OrderInfo::StatusCharging){performOrderAction(QString::fromLatin1(protocol::action::kOrderStop),chargingAction_);} else if(s==OrderInfo::StatusWaitSettlement){performOrderAction(QString::fromLatin1(protocol::action::kOrderSettle),chargingAction_);} });
    return page;
}

QWidget* MobileApp::buildOrdersPage()
{
    auto* page = new QWidget; page->setObjectName(QStringLiteral("Page")); auto* layout = new QVBoxLayout(page); layout->setContentsMargins(22,24,22,28); layout->setSpacing(14);
    auto* top = new QHBoxLayout; top->addWidget(label(QStringLiteral("订单"), "PageTitle")); top->addStretch(); auto* refresh = button(QStringLiteral("同步"), "Quiet"); refresh->setFixedWidth(58); top->addWidget(refresh); layout->addLayout(top);
    orderHeadline_ = label(QStringLiteral("暂无订单"), "SectionTitle"); layout->addWidget(orderHeadline_); auto* line = new QFrame; line->setObjectName(QStringLiteral("Divider")); layout->addWidget(line); orderSummary_ = label(QStringLiteral("预约后将在这里显示"), "Muted"); layout->addWidget(orderSummary_); orderNotice_ = label(QString(), "Muted"); layout->addWidget(orderNotice_); layout->addStretch(); auto* go = button(QStringLiteral("查找充电站"), "Secondary"); layout->addWidget(go);
    connect(refresh,&QPushButton::clicked,this,[this]{requestActiveOrder(OrdersPage);}); connect(go,&QPushButton::clicked,this,[this]{showPage(HomePage);}); return page;
}

QWidget* MobileApp::buildProfilePage()
{
    auto* page = new QWidget; page->setObjectName(QStringLiteral("Page")); auto* outer = new QVBoxLayout(page); outer->setContentsMargins(0,0,0,0); auto* scroll = new QScrollArea; scroll->setWidgetResizable(true); auto* body = new QWidget; auto* layout = new QVBoxLayout(body); layout->setContentsMargins(22,24,22,28); layout->setSpacing(14);
    layout->addWidget(label(QStringLiteral("我的"), "PageTitle")); profileName_ = label(QStringLiteral("未登录"), "ProfileName"); profilePhone_ = label(QString(), "Muted"); layout->addWidget(profileName_); layout->addWidget(profilePhone_);
    auto* wallet = new QFrame; wallet->setObjectName(QStringLiteral("SoftSurface")); auto* walletLayout = new QVBoxLayout(wallet); walletLayout->setContentsMargins(20,18,20,18); walletLayout->addWidget(label(QStringLiteral("账户余额"), "Muted")); profileBalance_ = label(QStringLiteral("¥ 0.00"), "Metric"); walletLayout->addWidget(profileBalance_); layout->addWidget(wallet);
    layout->addWidget(label(QStringLiteral("昵称"), "SectionTitle")); auto* nickRow = new QHBoxLayout; nickRow->setSpacing(8); nicknameInput_ = new QLineEdit; nicknameInput_->setPlaceholderText(QStringLiteral("2–20 个字符")); nicknameButton_ = button(QStringLiteral("保存"), "Secondary"); nicknameButton_->setFixedWidth(84); nickRow->addWidget(nicknameInput_); nickRow->addWidget(nicknameButton_); layout->addLayout(nickRow);
    layout->addWidget(label(QStringLiteral("充值"), "SectionTitle")); auto* chargeRow = new QHBoxLayout; chargeRow->setSpacing(8); rechargeInput_ = new QLineEdit; rechargeInput_->setPlaceholderText(QStringLiteral("金额")); rechargeInput_->setValidator(new QDoubleValidator(0.01,100000.0,2,rechargeInput_)); rechargeButton_ = button(QStringLiteral("充值"), "Primary"); rechargeButton_->setFixedWidth(84); chargeRow->addWidget(rechargeInput_); chargeRow->addWidget(rechargeButton_); layout->addLayout(chargeRow);
    profileNotice_ = label(QString(), "Muted"); layout->addWidget(profileNotice_); layout->addSpacing(10); auto* logout = button(QStringLiteral("退出登录"), "Danger"); layout->addWidget(logout); layout->addStretch(); scroll->setWidget(body); outer->addWidget(scroll);
    connect(nicknameButton_,&QPushButton::clicked,this,&MobileApp::updateProfile); connect(rechargeButton_,&QPushButton::clicked,this,&MobileApp::recharge); connect(logout,&QPushButton::clicked,this,[this]{Session::instance().logout(); phoneInput_->clear(); setNotice(loginNotice_,QStringLiteral("已安全退出")); showPage(LoginPage);}); return page;
}

QWidget* MobileApp::buildBottomNavigation()
{
    auto* bar = new QFrame; bar->setStyleSheet(QStringLiteral("QFrame{background:#FFFFFF;border-top:1px solid #DCE5E8}")); auto* layout = new QHBoxLayout(bar); layout->setContentsMargins(8,2,8,6); layout->setSpacing(0); navGroup_ = new QButtonGroup(this); navGroup_->setExclusive(true);
    const QStringList names{QStringLiteral("首页"),QStringLiteral("充电"),QStringLiteral("订单"),QStringLiteral("我的")}; const QList<Page> pages{HomePage,ChargingPage,OrdersPage,ProfilePage};
    for(int i=0;i<names.size();++i){auto* nav=new NavButton(names[i],i); navGroup_->addButton(nav,i); layout->addWidget(nav); connect(nav,&QAbstractButton::clicked,this,[this,page=pages[i]]{ if(page==ChargingPage||page==OrdersPage) requestActiveOrder(page); else showPage(page); }); if(i==0)nav->setChecked(true);} return bar;
}

void MobileApp::showPage(Page page)
{
    pages_->setCurrentIndex(page);
    const bool mainPage = page == HomePage || page == ChargingPage
        || page == OrdersPage || page == ProfilePage;
    bottomNav_->setVisible(mainPage);
    if (!mainPage) return;
    const int navIndex = page == HomePage ? 0 : page == ChargingPage ? 1
        : page == OrdersPage ? 2 : 3;
    if (auto* b = navGroup_->button(navIndex)) b->setChecked(true);
    if (page == ProfilePage) refreshSessionViews();
}

void MobileApp::setBusy(QPushButton* b, bool busy, const QString& normalText){b->setDisabled(busy);b->setText(busy?QStringLiteral("请稍候…"):normalText);}

void MobileApp::setNotice(QLabel* target,const QString& text,bool error){target->setText(text);target->setVisible(!text.isEmpty());target->setStyleSheet(error?QStringLiteral("color:#B83C3C"):QStringLiteral("color:#647487"));}

void MobileApp::attemptLogin()
{
    const QString phone=phoneInput_->text().trimmed(); if(!QRegularExpression(QStringLiteral("^1\\d{10}$")).match(phone).hasMatch()){setNotice(loginNotice_,QStringLiteral("请输入有效的 11 位手机号"),true);return;} setBusy(loginButton_,true,QStringLiteral("登录 / 自动注册")); setNotice(loginNotice_,QStringLiteral("正在验证账号…")); QJsonObject data;data.insert(QStringLiteral("phone"),phone);
    network_->sendRequest(QString::fromLatin1(protocol::action::kUserLogin),data,[this,phone](const protocol::Response& r){setBusy(loginButton_,false,QStringLiteral("登录 / 自动注册"));if(!r.isOk()){setNotice(loginNotice_,protocol::describeError(r.code,r.message),true);return;}const auto d=r.data;Session::instance().setUser(qint64(d.value(QStringLiteral("userId")).toDouble()),d.value(QStringLiteral("phone")).toString(phone),d.value(QStringLiteral("nickname")).toString(QStringLiteral("车主用户")),d.value(QStringLiteral("avatarUrl")).toString(),d.value(QStringLiteral("balance")).toDouble(),d.value(QStringLiteral("status")).toString());refreshSessionViews();showPage(HomePage);});
}

void MobileApp::locateAddress(){const QString address=addressInput_->text().trimmed();if(address.size()<2){setNotice(homeNotice_,QStringLiteral("请输入完整一些的位置"),true);return;}setBusy(locateButton_,true,QStringLiteral("更新位置"));setNotice(homeNotice_,QStringLiteral("正在解析位置…"));geocoder_->geocode(address);}

void MobileApp::requestNearbyStations()
{
    if(!Session::instance().hasLocation()){setNotice(homeNotice_,QStringLiteral("请先输入位置"),true);return;} setNotice(homeNotice_,QStringLiteral("正在同步附近站点…")); QJsonObject data;data.insert(QStringLiteral("latitude"),Session::instance().latitude());data.insert(QStringLiteral("longitude"),Session::instance().longitude());
    network_->sendRequest(QString::fromLatin1(protocol::action::kStationNearby),data,[this](const protocol::Response&r){if(!r.isOk()){setNotice(homeNotice_,protocol::describeError(r.code,r.message),true);renderStationList({});return;}const auto stations=StationInfo::fromJsonArray(r.data.value(QStringLiteral("stations")).toArray());renderStationList(stations);setNotice(homeNotice_,QString());});
}

void MobileApp::renderStationList(const QList<StationInfo>& stations)
{
    clearLayout(stationListLayout_);mapWidget_->setStations(stations);if(stations.isEmpty()){stationListLayout_->addWidget(label(QStringLiteral("没有找到站点。可以更换位置后重试。"),"Muted"));return;}for(const auto&s:stations){auto* row=surface();auto* l=new QHBoxLayout(row);l->setContentsMargins(16,14,12,14);l->setSpacing(10);auto* node=new QFrame;node->setFixedSize(12,12);node->setStyleSheet(QStringLiteral("background:%1;border:3px solid white;border-radius:6px").arg(s.availableChargers>0?Theme::Green:Theme::Amber));l->addWidget(node,0,Qt::AlignTop);auto* text=new QVBoxLayout;text->setSpacing(3);text->addWidget(label(s.name,"SectionTitle"));text->addWidget(label(QStringLiteral("%1 公里 · ¥%2/度 · %3/%4 空闲").arg(s.distanceKm,0,'f',1).arg(s.pricePerKwh,0,'f',2).arg(s.availableChargers).arg(s.totalChargers),"Muted"));l->addLayout(text,1);auto* open=button(QStringLiteral("查看"),"Secondary");open->setFixedSize(66,42);l->addWidget(open);connect(open,&QPushButton::clicked,this,[this,id=s.stationId]{requestStationDetail(id);});stationListLayout_->addWidget(row);}stationListLayout_->addStretch();
}

void MobileApp::requestStationDetail(qint64 stationId)
{
    showPage(StationPage);setNotice(detailNotice_,QStringLiteral("正在同步充电桩…"));clearLayout(chargerListLayout_);QJsonObject data;data.insert(QStringLiteral("stationId"),double(stationId));network_->sendRequest(QString::fromLatin1(protocol::action::kStationDetail),data,[this](const protocol::Response&r){if(!r.isOk()){setNotice(detailNotice_,protocol::describeError(r.code,r.message),true);return;}StationDetail d;d.station=StationInfo::fromJson(r.data);d.address=r.data.value(QStringLiteral("address")).toString();d.latitude=r.data.value(QStringLiteral("latitude")).toDouble();d.longitude=r.data.value(QStringLiteral("longitude")).toDouble();for(const auto&v:r.data.value(QStringLiteral("chargers")).toArray()){const auto c=ChargerInfo::fromJson(v.toObject());if(c.valid())d.chargers.append(c);}renderStationDetail(d);});
}

void MobileApp::renderStationDetail(const StationDetail& d)
{
    currentStation_=d;detailName_->setText(d.station.name);detailMeta_->setText(d.station.availableChargers>0?QStringLiteral("可充电"):QStringLiteral("暂无空闲"));detailMeta_->setObjectName(d.station.availableChargers>0?QStringLiteral("StatusOk"):QStringLiteral("StatusWarn"));detailMeta_->style()->unpolish(detailMeta_);detailMeta_->style()->polish(detailMeta_);detailAddress_->setText(d.address);detailPrice_->setText(QStringLiteral("¥ %1/度").arg(d.station.pricePerKwh,0,'f',2));detailAvailability_->setText(QStringLiteral("%1").arg(d.station.availableChargers));setNotice(detailNotice_,d.chargers.isEmpty()?QStringLiteral("暂无充电桩"):QString());clearLayout(chargerListLayout_);
    for(const auto&c:d.chargers){auto* row=surface();auto*l=new QHBoxLayout(row);l->setContentsMargins(16,15,12,15);auto* text=new QVBoxLayout;text->setSpacing(4);text->addWidget(label(c.code.isEmpty()?QStringLiteral("充电桩 %1").arg(c.chargerId):c.code,"SectionTitle"));text->addWidget(label(QStringLiteral("%1 · %2 kW · %3").arg(c.typeLabel()).arg(c.powerKw,0,'f',0).arg(c.statusLabel()),"Muted"));l->addLayout(text,1);auto* choose=button(QStringLiteral("查看"),"Secondary");choose->setFixedSize(72,46);l->addWidget(choose);connect(choose,&QPushButton::clicked,this,[this,c]{showChargerDetail(c);});chargerListLayout_->addWidget(row);}chargerListLayout_->addStretch();
}

void MobileApp::showChargerDetail(const ChargerInfo& charger)
{
    currentCharger_ = charger;
    chargerCode_->setText(charger.code.isEmpty()
        ? QStringLiteral("充电桩 %1").arg(charger.chargerId) : charger.code);
    chargerStatus_->setText(charger.statusLabel());
    const QString statusStyle = charger.isIdle() ? QStringLiteral("StatusOk")
        : charger.status == protocol::ChargerStatusFault
            ? QStringLiteral("StatusBad") : QStringLiteral("StatusWarn");
    chargerStatus_->setObjectName(statusStyle);
    chargerStatus_->style()->unpolish(chargerStatus_);
    chargerStatus_->style()->polish(chargerStatus_);
    chargerPower_->setText(QStringLiteral("%1 kW").arg(charger.powerKw,0,'f',0));
    chargerMeta_->setText(QStringLiteral("%1 · %2\n%3")
        .arg(charger.typeLabel(), currentStation_.station.name, currentStation_.address));
    reserveButton_->setEnabled(charger.isIdle());
    reserveButton_->setText(charger.isIdle()
        ? QStringLiteral("预约充电桩") : QStringLiteral("当前不可预约"));
    setNotice(chargerNotice_, QString());
    showPage(ChargerPage);
}

void MobileApp::showNavigation()
{
    if (!currentStation_.station.valid()) return;
    navigationTarget_->setText(currentCharger_.valid()
        ? QStringLiteral("%1 · %2").arg(currentStation_.station.name, currentCharger_.code)
        : currentStation_.station.name);
    navigationDistance_->setText(QStringLiteral("— km"));
    navigationDuration_->setText(QStringLiteral("— 分钟"));
    navigationAction_->setText(QStringLiteral("开始导航"));
    navigationAction_->setObjectName(QStringLiteral("Primary"));
    navigationAction_->style()->unpolish(navigationAction_);
    navigationAction_->style()->polish(navigationAction_);
    navigationMap_->setCenter(currentStation_.latitude, currentStation_.longitude,
                              currentStation_.station.name);
    showPage(NavigationPage);
    if (!Session::instance().hasLocation()) {
        setNotice(navigationNotice_, QStringLiteral("请先在首页设置起点"), true);
        return;
    }
    setNotice(navigationNotice_, QStringLiteral("正在规划路线…"));
    routePlanner_->plan(Session::instance().latitude(), Session::instance().longitude(),
                        currentStation_.latitude, currentStation_.longitude, false);
}

void MobileApp::reserveCharger(const ChargerInfo& c)
{
    setBusy(reserveButton_, true, QStringLiteral("预约充电桩"));
    setNotice(chargerNotice_,QStringLiteral("正在预约…"));QJsonObject data;data.insert(QStringLiteral("userId"),double(Session::instance().userId()));data.insert(QStringLiteral("chargerId"),double(c.chargerId));network_->sendRequest(QString::fromLatin1(protocol::action::kOrderReserve),data,[this](const protocol::Response&r){setBusy(reserveButton_,false,QStringLiteral("预约充电桩"));if(!r.isOk()){setNotice(chargerNotice_,protocol::describeError(r.code,r.message),true);return;}activeOrder_=orderFromPayload(r.data);renderOrder(activeOrder_);showPage(ChargingPage);});
}

void MobileApp::requestActiveOrder(Page destination)
{
    showPage(destination);
    QLabel* notice = destination == OrdersPage ? orderNotice_ : chargingNotice_;
    setNotice(notice, QStringLiteral("正在同步服务端状态…"));
    QJsonObject data;
    data.insert(QStringLiteral("userId"), double(Session::instance().userId()));
    network_->sendRequest(QString::fromLatin1(protocol::action::kOrderActive), data,
        [this, destination, notice](const protocol::Response& r) {
            if (!r.isOk() && r.code != protocol::CodeOrderConflict) {
                setNotice(notice, protocol::describeError(r.code, r.message), true);
                return;
            }
            activeOrder_ = r.isOk() ? orderFromPayload(r.data) : OrderInfo{};
            renderOrder(activeOrder_);
            setNotice(notice, activeOrder_.valid()
                ? QStringLiteral("状态已同步") : QStringLiteral("暂无进行中的订单"));
            if (destination == OrdersPage) {
                orderHeadline_->setText(statusName(activeOrder_.statusEnum()));
                orderSummary_->setText(activeOrder_.valid()
                    ? QStringLiteral("%1\n订单号 %2 · %3\n已充 %4 kWh · 当前金额 ¥%5")
                          .arg(activeOrder_.stationName).arg(activeOrder_.orderId)
                          .arg(activeOrder_.chargerCode).arg(activeOrder_.energyKwh,0,'f',2)
                          .arg(activeOrder_.estimatedAmount,0,'f',2)
                    : QStringLiteral("完成预约或开始充电后，这里会展示服务端返回的最新状态。"));
            }
        });
}

void MobileApp::renderOrder(const OrderInfo& o)
{
    const auto state=o.statusEnum();chargingStatus_->setText(statusName(state));QString obj=state==OrderInfo::StatusCharging?QStringLiteral("StatusOk"):(state==OrderInfo::StatusReserved||state==OrderInfo::StatusWaitSettlement?QStringLiteral("StatusWarn"):QStringLiteral("StatusInfo"));chargingStatus_->setObjectName(obj);chargingStatus_->style()->unpolish(chargingStatus_);chargingStatus_->style()->polish(chargingStatus_);
    if(!o.valid()){gauge_->setValue(0,QStringLiteral("—"),QStringLiteral("等待订单"));chargingStation_->setText(QStringLiteral("暂无订单"));chargingMetrics_->setText(QStringLiteral("请选择空闲充电桩"));chargingAmount_->setText(QStringLiteral("¥ —"));chargingAction_->setText(QStringLiteral("查找充电站"));chargingAction_->setObjectName(QStringLiteral("Primary"));chargingAction_->style()->unpolish(chargingAction_);chargingAction_->style()->polish(chargingAction_);return;}
    const double pct=o.progressPercent>=0?o.progressPercent:(o.targetEnergyKwh>0?o.energyKwh/o.targetEnergyKwh*100.0:0.0);gauge_->setValue(pct,state==OrderInfo::StatusCharging?QStringLiteral("%1%").arg(qRound(pct)):statusName(state),state==OrderInfo::StatusCharging?QStringLiteral("实时充电进度"):QStringLiteral("服务端订单状态"));chargingStation_->setText(o.stationName);chargingMetrics_->setText(QStringLiteral("%1 · %2 kW\n%3 · 已充 %4 kWh").arg(o.chargerCode).arg(o.powerKw,0,'f',0).arg(durationText(o.durationMs())).arg(o.energyKwh,0,'f',2));chargingAmount_->setText(QStringLiteral("¥ %1").arg(state==OrderInfo::StatusWaitSettlement?o.amount:o.estimatedAmount,0,'f',2));
    if(state==OrderInfo::StatusReserved)chargingAction_->setText(QStringLiteral("开始充电"));else if(state==OrderInfo::StatusCharging)chargingAction_->setText(QStringLiteral("结束充电"));else if(state==OrderInfo::StatusWaitSettlement)chargingAction_->setText(QStringLiteral("确认结算"));else chargingAction_->setText(QStringLiteral("查看附近站点"));chargingAction_->setObjectName(state==OrderInfo::StatusCharging?QStringLiteral("Danger"):QStringLiteral("Primary"));chargingAction_->style()->unpolish(chargingAction_);chargingAction_->style()->polish(chargingAction_);
}

void MobileApp::performOrderAction(const QString& action,QPushButton* source)
{
    const QString normal = source->text();
    setBusy(source, true, normal);
    QJsonObject data;
    data.insert(QStringLiteral("userId"), double(Session::instance().userId()));
    data.insert(QStringLiteral("orderId"), double(activeOrder_.orderId));
    network_->sendRequest(action, data, [this, source, normal](const protocol::Response& r) {
        setBusy(source, false, normal);
        if (!r.isOk()) {
            setNotice(chargingNotice_, protocol::describeError(r.code, r.message), true);
            return;
        }
        activeOrder_ = orderFromPayload(r.data);
        if (!activeOrder_.valid()) requestActiveOrder(ChargingPage);
        else renderOrder(activeOrder_);
        if (r.data.contains(QStringLiteral("balance"))) {
            Session::instance().setBalance(r.data.value(QStringLiteral("balance")).toDouble());
        }
        setNotice(chargingNotice_, QStringLiteral("操作已由服务端确认"));
    });
}

void MobileApp::updateProfile()
{
    const QString nick=nicknameInput_->text().trimmed();if(nick.size()<2||nick.size()>20){setNotice(profileNotice_,QStringLiteral("昵称需为 2–20 个字符"),true);return;}setBusy(nicknameButton_,true,QStringLiteral("保存"));QJsonObject d;d.insert(QStringLiteral("userId"),double(Session::instance().userId()));d.insert(QStringLiteral("nickname"),nick);network_->sendRequest(QString::fromLatin1(protocol::action::kUserProfileUpdate),d,[this,nick](const protocol::Response&r){setBusy(nicknameButton_,false,QStringLiteral("保存"));if(!r.isOk()){setNotice(profileNotice_,protocol::describeError(r.code,r.message),true);return;}Session::instance().setNickname(r.data.value(QStringLiteral("nickname")).toString(nick));nicknameInput_->clear();refreshSessionViews();setNotice(profileNotice_,QStringLiteral("昵称已保存"));});
}

void MobileApp::recharge()
{
    bool ok=false;const double amount=rechargeInput_->text().toDouble(&ok);if(!ok||amount<=0||amount>100000){setNotice(profileNotice_,QStringLiteral("请输入 0.01–100000 元的金额"),true);return;}setBusy(rechargeButton_,true,QStringLiteral("充值"));QJsonObject d;d.insert(QStringLiteral("userId"),double(Session::instance().userId()));d.insert(QStringLiteral("amount"),amount);network_->sendRequest(QString::fromLatin1(protocol::action::kUserRecharge),d,[this](const protocol::Response&r){setBusy(rechargeButton_,false,QStringLiteral("充值"));if(!r.isOk()){setNotice(profileNotice_,protocol::describeError(r.code,r.message),true);return;}Session::instance().setBalance(r.data.value(QStringLiteral("balance")).toDouble());rechargeInput_->clear();refreshSessionViews();setNotice(profileNotice_,QStringLiteral("充值已到账"));});
}

void MobileApp::refreshSessionViews(){const auto&s=Session::instance();if(locationLabel_)locationLabel_->setText(s.hasLocation()?s.locationLabel():QStringLiteral("尚未设置位置"));if(profileName_)profileName_->setText(s.nickname().isEmpty()?QStringLiteral("车主用户"):s.nickname());if(profilePhone_)profilePhone_->setText(s.phone());if(profileBalance_)profileBalance_->setText(QStringLiteral("¥ %1").arg(s.balance(),0,'f',2));}

void MobileApp::updateConnectionState(bool connected){if(!connectionLabel_)return;connectionLabel_->setText(connected?QStringLiteral("服务已连接"):QStringLiteral("服务未连接"));connectionLabel_->setObjectName(connected?QStringLiteral("StatusOk"):QStringLiteral("StatusWarn"));connectionLabel_->style()->unpolish(connectionLabel_);connectionLabel_->style()->polish(connectionLabel_);}
