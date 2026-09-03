#include "ui/MainWindow.h"
#include "ui/theme/Theme.h"

#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    QApplication::setApplicationName(QStringLiteral("EV Admin Client"));
    QApplication::setOrganizationName(QStringLiteral("EV Charging Platform"));
    application.setStyleSheet(theme::globalStyleSheet());

    // Commit 0 先直接展示后台骨架用于验收；Commit 1 完成服务端认证后，
    // 入口将切换为 LoginWindow，登录成功才创建 MainWindow。
    MainWindow window;
    window.show();
    return application.exec();
}
