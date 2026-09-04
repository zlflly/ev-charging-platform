#ifndef ADMIN_REPOSITORY_H
#define ADMIN_REPOSITORY_H

#include <QString>
#include <optional>

namespace repository {

struct Admin {
    int adminId;
    QString account;
    QString displayName;
    qint64 createdAt;
};

/**
 * 管理员数据访问层
 */
class AdminRepository {
public:
    // 验证管理员账号密码
    static std::optional<Admin> authenticate(const QString& account, const QString& password);

    // 根据 adminId 查找
    static std::optional<Admin> findById(int adminId);
};

} // namespace repository

#endif // ADMIN_REPOSITORY_H
