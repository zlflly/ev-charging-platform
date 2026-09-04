#ifndef USER_REPOSITORY_H
#define USER_REPOSITORY_H

#include <QString>
#include <QVariant>
#include <optional>
#include <QDateTime>

namespace repository {

struct User {
    int userId;
    QString phone;
    QString nickname;
    QString avatar;  // data-URL 或空
    double balance;
    int status;  // 0=正常, 1=冻结
    qint64 createdAt;
    qint64 updatedAt;
};

/**
 * 用户数据访问层
 * 封装 users 表的 CRUD 操作
 */
class UserRepository {
public:
    // 根据手机号查找用户
    static std::optional<User> findByPhone(const QString& phone);

    // 根据 userId 查找用户
    static std::optional<User> findById(int userId);

    // 创建新用户（自动生成默认昵称）
    static std::optional<User> create(const QString& phone);

    // 更新昵称
    static bool updateNickname(int userId, const QString& nickname);

    // 更新头像
    static bool updateAvatar(int userId, const QString& avatarData);

    // 更新余额（增加或减少，使用事务）
    static bool updateBalance(int userId, double delta);

    // 设置余额（绝对值）
    static bool setBalance(int userId, double newBalance);

    // 冻结/解冻用户
    static bool setStatus(int userId, int status);

    // 查询所有用户（管理员用）
    static QVector<User> findAll();

    // 根据手机号模糊查询
    static QVector<User> searchByPhone(const QString& phonePattern);
};

} // namespace repository

#endif // USER_REPOSITORY_H
