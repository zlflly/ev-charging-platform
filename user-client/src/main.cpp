#include "net/NetworkClient.h"
#include "config/AppConfig.h"
#include "session/Session.h"
#include "ui/MobileApp.h"
#include "ui/theme/Theme.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QTimer>

namespace {
// WSL / 虚拟机里的 OpenGL 驱动会被 Chromium 判定为不可信并把 WebGL 加入黑名单，
// 而高德 JS API 2.0 只有 WebGL 一条渲染路径，被禁用后地图会在加载完成后立刻变成空白。
// 这里在创建 QApplication 之前放开黑名单；用户显式设置过同名变量时不覆盖。
void relaxWebEngineGpuBlocklist()
{
    static const char* const kChromiumFlagsEnvironment = "QTWEBENGINE_CHROMIUM_FLAGS";
    QByteArray flags = qgetenv(kChromiumFlagsEnvironment).trimmed();
    if (flags.contains("ignore-gpu-blocklist")) return;
    if (!flags.isEmpty()) flags.append(' ');
    flags.append("--ignore-gpu-blocklist");
    qputenv(kChromiumFlagsEnvironment, flags);
}

void loadLocalEnvironment()
{
    const QStringList candidates{
        QDir::current().filePath(QStringLiteral("local.env")),
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("../local.env"))
    };
    for (const QString& path : candidates) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
        while (!file.atEnd()) {
            const QByteArray line = file.readLine().trimmed();
            if (line.isEmpty() || line.startsWith('#')) continue;
            const qsizetype separator = line.indexOf('=');
            if (separator <= 0) continue;
            const QByteArray key = line.left(separator).trimmed();
            const QByteArray value = line.mid(separator + 1).trimmed();
            if (qEnvironmentVariableIsEmpty(key.constData())) qputenv(key, value);
        }
        break;
    }
}
}

int main(int argc, char* argv[])
{
    relaxWebEngineGpuBlocklist();
    QApplication application(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("ev-user-client"));
    QCoreApplication::setOrganizationName(QStringLiteral("ev-charging-platform"));
    loadLocalEnvironment();
    application.setStyleSheet(Theme::globalStyleSheet());

    QFont font(QStringLiteral("Noto Sans CJK SC"));
    font.setPixelSize(14);
    application.setFont(font);

    if (qEnvironmentVariableIsSet("EV_PREVIEW_AUTH")) {
        Session::instance().setUser(1, QStringLiteral("13800000000"),
            QStringLiteral("界面预览"), QString(), 128.50, QStringLiteral("ACTIVE"));
        Session::instance().setLocation(appConfig::kDefaultLocationLatitude,
            appConfig::kDefaultLocationLongitude,
            QString::fromUtf8(appConfig::kDefaultLocationLabel));
    }

    NetworkClient network;
    MobileApp window(&network);
    window.show();
    const QString capturePath = qEnvironmentVariable("EV_CAPTURE_PATH");
    if (!capturePath.isEmpty()) {
        QTimer::singleShot(500, &application, [&application, &window, capturePath] {
            window.grab().save(capturePath);
            application.quit();
        });
    }
    return application.exec();
}
