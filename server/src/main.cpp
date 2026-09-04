// ============================================================================
// EV Charging Platform Server
// 东软电动汽车充电桩应用管理平台 - 服务端主程序
//
// Commit 0 实现：配置读取、TCP 监听、PING 处理、冒烟测试可通过
// Commit 1 实现：数据库初始化、Repository 层
// ============================================================================

#include <QCoreApplication>
#include <QTimer>
#include <csignal>
#include "config/ServerConfig.h"
#include "net/TcpServer.h"
#include "net/RequestDispatcher.h"
#include "repository/Database.h"

// 优雅退出信号处理
static QCoreApplication* g_app = nullptr;

void signalHandler(int signal)
{
    Q_UNUSED(signal)
    qInfo() << "\n[Main] Received interrupt signal, shutting down...";
    if (g_app) {
        g_app->quit();
    }
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    g_app = &app;

    // 设置应用信息
    QCoreApplication::setApplicationName("ev-server");
    QCoreApplication::setApplicationVersion("0.1.0");
    QCoreApplication::setOrganizationName("Neusoft");

    // 注册信号处理（Ctrl+C 优雅退出）
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // 解析命令行配置
    config::ServerConfig config = config::parseCommandLine(app);
    config::printConfig(config);

    // 初始化数据库
    qInfo() << "[Main] Initializing database...";
    if (!repository::Database::instance().initialize(config.databasePath)) {
        qCritical() << "[Main] Database initialization failed:"
                    << repository::Database::instance().lastError();
        return 1;
    }
    qInfo() << "[Main] Database initialized successfully";

    // 创建请求分发器
    net::RequestDispatcher dispatcher;

    // 创建 TCP 服务器
    net::TcpServer server(config);

    // 连接信号：请求 → 分发器
    QObject::connect(&server, &net::TcpServer::requestReceived,
                     &dispatcher, &net::RequestDispatcher::dispatch);

    // 启动监听
    if (!server.start()) {
        qCritical() << "[Main] Server start failed, exiting";
        return 1;
    }

    qInfo() << "[Main] Server started successfully. Press Ctrl+C to stop.";

    // 定时打印统计信息（每 30 秒）
    QTimer statsTimer;
    QObject::connect(&statsTimer, &QTimer::timeout, [&server]() {
        qInfo() << "[Stats] Active connections:" << server.activeConnectionCount();
    });
    statsTimer.start(30000);

    // 进入事件循环
    int exitCode = app.exec();

    // 清理
    server.stop();
    repository::Database::instance().close();
    qInfo() << "[Main] Server stopped, exit code:" << exitCode;

    return exitCode;
}