#include "app/ApplicationController.h"

#include "api/AdminApiClient.h"
#include "net/NetworkClient.h"
#include "session/AdminSession.h"
#include "ui/LoginWindow.h"
#include "ui/MainWindow.h"

ApplicationController::ApplicationController(QObject* parent)
    : QObject(parent)
    , network_(new NetworkClient(this))
    , session_(new AdminSession(this))
    , api_(new AdminApiClient(network_, session_, this))
    , loginWindow_(new LoginWindow(api_))
    , mainWindow_(new MainWindow(network_, session_))
{
    connect(api_, &AdminApiClient::loginSucceeded,
            this, &ApplicationController::showMainWindow);
    connect(api_, &AdminApiClient::sessionExpired, this,
            [this](const QString& message) {
        showLogin(message);
    });
    connect(mainWindow_, &MainWindow::logoutRequested, this, [this] {
        api_->logout();
        showLogin(QStringLiteral("已安全退出管理员账号"));
    });
}

ApplicationController::~ApplicationController()
{
    delete mainWindow_;
    delete loginWindow_;
}

void ApplicationController::start()
{
    showLogin();
}

void ApplicationController::showLogin(const QString& message)
{
    mainWindow_->hide();
    loginWindow_->resetForLogin(message);
    loginWindow_->show();
    loginWindow_->raise();
    loginWindow_->activateWindow();
}

void ApplicationController::showMainWindow()
{
    loginWindow_->hide();
    mainWindow_->show();
    mainWindow_->raise();
    mainWindow_->activateWindow();
}
