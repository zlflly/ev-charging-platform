#pragma once

// ============================================================================
// 启动配置：从命令行参数和配置文件读取监听地址、端口、数据库路径等
// ============================================================================

#include <QString>
#include <QCommandLineParser>
#include <QCoreApplication>
#include "protocol/Protocol.h"

namespace config {

struct ServerConfig {
    QString host = protocol::kDefaultHost;
    quint16 port = protocol::kDefaultPort;
    QString databasePath = "./ev.db";

    // 并发配置
    int maxConnections = 1000;
    int threadPoolSize = 4;

    // 日志配置
    bool verboseLogging = false;
};

// 从命令行参数解析配置
inline ServerConfig parseCommandLine(const QCoreApplication& app)
{
    ServerConfig config;

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("EV Charging Platform Server - 东软电动汽车充电桩应用管理平台服务端"));
    parser.addHelpOption();
    parser.addVersionOption();

    // --host <address>
    QCommandLineOption hostOption(
        QStringList() << "H" << "host",
        QStringLiteral("监听地址（默认 0.0.0.0）"),
        QStringLiteral("address"),
        config.host);
    parser.addOption(hostOption);

    // --port <port>
    QCommandLineOption portOption(
        QStringList() << "p" << "port",
        QStringLiteral("监听端口（默认 9000）"),
        QStringLiteral("port"),
        QString::number(config.port));
    parser.addOption(portOption);

    // --database <path>
    QCommandLineOption dbOption(
        QStringList() << "d" << "database",
        QStringLiteral("SQLite 数据库路径（默认 ./ev.db）"),
        QStringLiteral("path"),
        config.databasePath);
    parser.addOption(dbOption);

    // --max-connections <n>
    QCommandLineOption maxConnOption(
        "max-connections",
        QStringLiteral("最大并发连接数（默认 1000）"),
        QStringLiteral("n"),
        QString::number(config.maxConnections));
    parser.addOption(maxConnOption);

    // --threads <n>
    QCommandLineOption threadsOption(
        "threads",
        QStringLiteral("工作线程池大小（默认 4）"),
        QStringLiteral("n"),
        QString::number(config.threadPoolSize));
    parser.addOption(threadsOption);

    // --verbose
    QCommandLineOption verboseOption(
        QStringList() << "v" << "verbose",
        QStringLiteral("详细日志输出"));
    parser.addOption(verboseOption);

    parser.process(app);

    // 解析参数
    config.host = parser.value(hostOption);
    config.port = static_cast<quint16>(parser.value(portOption).toUInt());
    config.databasePath = parser.value(dbOption);
    config.maxConnections = parser.value(maxConnOption).toInt();
    config.threadPoolSize = parser.value(threadsOption).toInt();
    config.verboseLogging = parser.isSet(verboseOption);

    return config;
}

// 打印配置摘要
inline void printConfig(const ServerConfig& config)
{
    qInfo("========================================");
    qInfo("  EV Charging Platform Server");
    qInfo("========================================");
    qInfo("监听地址:     %s:%u", qPrintable(config.host), config.port);
    qInfo("数据库路径:   %s", qPrintable(config.databasePath));
    qInfo("最大连接数:   %d", config.maxConnections);
    qInfo("工作线程数:   %d", config.threadPoolSize);
    qInfo("详细日志:     %s", config.verboseLogging ? "启用" : "关闭");
    qInfo("========================================");
}

} // namespace config
