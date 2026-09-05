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
#include <QCheckBox>
#include <QDateTime>
#include <QDoubleValidator>
#include <QFile>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
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
#include <algorithm>

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

enum class HomeIcon { Refresh, Bolt, SlowBolt, Star, Target, Search, Locate, Navigation, Chevron, Pin };

QIcon homeIcon(HomeIcon kind, const QColor& color)
{
    QPixmap pixmap(40, 40);
    pixmap.fill(Qt::transparent);
    pixmap.setDevicePixelRatio(2.0);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);

    if (kind == HomeIcon::Refresh) {
        painter.drawArc(QRectF(3.5, 3.5, 13, 13), 36 * 16, 245 * 16);
        painter.drawLine(QPointF(15.8, 3.4), QPointF(16.5, 7.4));
        painter.drawLine(QPointF(15.8, 3.4), QPointF(12.0, 4.5));
        painter.drawArc(QRectF(3.5, 3.5, 13, 13), 216 * 16, 82 * 16);
        painter.drawLine(QPointF(4.2, 16.7), QPointF(3.5, 12.8));
        painter.drawLine(QPointF(4.2, 16.7), QPointF(8.0, 15.4));
    } else if (kind == HomeIcon::Bolt || kind == HomeIcon::SlowBolt) {
        QPainterPath path;
        path.moveTo(11.2, 1.8); path.lineTo(4.4, 11.0); path.lineTo(9.2, 11.0);
        path.lineTo(7.5, 18.2); path.lineTo(15.7, 8.0); path.lineTo(10.7, 8.0);
        path.closeSubpath();
        painter.drawPath(path);
        if (kind == HomeIcon::SlowBolt) {
            painter.setPen(QPen(color, 1.4, Qt::SolidLine, Qt::RoundCap));
            painter.drawLine(QPointF(2.2, 5.0), QPointF(4.7, 5.0));
            painter.drawLine(QPointF(1.2, 8.2), QPointF(3.6, 8.2));
        }
    } else if (kind == HomeIcon::Star) {
        QPainterPath star;
        for (int index = 0; index < 10; ++index) {
            const double radius = index % 2 == 0 ? 8.0 : 3.5;
            const double angle = qDegreesToRadians(-90.0 + index * 36.0);
            const QPointF point(10.0 + qCos(angle) * radius, 10.0 + qSin(angle) * radius);
            index == 0 ? star.moveTo(point) : star.lineTo(point);
        }
        star.closeSubpath();
        painter.drawPath(star);
    } else if (kind == HomeIcon::Target || kind == HomeIcon::Locate) {
        painter.drawEllipse(QPointF(10, 10), kind == HomeIcon::Locate ? 6.0 : 5.5,
                            kind == HomeIcon::Locate ? 6.0 : 5.5);
        painter.drawEllipse(QPointF(10, 10), 1.8, 1.8);
        painter.drawLine(QPointF(10, 0.8), QPointF(10, 3.3));
        painter.drawLine(QPointF(10, 16.7), QPointF(10, 19.2));
        painter.drawLine(QPointF(0.8, 10), QPointF(3.3, 10));
        painter.drawLine(QPointF(16.7, 10), QPointF(19.2, 10));
        if (kind == HomeIcon::Target) painter.drawRoundedRect(QRectF(3.2, 3.2, 13.6, 13.6), 6.8, 6.8);
    } else if (kind == HomeIcon::Search) {
        painter.drawEllipse(QRectF(2.4, 2.4, 11.2, 11.2));
        painter.drawLine(QPointF(12.3, 12.3), QPointF(18.0, 18.0));
    } else if (kind == HomeIcon::Navigation) {
        painter.setBrush(color);
        QPainterPath arrow;
        arrow.moveTo(10.0, 1.5); arrow.lineTo(17.0, 17.5); arrow.lineTo(10.0, 14.5);
        arrow.lineTo(3.0, 17.5); arrow.closeSubpath();
        painter.drawPath(arrow);
    } else if (kind == HomeIcon::Chevron) {
        painter.drawLine(QPointF(7.5, 4.5), QPointF(13.0, 10.0));
        painter.drawLine(QPointF(13.0, 10.0), QPointF(7.5, 15.5));
    } else if (kind == HomeIcon::Pin) {
        QPainterPath pin;
        pin.moveTo(10, 18.2);
        pin.cubicTo(8.0, 15.2, 4.0, 12.2, 4.0, 7.8);
        pin.cubicTo(4.0, 4.4, 6.7, 1.8, 10.0, 1.8);
        pin.cubicTo(13.3, 1.8, 16.0, 4.4, 16.0, 7.8);
        pin.cubicTo(16.0, 12.2, 12.0, 15.2, 10, 18.2);
        pin.closeSubpath();
        painter.drawPath(pin);
        painter.drawEllipse(QPointF(10, 7.8), 1.8, 1.8);
    }
    return QIcon(pixmap);
}

void configureIconButton(QPushButton* target, HomeIcon icon, const QColor& color,
                         const QSize& iconSize = QSize(18, 18))
{
    target->setIcon(homeIcon(icon, color));
    target->setIconSize(iconSize);
}

class StationCardWidget final : public QWidget
{
public:
    explicit StationCardWidget(bool recommended, QWidget* parent = nullptr)
        : QWidget(parent), recommended_(recommended)
    {
        setAttribute(Qt::WA_StyledBackground, false);
        setMinimumHeight(112);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }
protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const QRectF card = QRectF(rect()).adjusted(0.75, 0.75, -0.75, -0.75);
        painter.setPen(QPen(QColor(recommended_ ? "#AFC9FF" : "#E4EAF0"), recommended_ ? 1.5 : 1.0));
        painter.setBrush(Qt::white);
        painter.drawRoundedRect(card, 15, 15);
        if (!recommended_) return;

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(Theme::Blue));
        QPainterPath ribbon;
        ribbon.moveTo(1, 1); ribbon.lineTo(36, 1); ribbon.lineTo(1, 34); ribbon.closeSubpath();
        painter.drawPath(ribbon);
        painter.save();
        painter.translate(6.5, 21.5);
        painter.rotate(-43.0);
        painter.setPen(Qt::white);
        QFont ribbonFont = font();
        ribbonFont.setPixelSize(8);
        ribbonFont.setWeight(QFont::DemiBold);
        painter.setFont(ribbonFont);
        painter.drawText(QRectF(-2, -7, 28, 12), Qt::AlignCenter, QStringLiteral("推荐"));
        painter.restore();
    }
private:
    bool recommended_ = false;
};

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

QString clockDurationText(qint64 durationMs)
{
    const qint64 totalSeconds = qMax<qint64>(0, durationMs / 1000);
    return QStringLiteral("%1:%2:%3")
        .arg(totalSeconds / 3600, 2, 10, QLatin1Char('0'))
        .arg((totalSeconds / 60) % 60, 2, 10, QLatin1Char('0'))
        .arg(totalSeconds % 60, 2, 10, QLatin1Char('0'));
}

OrderInfo orderFromPayload(const QJsonObject& payload)
{
    const QJsonObject nested = payload.value(QStringLiteral("order")).toObject();
    return OrderInfo::fromJson(nested.isEmpty() ? payload : nested);
}

QString routeDistanceText(double meters)
{
    if (meters >= 1000.0) {
        return QStringLiteral("%1 公里").arg(meters / 1000.0, 0, 'f', 1);
    }
    return QStringLiteral("%1 米").arg(qMax(1, qRound(meters)));
}

QString maneuverForStep(const RouteStep& step)
{
    const QString text = step.action + QLatin1Char(' ') + step.assistantAction
        + QLatin1Char(' ') + step.instruction;
    if (text.contains(QStringLiteral("掉头"))) return QStringLiteral("uturn");
    if (text.contains(QStringLiteral("环岛"))) return QStringLiteral("roundabout");
    if (text.contains(QStringLiteral("左转")) || text.contains(QStringLiteral("向左"))) {
        return QStringLiteral("left");
    }
    if (text.contains(QStringLiteral("右转")) || text.contains(QStringLiteral("向右"))) {
        return QStringLiteral("right");
    }
    if (text.contains(QStringLiteral("到达")) || text.contains(QStringLiteral("终点"))) {
        return QStringLiteral("arrive");
    }
    return QStringLiteral("straight");
}

QString actionTextForStep(const RouteStep& step)
{
    if (!step.action.trimmed().isEmpty()) return step.action.trimmed();
    const QString maneuver = maneuverForStep(step);
    if (maneuver == QStringLiteral("left")) return QStringLiteral("左转");
    if (maneuver == QStringLiteral("right")) return QStringLiteral("右转");
    if (maneuver == QStringLiteral("uturn")) return QStringLiteral("掉头");
    if (maneuver == QStringLiteral("roundabout")) return QStringLiteral("进入环岛");
    if (maneuver == QStringLiteral("arrive")) return QStringLiteral("到达终点");
    return QStringLiteral("直行");
}

RouteResult previewNavigationRoute()
{
    const QList<QPointF> points{
        QPointF(116.171271, 39.735678), QPointF(116.171640, 39.735220),
        QPointF(116.172040, 39.734760), QPointF(116.172490, 39.734340),
        QPointF(116.172380, 39.733860), QPointF(116.172920, 39.733430),
        QPointF(116.173500, 39.732800)
    };
    struct PreviewStep { int from; int to; const char* instruction; const char* road; const char* action; double distance; qint64 duration; };
    const PreviewStep definitions[]{
        {0, 2, "沿良乡大学城西路向东南行驶 300 米", "良乡大学城西路", "直行", 300.0, 58},
        {2, 4, "右转进入良乡南大街，继续行驶 420 米", "良乡南大街", "右转", 420.0, 82},
        {4, 5, "左转进入学园北街，继续行驶 320 米", "学园北街", "左转", 320.0, 63},
        {5, 6, "前方到达北理良乡南门充电站", "北理良乡南门充电站", "到达终点", 160.0, 37}
    };
    RouteResult result;
    result.distanceMeters = 1200.0;
    result.durationSeconds = 240;
    result.path = points;
    for (const PreviewStep& definition : definitions) {
        RouteStep step;
        step.instruction = QString::fromUtf8(definition.instruction);
        step.roadName = QString::fromUtf8(definition.road);
        step.action = QString::fromUtf8(definition.action);
        step.distanceMeters = definition.distance;
        step.durationSeconds = definition.duration;
        for (int index = definition.from; index <= definition.to; ++index) {
            step.path.append(points.at(index));
        }
        result.steps.append(step);
    }
    result.firstInstruction = result.steps.first().instruction;
    result.firstRoadName = result.steps.first().roadName;
    result.firstStepDistanceMeters = result.steps.first().distanceMeters;
    return result;
}

class BackGlyphButton final : public QAbstractButton
{
public:
    explicit BackGlyphButton(QWidget* parent = nullptr) : QAbstractButton(parent)
    {
        setFixedSize(44, 44);
        setCursor(Qt::PointingHandCursor);
        setToolTip(QStringLiteral("返回"));
        setAccessibleName(QStringLiteral("返回"));
    }
protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        if (underMouse() || isDown()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(isDown() ? "#DDE9FF" : Theme::BlueSoft));
            painter.drawRoundedRect(rect().adjusted(3,3,-3,-3), 12, 12);
        }
        painter.setPen(QPen(QColor(Theme::Ink), 2.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        QPainterPath arrow;
        arrow.moveTo(26, 12);
        arrow.lineTo(16, 22);
        arrow.lineTo(26, 32);
        painter.drawPath(arrow);
    }
};

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
    setMinimumHeight(180);
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

void EnergyMapWidget::setRoute(const RouteResult& route)
{
    routePath_ = route.path;
    update();
}

void EnergyMapWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#EAF0F6"));
    p.drawRoundedRect(rect(), 20, 20);

    p.setBrush(QColor("#DFF2E8"));
    p.drawRoundedRect(QRectF(width() * 0.03, 10, width() * 0.20, height() * 0.28), 14, 14);
    p.drawRoundedRect(QRectF(width() * 0.70, height() * 0.45, width() * 0.17, height() * 0.24), 14, 14);
    p.setBrush(QColor("#E3ECF8"));
    p.drawRoundedRect(QRectF(width() * 0.34, 8, width() * 0.21, height() * 0.20), 12, 12);
    p.drawRoundedRect(QRectF(width() * 0.48, height() * 0.48, width() * 0.16, height() * 0.22), 12, 12);

    QPen street(QColor("#FFFFFF"), 11, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(street);
    QPainterPath a; a.moveTo(-20, height()*0.71); a.cubicTo(width()*0.22,height()*0.48,width()*0.47,height()*0.76,width()+20,height()*0.36); p.drawPath(a);
    QPainterPath b; b.moveTo(width()*0.15,-10); b.cubicTo(width()*0.29,height()*0.25,width()*0.20,height()*0.56,width()*0.43,height()+20); p.drawPath(b);
    QPainterPath c; c.moveTo(width()*0.75,-10); c.cubicTo(width()*0.71,height()*0.26,width()*0.68,height()*0.56,width()*0.61,height()+15); p.drawPath(c);
    QPainterPath d; d.moveTo(-10, height()*0.28); d.cubicTo(width()*0.32,height()*0.39,width()*0.63,height()*0.14,width()+10,height()*0.25); p.drawPath(d);
    QPen localStreet(QColor("#F8FAFC"), 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(localStreet);
    for (int index = 1; index < 6; ++index) {
        p.drawLine(QPointF(width() * index / 6.0, 2),
                   QPointF(width() * (index - 0.35) / 6.0, height() - 2));
    }

    QPen routeOutline(QColor("#D9E7FF"), 9, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    QPen route(QColor(Theme::Blue), 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(route);
    QPainterPath line;
    QPointF destinationLabelAnchor;
    bool hasRenderedRoute = false;
    if (routePath_.size() > 1) {
        const QPointF geoOrigin = routePath_.first();
        const double longitudeScale = qCos(qDegreesToRadians(geoOrigin.y()));
        QVector<QPointF> normalized;
        normalized.reserve(routePath_.size());
        for (const QPointF& point : routePath_) {
            normalized.append(QPointF((point.x() - geoOrigin.x()) * longitudeScale,
                                      -(point.y() - geoOrigin.y())));
        }
        const QPointF routeVector = normalized.last() - normalized.first();
        const double currentAngle = qAtan2(routeVector.y(), routeVector.x());
        const QPointF screenOrigin(width() * 0.68, height() - 34.0);
        const QPointF screenDestination(width() * 0.84, 52.0);
        const QPointF targetVector = screenDestination - screenOrigin;
        const double targetAngle = qAtan2(targetVector.y(), targetVector.x());
        const double rotation = targetAngle - currentAngle;
        const double cosine = qCos(rotation);
        const double sine = qSin(rotation);
        QVector<QPointF> rotated;
        rotated.reserve(normalized.size());
        for (const QPointF& point : normalized) {
            const QPointF value(point.x() * cosine - point.y() * sine,
                                point.x() * sine + point.y() * cosine);
            rotated.append(value);
        }
        const double routeLength = qMax(0.000001, qSqrt(routeVector.x() * routeVector.x()
            + routeVector.y() * routeVector.y()));
        const double targetLength = qSqrt(targetVector.x() * targetVector.x()
            + targetVector.y() * targetVector.y());
        const double scale = targetLength / routeLength;
        QVector<QPointF> screenPoints;
        screenPoints.reserve(rotated.size());
        for (const QPointF& point : rotated) screenPoints.append(screenOrigin + point * scale);

        line.moveTo(screenPoints.first());
        for (int index = 1; index < screenPoints.size(); ++index) line.lineTo(screenPoints.at(index));
        p.setPen(routeOutline); p.drawPath(line);
        p.setPen(route); p.drawPath(line);

        p.setPen(QPen(Qt::white, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        const int arrowStride = qMax(1, static_cast<int>(screenPoints.size()) / 4);
        for (int index = arrowStride; index < screenPoints.size(); index += arrowStride) {
            const QPointF tip = screenPoints.at(index);
            const QPointF previous = screenPoints.at(qMax(0, index - 1));
            const QPointF delta = tip - previous;
            const double length = qMax(0.001, qSqrt(delta.x() * delta.x() + delta.y() * delta.y()));
            const QPointF direction(delta.x() / length, delta.y() / length);
            const QPointF normal(-direction.y(), direction.x());
            const QPointF back = tip - direction * 6.0;
            p.drawLine(tip, back + normal * 3.2);
            p.drawLine(tip, back - normal * 3.2);
        }

        const QPointF origin = screenPoints.first();
        p.setPen(QPen(Qt::white, 3));
        p.setBrush(QColor(Theme::Blue));
        p.drawEllipse(origin, 17, 17);
        QPainterPath pointer;
        pointer.moveTo(origin + QPointF(1, -10));
        pointer.lineTo(origin + QPointF(8, 9));
        pointer.lineTo(origin + QPointF(0, 5));
        pointer.lineTo(origin + QPointF(-8, 9));
        pointer.closeSubpath();
        p.fillPath(pointer, Qt::white);

        const QPointF destination = screenPoints.last();
        p.setPen(QPen(Qt::white, 3));
        p.setBrush(QColor(Theme::Green));
        p.drawEllipse(destination, 15, 15);
        QPainterPath bolt;
        bolt.moveTo(destination + QPointF(2, -10));
        bolt.lineTo(destination + QPointF(-6, 1));
        bolt.lineTo(destination + QPointF(-1, 1));
        bolt.lineTo(destination + QPointF(-4, 10));
        bolt.lineTo(destination + QPointF(7, -3));
        bolt.lineTo(destination + QPointF(2, -3));
        bolt.closeSubpath();
        p.fillPath(bolt, Qt::white);
        destinationLabelAnchor = destination;
        hasRenderedRoute = true;
    } else if (!stations_.isEmpty()) {
        const QPointF origin(width() * 0.14, height() * 0.62);
        const QPointF destination(width() * 0.70, height() * 0.29);
        line.moveTo(origin);
        line.cubicTo(width() * 0.15, height() * 0.80,
                     width() * 0.48, height() * 0.64,
                     width() * 0.56, height() * 0.47);
        line.cubicTo(width() * 0.65, height() * 0.36,
                     width() * 0.70, height() * 0.45,
                     destination.x(), destination.y());
        p.setPen(routeOutline); p.drawPath(line);
        p.setPen(route); p.drawPath(line);

        p.setPen(QPen(Qt::white, 3));
        p.setBrush(QColor(Theme::Blue));
        p.drawEllipse(origin, 13, 13);
        QPainterPath pointer;
        pointer.moveTo(origin + QPointF(1, -8));
        pointer.lineTo(origin + QPointF(7, 7));
        pointer.lineTo(origin + QPointF(0, 4));
        pointer.lineTo(origin + QPointF(-7, 7));
        pointer.closeSubpath();
        p.fillPath(pointer, Qt::white);

        p.setBrush(QColor(Theme::Blue));
        p.drawEllipse(destination, 15, 15);
        p.setPen(Qt::white);
        QFont markerFont = font(); markerFont.setPixelSize(13); markerFont.setWeight(QFont::Bold);
        p.setFont(markerFont);
        p.drawText(QRectF(destination.x() - 15, destination.y() - 15, 30, 30),
                   Qt::AlignCenter, QStringLiteral("1"));
        destinationLabelAnchor = destination;
        hasRenderedRoute = true;
    }

    const int extraCount = qMin(2, qMax(0, stations_.size() - 1));
    const QPointF extraPoints[2] = {{width()*0.41,height()*0.43},{width()*0.84,height()*0.56}};
    for (int i = 0; i < extraCount; ++i) {
        p.setPen(QPen(Qt::white, 3));
        p.setBrush(QColor(stations_[i + 1].availableChargers > 0 ? Theme::Green : Theme::Amber));
        p.drawEllipse(extraPoints[i], 7, 7);
    }

    QFont f = font(); f.setPixelSize(12); p.setFont(f);
    if (hasRenderedRoute) {
        const bool homeRoute = routePath_.isEmpty() && !stations_.isEmpty();
        const QString routeLabel = homeRoute ? stations_.first().name : caption_;
        const qreal labelWidth = qMin<qreal>(170.0, width() * 0.43);
        const qreal labelLeft = qMin<qreal>(width() - labelWidth - 10.0,
                                            destinationLabelAnchor.x() + 20.0);
        const QRectF labelRect(qMax<qreal>(10.0, labelLeft),
                               qMax<qreal>(8.0, destinationLabelAnchor.y() - 24.0),
                               labelWidth, homeRoute ? 44.0 : 42.0);
        p.setPen(Qt::NoPen);
        p.setBrush(Qt::white);
        p.drawRoundedRect(labelRect, 9, 9);
        p.setPen(QColor(Theme::Ink));
        f.setPixelSize(11); f.setWeight(QFont::DemiBold); p.setFont(f);
        p.drawText(labelRect.adjusted(8, 3, -8, homeRoute ? -16 : -3),
                   Qt::AlignVCenter | Qt::AlignLeft, routeLabel);
        if (homeRoute) {
            const QString available = stations_.first().availableChargers > 0
                ? QStringLiteral("%1桩空闲").arg(stations_.first().availableChargers)
                : QStringLiteral("暂无空闲");
            const QRectF badge(labelRect.left() + 8, labelRect.bottom() - 18, 58, 15);
            p.setBrush(QColor(stations_.first().availableChargers > 0 ? Theme::Green : Theme::Amber));
            p.drawRoundedRect(badge, 5, 5);
            p.setPen(Qt::white);
            f.setPixelSize(8); f.setWeight(QFont::DemiBold); p.setFont(f);
            p.drawText(badge, Qt::AlignCenter, available);
        }
    } else if (stations_.isEmpty()) {
        p.setPen(QColor(Theme::Muted));
        p.drawText(rect().adjusted(24, 24, -24, -24), Qt::AlignCenter, caption_);
    }
}

AmapWidget::AmapWidget(QWidget* parent) : QWidget(parent)
{
    setMinimumHeight(180);
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

void AmapWidget::setRoute(const RouteResult& route, const QString& destinationLabel)
{
    if (fallback_) fallback_->setRoute(route);
    QJsonArray points;
    for (const QPointF& point : route.path) {
        QJsonObject value;
        value.insert(QStringLiteral("lng"), point.x());
        value.insert(QStringLiteral("lat"), point.y());
        points.append(value);
    }
    QJsonObject payload;
    payload.insert(QStringLiteral("points"), points);
    payload.insert(QStringLiteral("destination"), destinationLabel);
    runScript(QStringLiteral("window.setRoute(%1)")
        .arg(QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact))));
}

ChargeGauge::ChargeGauge(QWidget* parent) : QWidget(parent)
{
    setMinimumSize(210, 210);
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
    const qreal side = qMin(width(), height()) - 30;
    const QRectF ring((width()-side)/2, (height()-side)/2, side, side);
    QPen base(QColor("#DFE8F2"), 14, Qt::SolidLine, Qt::RoundCap); p.setPen(base); p.drawEllipse(ring);
    QConicalGradient gradient(ring.center(), 90);
    gradient.setColorAt(0.0, QColor(Theme::Blue));
    gradient.setColorAt(0.72, QColor("#2A8BFF"));
    gradient.setColorAt(0.88, QColor("#10B96B"));
    gradient.setColorAt(1.0, QColor(Theme::Blue));
    QPen live(QBrush(gradient), 14, Qt::SolidLine, Qt::RoundCap);
    p.setPen(live);
    p.drawArc(ring, 90*16, int(-360*16*percent_/100.0));
    p.setPen(QColor(Theme::Ink));
    QFont f = font(); f.setPixelSize(38); f.setWeight(QFont::Bold); p.setFont(f);
    p.drawText(rect().adjusted(0,48,0,-58), Qt::AlignCenter, centerText_);
    f.setPixelSize(12); f.setWeight(QFont::DemiBold); p.setFont(f); p.setPen(QColor(Theme::Blue));
    p.drawText(rect().adjusted(0,94,0,-28), Qt::AlignCenter, caption_);
}

SparklineWidget::SparklineWidget(const QColor& color, QWidget* parent)
    : QWidget(parent), color_(color)
{
    setFixedHeight(34);
}

void SparklineWidget::setSamples(const QList<double>& samples)
{
    samples_.clear();
    for (double value : samples) samples_.append(value);
    update();
}

void SparklineWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QRectF area = rect().adjusted(2,3,-2,-3);
    if (samples_.size() < 2) {
        QPen empty(QColor("#C9D6E7"), 1.5, Qt::DashLine);
        p.setPen(empty);
        p.drawLine(area.left(), area.center().y(), area.right(), area.center().y());
        return;
    }
    const auto [minIt,maxIt] = std::minmax_element(samples_.cbegin(),samples_.cend());
    const double minValue=*minIt, maxValue=*maxIt;
    const double range=qMax(0.001,maxValue-minValue);
    QPainterPath line;
    for(int i=0;i<samples_.size();++i){const qreal x=area.left()+area.width()*i/(samples_.size()-1);const qreal y=area.bottom()-area.height()*(samples_[i]-minValue)/range;if(i==0)line.moveTo(x,y);else line.lineTo(x,y);}
    QPainterPath fill=line;fill.lineTo(area.right(),area.bottom());fill.lineTo(area.left(),area.bottom());fill.closeSubpath();
    QLinearGradient shade(0,area.top(),0,area.bottom());QColor top=color_;top.setAlpha(70);QColor bottom=color_;bottom.setAlpha(0);shade.setColorAt(0,top);shade.setColorAt(1,bottom);p.fillPath(fill,shade);
    p.setPen(QPen(color_,2,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));p.drawPath(line);
}

MetricGlyphWidget::MetricGlyphWidget(Kind kind, const QColor& color,
                                     const QColor& background, QWidget* parent)
    : QWidget(parent), kind_(kind), color_(color), background_(background)
{
    setFixedSize(44,44);
}

void MetricGlyphWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);p.setRenderHint(QPainter::Antialiasing);p.setPen(Qt::NoPen);p.setBrush(background_);p.drawEllipse(rect());p.setPen(QPen(color_,2.4,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));p.setBrush(Qt::NoBrush);const QPointF c=rect().center();
    if(kind_==Battery){p.drawRoundedRect(QRectF(c.x()-8,c.y()-11,16,22),3,3);p.drawLine(c.x()-3,c.y()-14,c.x()+3,c.y()-14);p.drawLine(c.x(),c.y()-6,c.x(),c.y()+6);p.drawLine(c.x()-4,c.y(),c.x()+4,c.y());}
    else if(kind_==Bolt){QPainterPath path;path.moveTo(c.x()+1,c.y()-12);path.lineTo(c.x()-7,c.y()+1);path.lineTo(c.x()-1,c.y()+1);path.lineTo(c.x()-4,c.y()+12);path.lineTo(c.x()+8,c.y()-3);path.lineTo(c.x()+2,c.y()-3);path.closeSubpath();p.fillPath(path,color_);}
    else if(kind_==Clock){p.drawEllipse(QRectF(c.x()-10,c.y()-10,20,20));p.drawLine(c,c+QPointF(0,-6));p.drawLine(c,c+QPointF(6,3));}
    else {p.drawEllipse(QRectF(c.x()-10,c.y()-10,20,20));QFont f=font();f.setPixelSize(20);f.setWeight(QFont::Bold);p.setFont(f);p.drawText(rect(),Qt::AlignCenter,QStringLiteral("¥"));}
}

RouteTurnGlyph::RouteTurnGlyph(bool compact, QWidget* parent)
    : QWidget(parent), compact_(compact)
{
    setFixedSize(compact_ ? QSize(34, 34) : QSize(58, 58));
}

void RouteTurnGlyph::setManeuver(const QString& maneuver, bool active)
{
    maneuver_ = maneuver;
    active_ = active;
    update();
}

void RouteTurnGlyph::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QColor blue(Theme::Blue);
    const QColor color = active_ ? blue : QColor("#B8C5D3");
    if (!compact_) {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(Theme::BlueSoft));
        p.drawRoundedRect(rect(), 14, 14);
    }

    const QRectF box = rect().adjusted(compact_ ? 7 : 14, compact_ ? 6 : 11,
                                       compact_ ? -7 : -14, compact_ ? -6 : -11);
    QPen pen(color, compact_ ? 2.6 : 4.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    const QPointF bottom(box.center().x(), box.bottom());
    const QPointF top(box.center().x(), box.top());
    const double arrow = compact_ ? 4.5 : 6.5;
    if (maneuver_ == QStringLiteral("left") || maneuver_ == QStringLiteral("right")) {
        const bool right = maneuver_ == QStringLiteral("right");
        const double targetX = right ? box.right() : box.left();
        const double bendY = box.center().y() + 1;
        QPainterPath path;
        path.moveTo(bottom);
        path.lineTo(QPointF(bottom.x(), bendY + 4));
        path.quadTo(QPointF(bottom.x(), bendY), QPointF(bottom.x() + (right ? 4 : -4), bendY));
        path.lineTo(QPointF(targetX, bendY));
        p.drawPath(path);
        p.drawLine(QPointF(targetX, bendY), QPointF(targetX + (right ? -arrow : arrow), bendY - arrow));
        p.drawLine(QPointF(targetX, bendY), QPointF(targetX + (right ? -arrow : arrow), bendY + arrow));
    } else if (maneuver_ == QStringLiteral("uturn")) {
        QPainterPath path;
        path.moveTo(bottom);
        path.lineTo(QPointF(bottom.x(), box.center().y()));
        path.cubicTo(QPointF(bottom.x(), box.top()), QPointF(box.left(), box.top()), QPointF(box.left(), box.center().y()));
        path.lineTo(QPointF(box.left(), box.bottom() - 3));
        p.drawPath(path);
        p.drawLine(QPointF(box.left(), box.bottom() - 3), QPointF(box.left() - arrow / 1.5, box.bottom() - 9));
        p.drawLine(QPointF(box.left(), box.bottom() - 3), QPointF(box.left() + arrow / 1.5, box.bottom() - 9));
    } else if (maneuver_ == QStringLiteral("roundabout")) {
        const QRectF circle = box.adjusted(2, 2, -2, -2);
        p.drawArc(circle, 35 * 16, 285 * 16);
        const QPointF tip(circle.right() - 1, circle.center().y() - 3);
        p.drawLine(tip, tip + QPointF(-arrow, -2));
        p.drawLine(tip, tip + QPointF(-2, arrow));
    } else if (maneuver_ == QStringLiteral("arrive")) {
        p.drawEllipse(QPointF(box.center().x(), box.center().y() - 3), arrow + 2, arrow + 2);
        p.drawLine(QPointF(box.center().x(), box.center().y() + arrow), bottom);
        p.setBrush(color);
        p.drawEllipse(QPointF(box.center().x(), box.center().y() - 3), 2.1, 2.1);
    } else {
        p.drawLine(bottom, top);
        p.drawLine(top, top + QPointF(-arrow, arrow));
        p.drawLine(top, top + QPointF(arrow, arrow));
    }
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
            applyNavigationRoute(route);
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
    if (Session::instance().isLoggedIn() && previewPage == QStringLiteral("home")) {
        QList<StationInfo> stations;
        for (int i = 0; i < 3; ++i) {
            StationInfo station;
            station.stationId = i + 1;
            station.name = i == 0 ? QStringLiteral("北理良乡南门充电站")
                : i == 1 ? QStringLiteral("北理良乡充电站 A-002")
                         : QStringLiteral("良乡大学城充电站");
            station.pricePerKwh = i < 2 ? 1.28 : 1.36;
            station.totalChargers = i == 0 ? 5 : i == 1 ? 4 : 6;
            station.availableChargers = i == 0 ? 2 : i == 1 ? 1 : 0;
            station.distanceKm = i == 0 ? 1.2 : i == 1 ? 1.6 : 2.3;
            station.latitude = appConfig::kPreviewStationLatitude + i * 0.002;
            station.longitude = appConfig::kPreviewStationLongitude + i * 0.002;
            station.address = i < 2 ? QStringLiteral("北京市房山区良乡高教园区")
                                    : QStringLiteral("北京市房山区良乡大学城西路");
            station.fastChargers = i == 0 ? 4 : i == 1 ? 3 : 2;
            station.slowChargers = i == 0 ? 1 : i == 1 ? 1 : 4;
            station.maxPowerKw = i < 2 ? 120.0 : 60.0;
            stations.append(station);
        }
        renderStationList(stations);
        showPage(HomePage);
    } else if (Session::instance().isLoggedIn() && previewPage == QStringLiteral("charging")) {
        activeOrder_.orderId = 2026090401;
        activeOrder_.status = QStringLiteral("CHARGING");
        activeOrder_.stationName = QStringLiteral("北理良乡南门充电站");
        activeOrder_.chargerId = 11;
        activeOrder_.chargerCode = QStringLiteral("A-001");
        activeOrder_.chargerType = protocol::ChargerTypeFast;
        activeOrder_.powerKw = 120.0;
        activeOrder_.energyKwh = 32.60;
        activeOrder_.targetEnergyKwh = 52.58;
        activeOrder_.progressPercent = 62.0;
        activeOrder_.estimatedAmount = 12.86;
        activeOrder_.pricePerKwh = 1.28;
        activeOrder_.startTimeMs = QDateTime::currentMSecsSinceEpoch() - 1716000;
        activeOrder_.remainingSeconds = 4680;
        activeOrder_.voltageV = 624;
        activeOrder_.currentA = 192;
        activeOrder_.powerTrend = {112,114,113,116,118,117,120,121};
        activeOrder_.energyTrend = {18,20,23,25,27,29,31,32.6};
        renderOrder(activeOrder_);
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
            if (previewPage == QStringLiteral("navigation")) {
                showNavigation();
                if (!RoutePlanner::hasApiKey()) applyNavigationRoute(previewNavigationRoute());
            }
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
    auto* layout = new QVBoxLayout(page); layout->setContentsMargins(0,0,0,0); layout->setSpacing(0);

    auto* hero = new QFrame; hero->setObjectName(QStringLiteral("LoginHero"));
    hero->setMinimumHeight(430);
    hero->setStyleSheet(QStringLiteral(
        "QFrame#LoginHero{border-image:url(:/resources/login-hero-v2.png) 0 0 0 0 stretch stretch;}"));
    auto* heroLayout = new QVBoxLayout(hero); heroLayout->setContentsMargins(28,24,28,24); heroLayout->setSpacing(10);
    auto* brand = label(QStringLiteral("NEU · CHARGE"), "Muted"); brand->setStyleSheet(QStringLiteral("color:#176CFF;font-size:15px;font-weight:700;letter-spacing:2px"));
    connectionLabel_ = label(QStringLiteral("正在连接服务"), "StatusWarn"); connectionLabel_->setAlignment(Qt::AlignCenter); connectionLabel_->setFixedWidth(112);
    auto* top = new QHBoxLayout; top->addWidget(brand); top->addStretch(); top->addWidget(connectionLabel_); heroLayout->addLayout(top);
    heroLayout->addSpacing(58);
    auto* title = label(QStringLiteral("更近的能源线路<br>更<span style='color:#176CFF'>高效</span>的出发"));
    title->setTextFormat(Qt::RichText); title->setStyleSheet(QStringLiteral("font-size:34px;font-weight:750;color:#14243A")); heroLayout->addWidget(title);
    auto* marker = new QFrame; marker->setFixedSize(42,6); marker->setStyleSheet(QStringLiteral("background:#176CFF;border-radius:3px")); heroLayout->addWidget(marker);
    heroLayout->addWidget(label(QStringLiteral("查找附近充电站，智能规划路线"), "Muted"));
    heroLayout->addStretch();
    layout->addWidget(hero);

    auto* form = new QFrame; form->setObjectName(QStringLiteral("Surface"));
    auto* formLayout = new QVBoxLayout(form); formLayout->setContentsMargins(24,20,24,18); formLayout->setSpacing(12);
    auto* phoneRow = new QFrame; phoneRow->setStyleSheet(QStringLiteral("QFrame{background:#FFFFFF;border:1px solid #DCE5E8;border-radius:14px}"));
    auto* phoneLayout = new QHBoxLayout(phoneRow); phoneLayout->setContentsMargins(14,0,8,0); phoneLayout->setSpacing(10);
    auto* prefix = label(QStringLiteral("+86"), "SectionTitle"); prefix->setFixedWidth(44); phoneLayout->addWidget(prefix);
    auto* divider = new QFrame; divider->setFixedSize(1,28); divider->setStyleSheet(QStringLiteral("background:#DCE5E8")); phoneLayout->addWidget(divider);
    phoneInput_ = new QLineEdit; phoneInput_->setPlaceholderText(QStringLiteral("请输入 11 位手机号")); phoneInput_->setMaxLength(11); phoneInput_->setInputMethodHints(Qt::ImhDigitsOnly); phoneInput_->setStyleSheet(QStringLiteral("border:none;background:transparent")); phoneLayout->addWidget(phoneInput_,1); formLayout->addWidget(phoneRow);
    loginButton_ = button(QStringLiteral("登录 / 自动注册")); formLayout->addWidget(loginButton_);
    auto* consent = new QCheckBox(QStringLiteral("我已阅读并同意《用户服务协议》和《隐私政策》")); consent->setChecked(true); consent->setStyleSheet(QStringLiteral("QCheckBox{color:#647487;font-size:12px} QCheckBox::indicator{width:18px;height:18px}")); formLayout->addWidget(consent,0,Qt::AlignHCenter);
    loginNotice_ = label(QString(), "Muted"); loginNotice_->setMinimumHeight(22); formLayout->addWidget(loginNotice_);
    auto* benefits = new QHBoxLayout; benefits->setSpacing(0);
    const QStringList benefitTexts{QStringLiteral("附近充电站\n快速找站"),QStringLiteral("智能路线规划\n导航直达"),QStringLiteral("便捷充电\n状态同步")};
    for(const QString& text:benefitTexts){auto* item=label(text,"Muted");item->setAlignment(Qt::AlignCenter);item->setStyleSheet(QStringLiteral("color:#41536B;font-size:12px"));benefits->addWidget(item,1);} formLayout->addLayout(benefits);
    layout->addWidget(form);
    layout->addStretch();
    connect(loginButton_, &QPushButton::clicked, this, &MobileApp::attemptLogin);
    connect(phoneInput_, &QLineEdit::returnPressed, this, &MobileApp::attemptLogin);
    return page;
}

QWidget* MobileApp::buildHomePage()
{
    auto* page = new QWidget;
    page->setObjectName(QStringLiteral("Page"));
    auto* outer = new QVBoxLayout(page);
    outer->setContentsMargins(0, 0, 0, 0);

    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto* body = new QWidget;
    auto* layout = new QVBoxLayout(body);
    layout->setContentsMargins(16, 10, 16, 10);
    layout->setSpacing(0);

    auto* titleRow = new QHBoxLayout;
    titleRow->setSpacing(8);
    auto* texts = new QVBoxLayout;
    texts->setSpacing(1);
    texts->addWidget(label(QStringLiteral("附近充电站"), "HomeTitle"));
    auto* locationRow = new QHBoxLayout;
    locationRow->setSpacing(4);
    auto* pin = label(QString());
    pin->setPixmap(homeIcon(HomeIcon::Pin, QColor(Theme::Blue)).pixmap(15, 15));
    pin->setFixedSize(16, 16);
    pin->setAlignment(Qt::AlignCenter);
    locationRow->addWidget(pin);
    locationLabel_ = label(QStringLiteral("尚未设置位置"), "HomeLocation");
    locationLabel_->setWordWrap(false);
    locationRow->addWidget(locationLabel_);
    auto* locationChevron = label(QString());
    locationChevron->setPixmap(homeIcon(HomeIcon::Chevron, QColor(Theme::Muted)).pixmap(11, 11));
    locationChevron->setFixedSize(12, 16);
    locationRow->addWidget(locationChevron);
    locationRow->addStretch();
    texts->addLayout(locationRow);
    titleRow->addLayout(texts, 1);
    auto* refresh = button(QStringLiteral("刷新"), "HomeRefresh");
    configureIconButton(refresh, HomeIcon::Refresh, QColor(Theme::Blue), QSize(16, 16));
    refresh->setFixedSize(70, 42);
    titleRow->addWidget(refresh, 0, Qt::AlignTop);
    layout->addLayout(titleRow);

    layout->addSpacing(6);
    auto* searchRow = new QHBoxLayout;
    searchRow->setSpacing(8);
    auto* searchShell = new QFrame;
    searchShell->setObjectName(QStringLiteral("HomeSearchShell"));
    auto* searchLayout = new QHBoxLayout(searchShell);
    searchLayout->setContentsMargins(14, 0, 8, 0);
    searchLayout->setSpacing(8);
    auto* searchIcon = label(QString());
    searchIcon->setPixmap(homeIcon(HomeIcon::Search, QColor("#77879A")).pixmap(20, 20));
    searchIcon->setFixedSize(22, 22);
    searchLayout->addWidget(searchIcon);
    addressInput_ = new QLineEdit;
    addressInput_->setPlaceholderText(QStringLiteral("输入城市、区域或详细地址"));
    addressInput_->setStyleSheet(QStringLiteral("QLineEdit{border:none;background:transparent;padding:0;min-height:48px;} QLineEdit:focus{border:none;padding:0;}"));
    searchLayout->addWidget(addressInput_, 1);
    locateButton_ = button(QStringLiteral("定位"), "HomeLocate");
    configureIconButton(locateButton_, HomeIcon::Locate, QColor(Theme::Blue), QSize(19, 19));
    locateButton_->setFixedSize(86, 50);
    searchRow->addWidget(searchShell, 1);
    searchRow->addWidget(locateButton_);
    layout->addLayout(searchRow);

    homeNotice_ = label(QString(), "Muted");
    homeNotice_->hide();
    layout->addWidget(homeNotice_);

    layout->addSpacing(8);
    auto* mapStage = new QWidget;
    mapStage->setFixedHeight(230);
    auto* mapStack = new QGridLayout(mapStage);
    mapStack->setContentsMargins(0, 0, 0, 0);
    mapStack->setSpacing(0);
    mapWidget_ = new AmapWidget;
    mapWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mapStack->addWidget(mapWidget_, 0, 0);
    layout->addWidget(mapStage);

    layout->addSpacing(10);
    auto* listTitle = new QHBoxLayout;
    listTitle->setSpacing(3);
    listTitle->addWidget(label(QStringLiteral("附近站点"), "HomeSectionTitle"));
    const QStringList filterNames{QStringLiteral("快充"), QStringLiteral("慢充"),
                                  QStringLiteral("空闲"), QStringLiteral("最近")};
    const QStringList filterTips{QStringLiteral("优先显示快充站点"), QStringLiteral("优先显示慢充站点"),
                                 QStringLiteral("空闲站点优先"), QStringLiteral("距离最近优先")};
    const QList<HomeIcon> filterIcons{HomeIcon::Bolt, HomeIcon::SlowBolt,
                                      HomeIcon::Star, HomeIcon::Target};
    auto* filterGroup = new QButtonGroup(this);
    filterGroup->setExclusive(true);
    for (int index = 0; index < filterNames.size(); ++index) {
        auto* chip = button(filterNames.at(index), "HomeFilter");
        chip->setToolTip(filterTips.at(index));
        chip->setCheckable(true);
        chip->setChecked(index == 0);
        chip->setFixedSize(49, 30);
        chip->setProperty("homeIconKind", static_cast<int>(filterIcons.at(index)));
        configureIconButton(chip, filterIcons.at(index),
                            QColor(index == 0 ? Theme::Blue : "#7B8CA2"), QSize(12, 12));
        filterGroup->addButton(chip, index);
        listTitle->addWidget(chip);
        connect(chip, &QPushButton::clicked, this, [this, filterGroup, index] {
            homeFilter_ = index;
            for (QAbstractButton* item : filterGroup->buttons()) {
                auto* filter = qobject_cast<QPushButton*>(item);
                if (!filter) continue;
                configureIconButton(filter,
                    static_cast<HomeIcon>(filter->property("homeIconKind").toInt()),
                    QColor(filter->isChecked() ? Theme::Blue : "#7B8CA2"), QSize(12, 12));
            }
            applyHomeFilter();
        });
    }
    listTitle->addStretch();
    stationCountLabel_ = label(QStringLiteral("共 0 个"), "HomeSectionCount");
    stationCountLabel_->setWordWrap(false);
    listTitle->addWidget(stationCountLabel_);
    auto* listChevron = label(QString());
    listChevron->setPixmap(homeIcon(HomeIcon::Chevron, QColor(Theme::Muted)).pixmap(12, 12));
    listChevron->setFixedSize(14, 18);
    listTitle->addWidget(listChevron);
    layout->addLayout(listTitle);

    layout->addSpacing(8);
    stationListBody_ = new QWidget;
    stationListLayout_ = new QVBoxLayout(stationListBody_);
    stationListLayout_->setContentsMargins(0, 0, 0, 0);
    stationListLayout_->setSpacing(8);
    layout->addWidget(stationListBody_);
    layout->addStretch();
    scroll->setWidget(body);
    outer->addWidget(scroll);

    connect(locateButton_, &QPushButton::clicked, this, &MobileApp::locateAddress);
    connect(addressInput_, &QLineEdit::returnPressed, this, &MobileApp::locateAddress);
    connect(refresh, &QPushButton::clicked, this, &MobileApp::requestNearbyStations);
    return page;
}

QWidget* MobileApp::buildStationPage()
{
    auto* page = new QWidget; page->setObjectName(QStringLiteral("Page")); auto* outer = new QVBoxLayout(page); outer->setContentsMargins(0,0,0,0);
    auto* scroll = new QScrollArea; scroll->setWidgetResizable(true); auto* body = new QWidget; auto* layout = new QVBoxLayout(body); layout->setContentsMargins(18,18,18,12); layout->setSpacing(12);
    auto* top = new QHBoxLayout; auto* back = button(QStringLiteral("返回"), "Quiet"); back->setFixedWidth(72); top->addWidget(back); top->addStretch(); top->addWidget(label(QStringLiteral("站点详情"),"SectionTitle")); top->addStretch(); auto* favorite=button(QStringLiteral("收藏"),"Quiet"); favorite->setFixedWidth(72); top->addWidget(favorite); layout->addLayout(top);
    auto* stationPlate = new QFrame; stationPlate->setObjectName(QStringLiteral("SoftSurface")); auto* plateLayout = new QVBoxLayout(stationPlate); plateLayout->setContentsMargins(18,18,18,18); plateLayout->setSpacing(7);
    auto* heading = new QHBoxLayout; detailName_ = label(QStringLiteral("充电站"), "PageTitle"); detailName_->setStyleSheet(QStringLiteral("font-size:24px")); heading->addWidget(detailName_, 1); detailMeta_ = label(QStringLiteral("读取中"), "StatusInfo"); detailMeta_->setAlignment(Qt::AlignCenter); detailMeta_->setFixedWidth(96); heading->addWidget(detailMeta_); plateLayout->addLayout(heading);
    detailAddress_ = label(QString(), "Muted"); plateLayout->addWidget(detailAddress_); plateLayout->addSpacing(12);
    auto* metricLayout = new QHBoxLayout; auto* priceBox = new QVBoxLayout; priceBox->addWidget(label(QStringLiteral("电价"), "Muted")); detailPrice_ = label(QStringLiteral("¥ —"), "Amount"); priceBox->addWidget(detailPrice_); metricLayout->addLayout(priceBox); metricLayout->addStretch();
    auto* availableBox = new QVBoxLayout; availableBox->addWidget(label(QStringLiteral("当前空闲"), "Muted")); detailAvailability_ = label(QStringLiteral("—"), "Metric"); detailAvailability_->setStyleSheet(QStringLiteral("color:#149B68")); availableBox->addWidget(detailAvailability_); metricLayout->addLayout(availableBox); plateLayout->addLayout(metricLayout);
    auto* divider = new QFrame; divider->setObjectName(QStringLiteral("Divider")); plateLayout->addWidget(divider); auto* stationStats = new QHBoxLayout; stationStats->setSpacing(4);
    auto addStat=[&](const QString& title,QLabel*& value){auto* box=new QVBoxLayout;box->setSpacing(2);value=label(QStringLiteral("—"),"SectionTitle");value->setAlignment(Qt::AlignCenter);auto* caption=label(title,"Muted");caption->setAlignment(Qt::AlignCenter);box->addWidget(value);box->addWidget(caption);stationStats->addLayout(box,1);};addStat(QStringLiteral("总桩"),detailTotal_);addStat(QStringLiteral("快充"),detailFast_);addStat(QStringLiteral("慢充"),detailSlow_);plateLayout->addLayout(stationStats);layout->addWidget(stationPlate);
    detailNotice_ = label(QString(), "Muted"); layout->addWidget(detailNotice_); layout->addSpacing(8); layout->addWidget(label(QStringLiteral("充电桩"), "SectionTitle")); chargerListBody_ = new QWidget; chargerListLayout_ = new QVBoxLayout(chargerListBody_); chargerListLayout_->setContentsMargins(0,0,0,0); chargerListLayout_->setSpacing(10); layout->addWidget(chargerListBody_); layout->addStretch(); scroll->setWidget(body); outer->addWidget(scroll,1);
    auto* route = button(QStringLiteral("导航到站"), "Secondary"); auto* reserve = button(QStringLiteral("预约充电"), "Primary"); outer->addWidget(bottomActionBar({route,reserve}));
    connect(back,&QPushButton::clicked,this,[this]{showPage(HomePage);});
    connect(route,&QPushButton::clicked,this,[this]{ currentCharger_ = ChargerInfo{}; showNavigation(); });
    connect(reserve,&QPushButton::clicked,this,[this]{if(preferredCharger_.valid())showChargerDetail(preferredCharger_);});
    return page;
}

QWidget* MobileApp::buildChargerPage()
{
    auto* page = new QWidget;
    page->setObjectName(QStringLiteral("Page"));
    auto* outer = new QVBoxLayout(page);
    outer->setContentsMargins(0,0,0,0);
    outer->setSpacing(0);

    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    auto* body = new QWidget;
    auto* layout = new QVBoxLayout(body);
    layout->setContentsMargins(16,14,16,12);
    layout->setSpacing(10);
    scroll->setWidget(body);
    outer->addWidget(scroll,1);

    auto* top = new QHBoxLayout;
    auto* back = button(QStringLiteral("返回"), "Quiet");
    back->setFixedWidth(72);
    top->addWidget(back);
    top->addStretch();
    top->addWidget(label(QStringLiteral("充电桩详情"),"PageTitle"));
    top->addStretch();
    auto* help = button(QStringLiteral("客服"),"Quiet");
    help->setFixedWidth(72);
    top->addWidget(help);
    layout->addLayout(top);

    auto* stationRow = new QHBoxLayout;
    chargerStationLabel_ = label(QStringLiteral("所属充电站"),"SectionTitle");
    stationRow->addWidget(chargerStationLabel_,1);
    chargerMeta_ = label(QStringLiteral("读取中"),"StatusOk");
    chargerMeta_->setAlignment(Qt::AlignCenter);
    stationRow->addWidget(chargerMeta_);
    layout->addLayout(stationRow);

    auto* hero = new QFrame;
    hero->setObjectName(QStringLiteral("ChargerHero"));
    hero->setStyleSheet(QStringLiteral("QFrame#ChargerHero{background:#FFFFFF;border:1px solid #DDE7FA;border-radius:16px}"));
    auto* heroLayout = new QVBoxLayout(hero);
    heroLayout->setContentsMargins(18,14,18,12);
    heroLayout->setSpacing(8);

    auto* deviceRow = new QHBoxLayout;
    auto* identity = new QVBoxLayout;
    chargerCode_ = label(QStringLiteral("充电桩"),"PageTitle");
    identity->addWidget(chargerCode_);
    auto* stateRow = new QHBoxLayout;
    chargerStatus_ = label(QStringLiteral("读取中"),"StatusInfo");
    chargerStatus_->setAlignment(Qt::AlignCenter);
    chargerStatus_->setFixedWidth(78);
    stateRow->addWidget(chargerStatus_);
    chargerHint_ = label(QStringLiteral("状态同步中"),"StatusInfo");
    chargerHint_->setAlignment(Qt::AlignCenter);
    stateRow->addWidget(chargerHint_);
    stateRow->addStretch();
    identity->addLayout(stateRow);
    identity->addStretch();
    deviceRow->addLayout(identity,1);
    auto* product = new QLabel;
    product->setPixmap(QPixmap(QStringLiteral(":/resources/charger-product-v2.png"))
        .scaled(108,132,Qt::KeepAspectRatio,Qt::SmoothTransformation));
    product->setFixedSize(114,136);
    product->setAlignment(Qt::AlignCenter);
    deviceRow->addWidget(product);
    heroLayout->addLayout(deviceRow);

    auto* specs = new QHBoxLayout;
    specs->setSpacing(12);
    auto addSpec = [&](const QString& caption, QLabel*& value) {
        auto* box = new QVBoxLayout;
        box->setSpacing(3);
        box->addWidget(label(caption,"Muted"));
        value = label(QStringLiteral("—"),"SectionTitle");
        box->addWidget(value);
        specs->addLayout(box,1);
    };
    addSpec(QStringLiteral("额定功率"),chargerPower_);
    addSpec(QStringLiteral("枪型"),chargerMethod_);
    addSpec(QStringLiteral("当前状态"),chargerStateSpec_);
    heroLayout->addLayout(specs);

    auto* heroDivider = new QFrame;
    heroDivider->setObjectName(QStringLiteral("Divider"));
    heroLayout->addWidget(heroDivider);
    auto* priceRow = new QHBoxLayout;
    priceRow->addWidget(label(QStringLiteral("充电价格"),"SectionTitle"));
    priceRow->addStretch();
    chargerPrice_ = label(QStringLiteral("¥ — /度"),"Amount");
    priceRow->addWidget(chargerPrice_);
    heroLayout->addLayout(priceRow);
    heroLayout->addWidget(label(QStringLiteral("最终费用以服务端结算结果为准"),"Muted"));
    layout->addWidget(hero);

    auto* details = surface();
    auto* detailsLayout = new QVBoxLayout(details);
    detailsLayout->setContentsMargins(18,8,18,8);
    detailsLayout->setSpacing(0);
    auto addDetail = [&](const QString& name, QLabel*& value, bool divider) {
        auto* row = new QHBoxLayout;
        row->setContentsMargins(0,9,0,9);
        row->addWidget(label(name,"SectionTitle"));
        row->addStretch();
        value = label(QString(),"Muted");
        value->setAlignment(Qt::AlignRight);
        value->setMaximumWidth(220);
        row->addWidget(value);
        detailsLayout->addLayout(row);
        if (divider) { auto* line=new QFrame; line->setObjectName(QStringLiteral("Divider")); detailsLayout->addWidget(line); }
    };
    QLabel* billing = nullptr;
    QLabel* openHours = nullptr;
    QLabel* support = nullptr;
    addDetail(QStringLiteral("计费说明"),billing,true);
    billing->setText(QStringLiteral("服务端实时计费"));
    addDetail(QStringLiteral("开放时间"),openHours,true);
    openHours->setText(QStringLiteral("以站点营业状态为准"));
    addDetail(QStringLiteral("桩位位置"),chargerLocation_,true);
    addDetail(QStringLiteral("支持方式"),support,false);
    support->setText(QStringLiteral("预约充电"));
    layout->addWidget(details);

    auto* guide = surface();
    auto* guideLayout = new QVBoxLayout(guide);
    guideLayout->setContentsMargins(18,13,18,13);
    guideLayout->setSpacing(9);
    guideLayout->addWidget(label(QStringLiteral("到站后，3 步开始充电"),"SectionTitle"));
    auto* steps = new QHBoxLayout;
    const QStringList stepTitles{QStringLiteral("确认桩编号"),QStringLiteral("插枪"),QStringLiteral("开始充电")};
    for (int i=0;i<stepTitles.size();++i) {
        auto* box = new QVBoxLayout;
        auto* number = label(QString::number(i+1));
        number->setAlignment(Qt::AlignCenter);
        number->setFixedSize(28,28);
        number->setStyleSheet(QStringLiteral("background:#176CFF;color:white;border-radius:14px;font-weight:700"));
        box->addWidget(number,0,Qt::AlignHCenter);
        auto* title = label(stepTitles[i],"Muted");
        title->setAlignment(Qt::AlignCenter);
        box->addWidget(title);
        steps->addLayout(box,1);
    }
    guideLayout->addLayout(steps);
    layout->addWidget(guide);

    auto* tips = surface();
    auto* tipsLayout = new QVBoxLayout(tips);
    tipsLayout->setContentsMargins(18,12,18,12);
    tipsLayout->setSpacing(4);
    tipsLayout->addWidget(label(QStringLiteral("温馨提示"),"SectionTitle"));
    tipsLayout->addWidget(label(QStringLiteral("确认车辆连接可靠后再启动充电\n充电结束后请及时将充电枪归位"),"Muted"));
    layout->addWidget(tips);
    chargerNotice_ = label(QString(),"Muted");
    layout->addWidget(chargerNotice_);
    layout->addSpacing(12);
    layout->addStretch();

    auto* actionBar = new QWidget;
    auto* actionLayout = new QHBoxLayout(actionBar);
    actionLayout->setContentsMargins(16,8,16,16);
    actionLayout->setSpacing(10);
    auto* navigate = button(QStringLiteral("导航到此充电桩"),"Secondary");
    reserveButton_ = button(QStringLiteral("预约充电桩"),"Primary");
    actionLayout->addWidget(navigate,1);
    actionLayout->addWidget(reserveButton_,1);
    outer->addWidget(actionBar);
    connect(back,&QPushButton::clicked,this,[this]{showPage(StationPage);});
    connect(navigate,&QPushButton::clicked,this,&MobileApp::showNavigation);
    connect(reserveButton_,&QPushButton::clicked,this,[this]{reserveCharger(currentCharger_);});
    return page;
}

QWidget* MobileApp::buildNavigationPage()
{
    auto* page = new QWidget;
    page->setObjectName(QStringLiteral("Page"));
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);

    auto* top = new QWidget;
    top->setFixedHeight(62);
    auto* topLayout = new QHBoxLayout(top);
    topLayout->setContentsMargins(12,8,12,6);
    auto* back = new BackGlyphButton;
    topLayout->addWidget(back);
    topLayout->addStretch();
    auto* title = label(QStringLiteral("导航到充电站"), "NavigationHeader");
    title->setAlignment(Qt::AlignCenter);
    topLayout->addWidget(title);
    topLayout->addStretch();
    auto* titleSpacer = new QWidget;
    titleSpacer->setFixedWidth(44);
    topLayout->addWidget(titleSpacer);
    layout->addWidget(top);

    auto* mapStage = new QWidget;
    auto* mapStack = new QGridLayout(mapStage);
    mapStack->setContentsMargins(0,0,0,0);
    mapStack->setSpacing(0);
    navigationMap_ = new AmapWidget;
    navigationMap_->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    mapStack->addWidget(navigationMap_,0,0);

    auto* overlay = new QWidget;
    overlay->setAttribute(Qt::WA_TransparentForMouseEvents);
    auto* overlayLayout = new QVBoxLayout(overlay);
    overlayLayout->setContentsMargins(14,14,14,14);
    overlayLayout->setSpacing(9);

    auto* instructionCard = new QFrame;
    instructionCard->setObjectName(QStringLiteral("RouteInstructionCard"));
    instructionCard->setMaximumWidth(334);
    auto* instructionLayout = new QHBoxLayout(instructionCard);
    instructionLayout->setContentsMargins(14,14,16,14);
    instructionLayout->setSpacing(12);
    auto* instructionMain = new QHBoxLayout;
    instructionMain->setSpacing(12);
    navigationTurnGlyph_ = new RouteTurnGlyph(false);
    instructionMain->addWidget(navigationTurnGlyph_,0,Qt::AlignVCenter);
    auto* instructionText = new QVBoxLayout;
    instructionText->setSpacing(1);
    navigationStepLead_ = label(QStringLiteral("正在规划路线…"), "NavigationStepLead");
    navigationRoad_ = label(QStringLiteral("请稍候"), "NavigationRoad");
    navigationRoad_->setMaximumHeight(48);
    instructionText->addWidget(navigationStepLead_);
    instructionText->addWidget(navigationRoad_);
    instructionMain->addLayout(instructionText,1);
    instructionLayout->addLayout(instructionMain);
    overlayLayout->addWidget(instructionCard,0,Qt::AlignLeft);
    overlayLayout->addStretch();

    auto* summaryCard = new QFrame;
    summaryCard->setObjectName(QStringLiteral("NavigationStatsCard"));
    summaryCard->setFixedWidth(232);
    auto* summaryLayout = new QHBoxLayout(summaryCard);
    summaryLayout->setContentsMargins(13,10,13,10);
    summaryLayout->setSpacing(12);
    auto* remainingBox = new QVBoxLayout;
    remainingBox->setSpacing(0);
    navigationDuration_ = label(QStringLiteral("— 分钟"), "MapSummaryDuration");
    navigationDuration_->setWordWrap(false);
    navigationDistance_ = label(QStringLiteral("— 公里"), "MapSummaryDistance");
    navigationDistance_->setWordWrap(false);
    remainingBox->addWidget(navigationDuration_);
    remainingBox->addWidget(navigationDistance_);
    summaryLayout->addLayout(remainingBox,1);
    auto* summaryDivider = new QFrame;
    summaryDivider->setObjectName(QStringLiteral("VerticalDivider"));
    summaryDivider->setMinimumHeight(46);
    summaryLayout->addWidget(summaryDivider);
    auto* etaSummary = new QVBoxLayout;
    etaSummary->setSpacing(2);
    etaSummary->addWidget(label(QStringLiteral("预计到达"), "MapSummaryLabel"));
    navigationMapEta_ = label(QStringLiteral("—:—"), "MapSummaryEta");
    navigationMapEta_->setWordWrap(false);
    etaSummary->addWidget(navigationMapEta_);
    summaryLayout->addLayout(etaSummary,1);
    overlayLayout->addWidget(summaryCard,0,Qt::AlignLeft);
    mapStack->addWidget(overlay,0,0);
    layout->addWidget(mapStage,1);

    auto* panel = new QFrame;
    panel->setObjectName(QStringLiteral("NavigationPanel"));
    auto* panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(16,8,16,14);
    panelLayout->setSpacing(8);

    auto* handle = new QFrame;
    handle->setObjectName(QStringLiteral("NavigationHandle"));
    handle->setFixedWidth(46);
    panelLayout->addWidget(handle,0,Qt::AlignHCenter);

    auto* stationRow = new QHBoxLayout;
    stationRow->setSpacing(10);
    stationRow->addWidget(new MetricGlyphWidget(MetricGlyphWidget::Bolt,
        QColor("#FFFFFF"), QColor(Theme::Blue)));
    auto* stationText = new QVBoxLayout;
    stationText->setSpacing(2);
    navigationTarget_ = label(QStringLiteral("正在准备终点"), "NavigationStationTitle");
    navigationTarget_->setMaximumHeight(46);
    navigationStationMeta_ = label(QStringLiteral("正在读取充电站信息"), "NavigationStationMeta");
    stationText->addWidget(navigationTarget_);
    stationText->addWidget(navigationStationMeta_);
    stationRow->addLayout(stationText,1);
    navigationStatus_ = label(QStringLiteral("同步中"), "StatusInfo");
    navigationStatus_->setWordWrap(false);
    navigationStatus_->setAlignment(Qt::AlignCenter);
    stationRow->addWidget(navigationStatus_,0,Qt::AlignTop);
    panelLayout->addLayout(stationRow);

    auto* divider = new QFrame;
    divider->setObjectName(QStringLiteral("Divider"));
    panelLayout->addWidget(divider);

    auto* stationMetrics = new QHBoxLayout;
    stationMetrics->setSpacing(0);
    auto addStationMetric = [&](const QString& caption, QLabel*& value, const char* objectName) {
        auto* box = new QVBoxLayout;
        box->setSpacing(2);
        auto* captionLabel = label(caption, "NavigationMetricLabel");
        captionLabel->setAlignment(Qt::AlignCenter);
        value = label(QStringLiteral("—"), objectName);
        value->setAlignment(Qt::AlignCenter);
        value->setWordWrap(false);
        box->addWidget(captionLabel);
        box->addWidget(value);
        stationMetrics->addLayout(box,1);
    };
    addStationMetric(QStringLiteral("空闲"), navigationAvailability_, "NavigationMetricValueGreen");
    addStationMetric(QStringLiteral("电价"), navigationPrice_, "NavigationMetricValue");
    auto* etaBox = new QVBoxLayout;
    etaBox->setSpacing(0);
    auto* etaCaption = label(QStringLiteral("距终点"), "NavigationMetricLabel");
    etaCaption->setAlignment(Qt::AlignCenter);
    navigationEta_ = label(QStringLiteral("— 公里"), "NavigationMetricValue");
    navigationEta_->setAlignment(Qt::AlignCenter);
    navigationEta_->setWordWrap(false);
    navigationEtaHint_ = label(QStringLiteral("预计 —:— 到达"), "NavigationMetricLabel");
    navigationEtaHint_->setAlignment(Qt::AlignCenter);
    navigationEtaHint_->setWordWrap(false);
    etaBox->addWidget(etaCaption);
    etaBox->addWidget(navigationEta_);
    etaBox->addWidget(navigationEtaHint_);
    stationMetrics->addLayout(etaBox,1);
    panelLayout->addLayout(stationMetrics);

    navigationNotice_ = label(QStringLiteral("正在规划路线…"), "Muted");
    navigationNotice_->setAlignment(Qt::AlignCenter);
    panelLayout->addWidget(navigationNotice_);

    auto* actions = new QHBoxLayout;
    actions->setSpacing(10);
    navigationEndAction_ = button(QStringLiteral("结束导航"), "NavigationSecondary");
    navigationAction_ = button(QStringLiteral("继续导航"), "Primary");
    actions->addWidget(navigationEndAction_,1);
    actions->addWidget(navigationAction_,1);
    panelLayout->addLayout(actions);
    layout->addWidget(panel);

    auto leaveNavigation = [this] {
        showPage(currentCharger_.valid() ? ChargerPage : StationPage);
    };
    connect(back,&QAbstractButton::clicked,this,leaveNavigation);
    connect(navigationEndAction_,&QPushButton::clicked,this,leaveNavigation);
    connect(navigationAction_,&QPushButton::clicked,this,[this]{
        if (navigationRoute_.steps.isEmpty()) {
            setNotice(navigationNotice_, QStringLiteral("路线尚未准备好，请稍后再试"), true);
            return;
        }
        if (navigationStepIndex_ + 1 < navigationRoute_.steps.size()) {
            updateNavigationStep(navigationStepIndex_ + 1);
            setNotice(navigationNotice_, QString());
            return;
        }
        navigationAction_->setText(QStringLiteral("已到达"));
        navigationAction_->setEnabled(false);
        setNotice(navigationNotice_, QStringLiteral("全部导航步骤已完成"));
    });
    return page;
}

QWidget* MobileApp::buildChargingPage()
{
    auto* page = new QWidget;
    page->setObjectName(QStringLiteral("Page"));
    auto* outer = new QVBoxLayout(page);
    outer->setContentsMargins(0,0,0,0);
    outer->setSpacing(0);
    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    auto* body = new QWidget;
    auto* layout = new QVBoxLayout(body);
    layout->setContentsMargins(16,14,16,10);
    layout->setSpacing(10);
    scroll->setWidget(body);
    outer->addWidget(scroll,1);

    auto* top = new QHBoxLayout;
    top->addWidget(label(QStringLiteral("充电中"),"PageTitle"));
    chargingStatus_ = label(QStringLiteral("正在同步"),"StatusOk");
    chargingStatus_->setAlignment(Qt::AlignCenter);
    chargingStatus_->setFixedSize(76,30);
    top->addWidget(chargingStatus_);
    top->addStretch();
    auto* refresh = button(QStringLiteral("同步"),"Secondary");
    refresh->setFixedSize(68,34);
    refresh->setStyleSheet(QStringLiteral("background:#EAF1FF;color:#1558C7;border-radius:12px;min-height:34px;max-height:34px;padding:0 12px;font-weight:650"));
    top->addWidget(refresh);
    layout->addLayout(top);

    auto* summary = new QFrame;
    summary->setObjectName(QStringLiteral("ChargingSummary"));
    summary->setStyleSheet(QStringLiteral("QFrame#ChargingSummary{background:#FFFFFF;border:1px solid #E1E9F2;border-radius:16px}"));
    auto* summaryLayout = new QVBoxLayout(summary);
    summaryLayout->setContentsMargins(16,14,16,14);
    summaryLayout->setSpacing(9);
    auto* stationRow = new QHBoxLayout;
    auto* stationIcon = new MetricGlyphWidget(MetricGlyphWidget::Battery,QColor(Theme::Blue),QColor(Theme::BlueSoft));
    stationRow->addWidget(stationIcon);
    auto* stationText = new QVBoxLayout;
    chargingStation_ = label(QStringLiteral("暂无订单"),"SectionTitle");
    stationText->addWidget(chargingStation_);
    chargingMetrics_ = label(QStringLiteral("请选择空闲充电桩"),"Muted");
    stationText->addWidget(chargingMetrics_);
    stationRow->addLayout(stationText,1);
    chargingStateText_ = label(QStringLiteral("等待"),"StatusInfo");
    chargingStateText_->setAlignment(Qt::AlignCenter);
    chargingStateText_->setFixedSize(68,46);
    stationRow->addWidget(chargingStateText_);
    summaryLayout->addLayout(stationRow);
    auto* summaryDivider = new QFrame; summaryDivider->setObjectName(QStringLiteral("Divider")); summaryLayout->addWidget(summaryDivider);
    auto* meta = new QHBoxLayout;
    chargingStart_ = label(QStringLiteral("开始 —"),"Muted");
    chargingMode_ = label(QStringLiteral("模式 —"),"Muted");
    auto* stateCopy = label(QStringLiteral("服务端同步"),"Muted");
    meta->addWidget(chargingStart_);meta->addStretch();meta->addWidget(chargingMode_);meta->addStretch();meta->addWidget(stateCopy);summaryLayout->addLayout(meta);
    layout->addWidget(summary);

    auto* liveArea = new QHBoxLayout;
    liveArea->setSpacing(4);
    auto* powerSide = new QVBoxLayout;powerSide->setSpacing(2);powerSide->addStretch();chargingPowerLive_=label(QStringLiteral("— kW"),"Amount");chargingPowerLive_->setWordWrap(false);chargingPowerLive_->setStyleSheet(QStringLiteral("font-size:20px;font-weight:700"));chargingPowerLive_->setAlignment(Qt::AlignCenter);powerSide->addWidget(chargingPowerLive_);auto* powerCaption=label(QStringLiteral("当前功率"),"Muted");powerCaption->setAlignment(Qt::AlignCenter);powerSide->addWidget(powerCaption);powerWave_=new SparklineWidget(QColor(Theme::Blue));powerSide->addWidget(powerWave_);powerSide->addStretch();liveArea->addLayout(powerSide,1);
    gauge_ = new ChargeGauge; gauge_->setFixedSize(210,210); liveArea->addWidget(gauge_);
    auto* energySide = new QVBoxLayout;energySide->setSpacing(2);energySide->addStretch();chargingEnergyLive_=label(QStringLiteral("— kWh"),"Amount");chargingEnergyLive_->setWordWrap(false);chargingEnergyLive_->setStyleSheet(QStringLiteral("font-size:20px;font-weight:700"));chargingEnergyLive_->setAlignment(Qt::AlignCenter);energySide->addWidget(chargingEnergyLive_);auto* energyCaption=label(QStringLiteral("已充电量"),"Muted");energyCaption->setAlignment(Qt::AlignCenter);energySide->addWidget(energyCaption);energyWave_=new SparklineWidget(QColor(Theme::Green));energySide->addWidget(energyWave_);energySide->addStretch();liveArea->addLayout(energySide,1);layout->addLayout(liveArea);

    auto* strip = surface();auto* stripLayout=new QHBoxLayout(strip);stripLayout->setContentsMargins(12,10,12,10);stripLayout->setSpacing(6);
    auto addStrip=[&](const QString& caption,QLabel*& value){auto* box=new QVBoxLayout;box->setSpacing(1);value=label(QStringLiteral("—"),"SectionTitle");value->setAlignment(Qt::AlignCenter);auto* cap=label(caption,"Muted");cap->setAlignment(Qt::AlignCenter);box->addWidget(value);box->addWidget(cap);stripLayout->addLayout(box,1);};addStrip(QStringLiteral("已用时"),chargingDuration_);addStrip(QStringLiteral("预计剩余"),chargingRemaining_);addStrip(QStringLiteral("预计费用"),chargingFeeSummary_);layout->addWidget(strip);

    auto* grid = new QGridLayout;grid->setHorizontalSpacing(8);grid->setVerticalSpacing(8);
    auto addTile=[&](int row,int col,MetricGlyphWidget::Kind kind,const QColor& color,const QColor& bg,const QString& title,QLabel*& value,QLabel* detail){auto* tile=surface();auto* tileLayout=new QHBoxLayout(tile);tileLayout->setContentsMargins(10,8,10,8);tileLayout->setSpacing(8);tileLayout->addWidget(new MetricGlyphWidget(kind,color,bg));auto* text=new QVBoxLayout;text->setSpacing(0);text->addWidget(label(title,"Muted"));value=label(QStringLiteral("—"),"SectionTitle");value->setWordWrap(false);value->setStyleSheet(QStringLiteral("font-size:17px;font-weight:700"));text->addWidget(value);if(detail){detail->setWordWrap(true);detail->setStyleSheet(QStringLiteral("color:#647487;font-size:11px"));text->addWidget(detail);}tileLayout->addLayout(text,1);grid->addWidget(tile,row,col);};
    addTile(0,0,MetricGlyphWidget::Battery,QColor(Theme::Blue),QColor(Theme::BlueSoft),QStringLiteral("已充电量"),chargingEnergyTile_,nullptr);
    chargingElectrical_=label(QStringLiteral("电压、电流待服务端返回"),"Muted");
    addTile(0,1,MetricGlyphWidget::Bolt,QColor(Theme::Green),QColor(Theme::GreenSoft),QStringLiteral("当前功率"),chargingPowerTile_,chargingElectrical_);
    addTile(1,0,MetricGlyphWidget::Clock,QColor(Theme::Blue),QColor(Theme::BlueSoft),QStringLiteral("已用时"),chargingDurationTile_,nullptr);
    addTile(1,1,MetricGlyphWidget::Coin,QColor("#F39A16"),QColor("#FFF1DC"),QStringLiteral("累计费用"),chargingFeeTile_,nullptr);
    layout->addLayout(grid);

    chargingAmount_=chargingFeeTile_;
    chargingNotice_=label(QString(),"Muted");layout->addWidget(chargingNotice_);layout->addStretch();

    auto* actions=new QWidget;auto* actionsLayout=new QHBoxLayout(actions);actionsLayout->setContentsMargins(16,8,16,10);actionsLayout->setSpacing(10);chargingAction_=button(QStringLiteral("停止充电"),"Danger");chargingOrderButton_=button(QStringLiteral("查看订单"),"Primary");actionsLayout->addWidget(chargingAction_,1);actionsLayout->addWidget(chargingOrderButton_,1);outer->addWidget(actions);
    connect(refresh,&QPushButton::clicked,this,[this]{requestActiveOrder(ChargingPage);});
    connect(chargingOrderButton_,&QPushButton::clicked,this,[this]{requestActiveOrder(OrdersPage);});
    connect(chargingAction_,&QPushButton::clicked,this,[this]{const auto s=activeOrder_.statusEnum();if(!activeOrder_.valid()){showPage(HomePage);}else if(s==OrderInfo::StatusReserved){performOrderAction(QString::fromLatin1(protocol::action::kOrderStart),chargingAction_);}else if(s==OrderInfo::StatusCharging){performOrderAction(QString::fromLatin1(protocol::action::kOrderStop),chargingAction_);}else if(s==OrderInfo::StatusWaitSettlement){performOrderAction(QString::fromLatin1(protocol::action::kOrderSettle),chargingAction_);}});
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
    nearbyStations_ = stations;
    if (stationCountLabel_) {
        stationCountLabel_->setText(QStringLiteral("共 %1 个").arg(stations.size()));
    }

    QList<StationInfo> byDistance = stations;
    std::sort(byDistance.begin(), byDistance.end(), [](const StationInfo& left, const StationInfo& right) {
        return left.distanceKm < right.distanceKm;
    });
    mapWidget_->setStations(byDistance);
    applyHomeFilter();
}

void MobileApp::applyHomeFilter()
{
    QList<StationInfo> stations = nearbyStations_;
    if (homeFilter_ == 0) {
        stations.erase(std::remove_if(stations.begin(), stations.end(), [](const StationInfo& station) {
            return station.fastChargers == 0;
        }), stations.end());
        std::stable_sort(stations.begin(), stations.end(), [](const StationInfo& left, const StationInfo& right) {
            return left.fastChargers > right.fastChargers;
        });
    } else if (homeFilter_ == 1) {
        stations.erase(std::remove_if(stations.begin(), stations.end(), [](const StationInfo& station) {
            return station.slowChargers == 0;
        }), stations.end());
        std::stable_sort(stations.begin(), stations.end(), [](const StationInfo& left, const StationInfo& right) {
            return left.slowChargers > right.slowChargers;
        });
    } else if (homeFilter_ == 2) {
        std::stable_sort(stations.begin(), stations.end(), [](const StationInfo& left, const StationInfo& right) {
            if ((left.availableChargers > 0) != (right.availableChargers > 0)) {
                return left.availableChargers > 0;
            }
            return left.availableChargers > right.availableChargers;
        });
    } else {
        std::sort(stations.begin(), stations.end(), [](const StationInfo& left, const StationInfo& right) {
            return left.distanceKm < right.distanceKm;
        });
    }

    clearLayout(stationListLayout_);
    if (stations.isEmpty()) {
        auto* empty = surface();
        empty->setMinimumHeight(96);
        auto* emptyLayout = new QVBoxLayout(empty);
        emptyLayout->setContentsMargins(16, 14, 16, 14);
        auto* headline = label(QStringLiteral("没有符合当前条件的站点"), "SectionTitle");
        headline->setAlignment(Qt::AlignCenter);
        emptyLayout->addWidget(headline);
        auto* hint = label(QStringLiteral("可切换筛选，或重新输入位置后刷新"), "Muted");
        hint->setAlignment(Qt::AlignCenter);
        emptyLayout->addWidget(hint);
        stationListLayout_->addWidget(empty);
        stationListLayout_->addStretch();
        return;
    }

    int rank = 1;
    for (const StationInfo& station : stations) {
        const bool recommended = rank == 1;
        auto* card = new StationCardWidget(recommended);
        auto* cardLayout = new QHBoxLayout(card);
        cardLayout->setContentsMargins(recommended ? 18 : 12, 7, 10, 7);
        cardLayout->setSpacing(7);

        auto* badge = label(QString::number(rank));
        badge->setAlignment(Qt::AlignCenter);
        badge->setFixedSize(27, 27);
        badge->setStyleSheet(recommended
            ? QStringLiteral("background:#176CFF;color:#FFFFFF;border-radius:13px;font-size:14px;font-weight:700")
            : QStringLiteral("background:#EAF1FF;color:#176CFF;border-radius:13px;font-size:14px;font-weight:700"));
        cardLayout->addWidget(badge, 0, Qt::AlignTop);

        auto* information = new QVBoxLayout;
        information->setSpacing(3);
        auto* stationName = label(station.name, "StationName");
        stationName->setWordWrap(false);
        stationName->setMinimumWidth(0);
        information->addWidget(stationName);

        auto* tagRow = new QHBoxLayout;
        tagRow->setSpacing(4);
        auto addTag = [&](const QString& text, const char* objectName = "StationTag") {
            auto* tag = label(text, objectName);
            tag->setWordWrap(false);
            tag->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
            tagRow->addWidget(tag);
        };
        addTag(station.availableChargers > 0 ? QStringLiteral("可充电") : QStringLiteral("暂无空闲"),
               station.availableChargers > 0 ? "StationTagBlue" : "StationTagAmber");
        if (station.fastChargers > 0) addTag(QStringLiteral("快充"));
        if (station.maxPowerKw > 0.0) {
            addTag(QStringLiteral("%1 kW").arg(station.maxPowerKw, 0, 'f', 0));
        }
        tagRow->addStretch();
        information->addLayout(tagRow);

        auto* address = label(station.address.isEmpty()
            ? QStringLiteral("站点地址请查看详情") : station.address, "HomeLocation");
        address->setStyleSheet(QStringLiteral("color:#54677F;font-size:11px"));
        address->setWordWrap(false);
        address->setMinimumWidth(0);
        information->addWidget(address);

        cardLayout->addLayout(information, 1);

        auto* divider = new QFrame;
        divider->setObjectName(QStringLiteral("VerticalDivider"));
        cardLayout->addWidget(divider);

        auto* metrics = new QVBoxLayout;
        metrics->setSpacing(1);
        auto* price = label(QStringLiteral("¥ %1").arg(station.pricePerKwh, 0, 'f', 2), "StationPrice");
        price->setWordWrap(false);
        metrics->addWidget(price);
        metrics->addWidget(label(QStringLiteral("电价 / 度"), "StationCaption"));
        metrics->addSpacing(5);
        auto* availability = label(QStringLiteral("%1 / %2")
            .arg(station.availableChargers).arg(station.totalChargers),
            station.availableChargers > 0 ? "StationAvailable" : "StationUnavailable");
        availability->setWordWrap(false);
        metrics->addWidget(availability);
        metrics->addWidget(label(QStringLiteral("空闲 / 总数"), "StationCaption"));
        metrics->addStretch();
        cardLayout->addLayout(metrics);

        auto* actions = new QVBoxLayout;
        actions->setSpacing(2);
        auto* distance = label(QStringLiteral("%1 km").arg(station.distanceKm, 0, 'f', 1), "StationDistance");
        distance->setAlignment(Qt::AlignRight);
        distance->setWordWrap(false);
        actions->addWidget(distance);
        const int minutes = qMax(1, qCeil(station.distanceKm / 0.30));
        auto* eta = label(QStringLiteral("约 %1 分钟").arg(minutes), "StationEta");
        eta->setAlignment(Qt::AlignRight);
        eta->setWordWrap(false);
        actions->addWidget(eta);
        auto* navigate = button(QStringLiteral("导航"), "StationNavigate");
        configureIconButton(navigate, HomeIcon::Navigation, Qt::white, QSize(13, 13));
        navigate->setFixedSize(66, 30);
        actions->addWidget(navigate);
        auto* detail = button(QStringLiteral("详情"), "StationDetail");
        detail->setFixedSize(66, 28);
        actions->addWidget(detail);
        cardLayout->addLayout(actions);

        connect(detail, &QPushButton::clicked, this, [this, id = station.stationId] {
            requestStationDetail(id);
        });
        connect(navigate, &QPushButton::clicked, this, [this, id = station.stationId] {
            requestStationDetail(id, true);
        });
        stationListLayout_->addWidget(card);
        ++rank;
    }
    stationListLayout_->addStretch();
}

void MobileApp::requestStationDetail(qint64 stationId, bool navigateAfter)
{
    navigateAfterDetail_=navigateAfter;showPage(StationPage);setNotice(detailNotice_,QStringLiteral("正在同步充电桩…"));clearLayout(chargerListLayout_);QJsonObject data;data.insert(QStringLiteral("stationId"),double(stationId));network_->sendRequest(QString::fromLatin1(protocol::action::kStationDetail),data,[this](const protocol::Response&r){if(!r.isOk()){setNotice(detailNotice_,protocol::describeError(r.code,r.message),true);return;}StationDetail d;d.station=StationInfo::fromJson(r.data);d.address=r.data.value(QStringLiteral("address")).toString();d.latitude=r.data.value(QStringLiteral("latitude")).toDouble();d.longitude=r.data.value(QStringLiteral("longitude")).toDouble();for(const auto&v:r.data.value(QStringLiteral("chargers")).toArray()){const auto c=ChargerInfo::fromJson(v.toObject());if(c.valid())d.chargers.append(c);}renderStationDetail(d);if(navigateAfterDetail_){navigateAfterDetail_=false;currentCharger_=ChargerInfo{};showNavigation();}});
}

void MobileApp::renderStationDetail(const StationDetail& d)
{
    currentStation_=d;preferredCharger_=ChargerInfo{};int fastCount=0;int slowCount=0;for(const auto&c:d.chargers){if(c.type==protocol::ChargerTypeFast)++fastCount;else ++slowCount;if(!preferredCharger_.valid()&&c.isIdle())preferredCharger_=c;}detailName_->setText(d.station.name);detailMeta_->setText(d.station.availableChargers>0?QStringLiteral("可充电"):QStringLiteral("暂无空闲"));detailMeta_->setObjectName(d.station.availableChargers>0?QStringLiteral("StatusOk"):QStringLiteral("StatusWarn"));detailMeta_->style()->unpolish(detailMeta_);detailMeta_->style()->polish(detailMeta_);detailAddress_->setText(d.address);detailPrice_->setText(QStringLiteral("¥ %1/度").arg(d.station.pricePerKwh,0,'f',2));detailAvailability_->setText(QStringLiteral("%1").arg(d.station.availableChargers));detailTotal_->setText(QString::number(d.chargers.size()));detailFast_->setText(QString::number(fastCount));detailSlow_->setText(QString::number(slowCount));setNotice(detailNotice_,d.chargers.isEmpty()?QStringLiteral("暂无充电桩"):QString());clearLayout(chargerListLayout_);
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
    chargerHint_->setText(charger.isIdle()
        ? QStringLiteral("可直接预约") : QStringLiteral("暂不可预约"));
    chargerStationLabel_->setText(currentStation_.station.name);
    chargerMeta_->setText(QStringLiteral("%1桩空闲")
        .arg(currentStation_.station.availableChargers));
    chargerPower_->setText(QStringLiteral("%1 kW").arg(charger.powerKw,0,'f',0));
    chargerMethod_->setText(charger.type == protocol::ChargerTypeFast
        ? QStringLiteral("直流快充") : QStringLiteral("交流慢充"));
    chargerStateSpec_->setText(charger.statusLabel());
    chargerPrice_->setText(QStringLiteral("¥ %1 /度")
        .arg(currentStation_.station.pricePerKwh,0,'f',2));
    chargerLocation_->setText(currentStation_.address);
    reserveButton_->setEnabled(charger.isIdle());
    reserveButton_->setText(charger.isIdle()
        ? QStringLiteral("预约充电桩") : QStringLiteral("当前不可预约"));
    setNotice(chargerNotice_, QString());
    showPage(ChargerPage);
}

void MobileApp::showNavigation()
{
    if (!currentStation_.station.valid()) return;
    navigationRoute_ = RouteResult{};
    navigationStepIndex_ = 0;
    navigationTarget_->setText(currentCharger_.valid()
        ? QStringLiteral("%1 · %2").arg(currentStation_.station.name, currentCharger_.code)
        : currentStation_.station.name);
    navigationStationMeta_->setText(currentCharger_.valid()
        ? QStringLiteral("%1 %2 kW · %3").arg(currentCharger_.typeLabel())
              .arg(currentCharger_.powerKw,0,'f',0)
              .arg(currentCharger_.isIdle() ? QStringLiteral("空闲")
                                            : currentCharger_.statusLabel())
        : QStringLiteral("%1 个充电桩 · 当前 %2 个空闲")
              .arg(currentStation_.station.totalChargers)
              .arg(currentStation_.station.availableChargers));
    const bool available = currentStation_.station.availableChargers > 0;
    navigationStatus_->setText(available ? QStringLiteral("可充电") : QStringLiteral("暂无空闲"));
    navigationStatus_->setObjectName(available ? QStringLiteral("StatusOk") : QStringLiteral("StatusWarn"));
    navigationStatus_->style()->unpolish(navigationStatus_);
    navigationStatus_->style()->polish(navigationStatus_);
    navigationAvailability_->setText(QStringLiteral("%1 / %2")
        .arg(currentStation_.station.availableChargers)
        .arg(currentStation_.station.totalChargers));
    navigationPrice_->setText(QStringLiteral("¥ %1/度")
        .arg(currentStation_.station.pricePerKwh,0,'f',2));
    navigationMapEta_->setText(QStringLiteral("—:—"));
    navigationEta_->setText(QStringLiteral("— 公里"));
    navigationEtaHint_->setText(QStringLiteral("预计 —:— 到达"));
    navigationDistance_->setText(QStringLiteral("— 公里"));
    navigationDuration_->setText(QStringLiteral("— 分钟"));
    navigationStepLead_->setText(QStringLiteral("正在规划路线…"));
    navigationRoad_->setText(QStringLiteral("请稍候"));
    navigationTurnGlyph_->setManeuver(QStringLiteral("straight"), false);
    navigationAction_->setText(QStringLiteral("继续导航"));
    navigationAction_->setEnabled(true);
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

void MobileApp::applyNavigationRoute(const RouteResult& route)
{
    navigationRoute_ = route;
    navigationStepIndex_ = 0;
    navigationMap_->setRoute(route, navigationTarget_->text());
    if (route.steps.isEmpty()) {
        navigationAction_->setEnabled(false);
        setNotice(navigationNotice_, QStringLiteral("路线已绘制，但没有可用的分步指引"), true);
        return;
    }
    navigationAction_->setEnabled(true);
    navigationAction_->setText(QStringLiteral("继续导航"));
    updateNavigationStep(0);
    setNotice(navigationNotice_, QString());
}

void MobileApp::updateNavigationStep(int index)
{
    if (navigationRoute_.steps.isEmpty()) return;
    navigationStepIndex_ = qBound(0, index, static_cast<int>(navigationRoute_.steps.size()) - 1);
    const RouteStep& step = navigationRoute_.steps.at(navigationStepIndex_);
    navigationStepLead_->setText(QStringLiteral("%1后  %2")
        .arg(routeDistanceText(step.distanceMeters), actionTextForStep(step)));
    navigationRoad_->setText(step.roadName.trimmed().isEmpty()
        ? step.instruction : step.roadName);
    navigationTurnGlyph_->setManeuver(maneuverForStep(step), true);

    double remainingDistance = 0.0;
    qint64 remainingDuration = 0;
    for (int stepIndex = navigationStepIndex_; stepIndex < navigationRoute_.steps.size(); ++stepIndex) {
        remainingDistance += navigationRoute_.steps.at(stepIndex).distanceMeters;
        remainingDuration += navigationRoute_.steps.at(stepIndex).durationSeconds;
    }
    const qint64 remainingMinutes = qMax<qint64>(1, (remainingDuration + 59) / 60);
    const QString eta = QDateTime::currentDateTime().addSecs(remainingDuration)
        .toString(QStringLiteral("HH:mm"));
    navigationMapEta_->setText(eta);
    navigationEta_->setText(routeDistanceText(remainingDistance));
    navigationEtaHint_->setText(QStringLiteral("预计 %1 到达").arg(eta));
    navigationDuration_->setText(QStringLiteral("%1 分钟").arg(remainingMinutes));
    navigationDistance_->setText(routeDistanceText(remainingDistance));
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
    const auto state=o.statusEnum();
    const QString stateText=statusName(state);
    const QString statusObject=state==OrderInfo::StatusCharging?QStringLiteral("StatusOk"):(state==OrderInfo::StatusReserved||state==OrderInfo::StatusWaitSettlement?QStringLiteral("StatusWarn"):QStringLiteral("StatusInfo"));
    chargingStatus_->setText(state==OrderInfo::StatusCharging?QStringLiteral("充电正常"):stateText);
    chargingStatus_->setObjectName(statusObject);chargingStatus_->style()->unpolish(chargingStatus_);chargingStatus_->style()->polish(chargingStatus_);
    chargingStateText_->setText(stateText);chargingStateText_->setObjectName(statusObject);chargingStateText_->style()->unpolish(chargingStateText_);chargingStateText_->style()->polish(chargingStateText_);

    if(!o.valid()){
        gauge_->setValue(0,QStringLiteral("—"),QStringLiteral("等待订单"));
        chargingStation_->setText(QStringLiteral("暂无订单"));chargingMetrics_->setText(QStringLiteral("请选择空闲充电桩"));
        chargingStart_->setText(QStringLiteral("开始 —"));chargingMode_->setText(QStringLiteral("模式 —"));
        chargingPowerLive_->setText(QStringLiteral("— kW"));chargingEnergyLive_->setText(QStringLiteral("— kWh"));
        chargingDuration_->setText(QStringLiteral("—"));chargingRemaining_->setText(QStringLiteral("—"));chargingFeeSummary_->setText(QStringLiteral("¥ —"));
        chargingEnergyTile_->setText(QStringLiteral("— kWh"));chargingPowerTile_->setText(QStringLiteral("— kW"));chargingDurationTile_->setText(QStringLiteral("—"));chargingFeeTile_->setText(QStringLiteral("¥ —"));chargingElectrical_->setText(QStringLiteral("电压、电流待服务端返回"));
        powerWave_->setSamples({});energyWave_->setSamples({});
        chargingAction_->setText(QStringLiteral("查找充电站"));chargingAction_->setObjectName(QStringLiteral("Primary"));chargingOrderButton_->setEnabled(false);chargingAction_->style()->unpolish(chargingAction_);chargingAction_->style()->polish(chargingAction_);return;
    }

    const double pct=o.progressPercent>=0?o.progressPercent:(o.targetEnergyKwh>0?o.energyKwh/o.targetEnergyKwh*100.0:0.0);
    const qint64 duration=o.durationMs();
    qint64 remaining=o.remainingSeconds;
    if(remaining<=0&&o.targetEnergyKwh>o.energyKwh&&o.powerKw>0){remaining=qint64((o.targetEnergyKwh-o.energyKwh)/o.powerKw*3600.0);}
    const double fee=state==OrderInfo::StatusWaitSettlement?o.amount:o.estimatedAmount;
    gauge_->setValue(pct,state==OrderInfo::StatusCharging?QStringLiteral("%1%").arg(qRound(pct)):stateText,state==OrderInfo::StatusCharging?QStringLiteral("极速充电中"):QStringLiteral("服务端订单状态"));
    chargingStation_->setText(o.stationName);chargingMetrics_->setText(QStringLiteral("设备编号 %1").arg(o.chargerCode));
    chargingStart_->setText(QStringLiteral("开始 %1").arg(o.startTimeMs>0?QDateTime::fromMSecsSinceEpoch(o.startTimeMs).toString(QStringLiteral("HH:mm")):QStringLiteral("—")));
    chargingMode_->setText(QStringLiteral("模式 %1").arg(o.chargerType==protocol::ChargerTypeFast?QStringLiteral("快充"):QStringLiteral("慢充")));
    chargingPowerLive_->setText(QStringLiteral("%1 kW").arg(o.powerKw,0,'f',0));chargingEnergyLive_->setText(QStringLiteral("%1 kWh").arg(o.energyKwh,0,'f',2));
    chargingDuration_->setText(clockDurationText(duration));chargingRemaining_->setText(remaining>0?clockDurationText(remaining*1000):QStringLiteral("—"));chargingFeeSummary_->setText(QStringLiteral("¥ %1").arg(fee,0,'f',2));
    chargingEnergyTile_->setText(QStringLiteral("%1 kWh").arg(o.energyKwh,0,'f',2));chargingPowerTile_->setText(QStringLiteral("%1 kW").arg(o.powerKw,0,'f',0));chargingDurationTile_->setText(clockDurationText(duration));chargingFeeTile_->setText(QStringLiteral("¥ %1").arg(fee,0,'f',2));
    chargingElectrical_->setText(o.voltageV>0&&o.currentA>0?QStringLiteral("电压 %1 V · 电流 %2 A").arg(o.voltageV,0,'f',0).arg(o.currentA,0,'f',0):QStringLiteral("电压、电流待服务端返回"));
    powerWave_->setSamples(o.powerTrend);energyWave_->setSamples(o.energyTrend);chargingOrderButton_->setEnabled(true);
    if(state==OrderInfo::StatusReserved)chargingAction_->setText(QStringLiteral("开始充电"));else if(state==OrderInfo::StatusCharging)chargingAction_->setText(QStringLiteral("停止充电"));else if(state==OrderInfo::StatusWaitSettlement)chargingAction_->setText(QStringLiteral("确认结算"));else chargingAction_->setText(QStringLiteral("查看附近站点"));chargingAction_->setObjectName(state==OrderInfo::StatusCharging?QStringLiteral("Danger"):QStringLiteral("Primary"));chargingAction_->style()->unpolish(chargingAction_);chargingAction_->style()->polish(chargingAction_);
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
