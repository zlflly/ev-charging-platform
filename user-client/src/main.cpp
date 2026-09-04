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
