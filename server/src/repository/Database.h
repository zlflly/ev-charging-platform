#ifndef DATABASE_H
#define DATABASE_H

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QVariant>
#include <memory>

namespace repository {

/**
 * 数据库连接管理器
 * 负责 SQLite 初始化、建表、种子数据和连接生命周期
 */
class Database {
public:
    static Database& instance();

    // 初始化数据库（打开连接、建表、插入种子数据）
    bool initialize(const QString& dbPath);

    // 关闭数据库连接
    void close();

    // 获取底层 QSqlDatabase（供 Repository 层使用）
    QSqlDatabase& db() { return m_db; }

    // 检查数据库是否已打开
    bool isOpen() const { return m_db.isOpen(); }

    // 获取最后一次错误
    QString lastError() const { return m_lastError; }

private:
    Database() = default;
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    // 建表（幂等）
    bool createTables();

    // 插入种子数据（可重复执行）
    bool insertSeedData();

    QSqlDatabase m_db;
    QString m_lastError;
};

} // namespace repository

#endif // DATABASE_H
