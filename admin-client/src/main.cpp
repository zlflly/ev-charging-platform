#include "app/ApplicationController.h"
#include "ui/theme/Theme.h"

#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    QApplication::setApplicationName(QStringLiteral("EV Admin Client"));
    QApplication::setOrganizationName(QStringLiteral("EV Charging Platform"));
    application.setStyleSheet(theme::globalStyleSheet());

    ApplicationController controller;
    controller.start();
    return application.exec();
}
