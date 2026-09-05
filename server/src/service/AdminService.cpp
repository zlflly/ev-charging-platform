#include "AdminService.h"
#include "net/SessionManager.h"
#include "protocol/Protocol.h"
#include "protocol/ProtocolHelper.h"
#include "repository/AdminRepository.h"
#include "repository/ChargerRepository.h"
#include "repository/Database.h"
#include "repository/StationRepository.h"
#include "repository/UserRepository.h"
#include "repository/OrderRepository.h"

#include <QDebug>
#include <QDateTime>
#include <QJsonArray>
#include <QSqlError>
#include <QSqlQuery>
#include <QRegularExpression>

namespace service {

AdminService::AdminService(QObject* parent)
    : QObject(parent)
{
}

// ============================================================================
// admin.login：管理员登录
// ============================================================================

QJsonObject AdminService::handleLogin(const QJsonObject& data, QTcpSocket* socket)
{
    QString account = data.value(QStringLiteral("account")).toString();
    QString password = data.value(QStringLiteral("password")).toString();

    if (account.isEmpty() || password.isEmpty()) {
        return protocol::makeErrorResponse(
            protocol::CodeBadRequest,
            QStringLiteral("账号和密码不能为空"));
    }

    auto admin = repository::AdminRepository::authenticate(account, password);
    if (!admin) {
        return protocol::makeErrorResponse(
            protocol::CodeAdminAuthFailed,
            QStringLiteral("管理员账号或密码错误"));
    }

    // 绑定管理员会话
    net::SessionManager::instance().bindAdmin(socket, admin->adminId);

    QJsonObject result;
    result.insert(QStringLiteral("adminId"), admin->adminId);
    result.insert(QStringLiteral("account"), admin->account);
    result.insert(QStringLiteral("displayName"), admin->displayName);

    qInfo() << "[AdminService] Admin" << admin->adminId << "logged in";

    return protocol::makeSuccessResponse(result);
}

// ============================================================================
// admin.charger.overview：充电桩状态总览
// ============================================================================

QJsonObject AdminService::handleChargerOverview(const QJsonObject& data, QTcpSocket* socket)
{
    Q_UNUSED(data);

    // 检查管理员登录
    if (!net::SessionManager::instance().isAdminLoggedIn(socket)) {
        return protocol::makeErrorResponse(
            protocol::CodeNotLoggedIn,
            QStringLiteral("请先登录"));
    }

    // 聚合统计各状态桩数量
    auto allChargers = repository::ChargerRepository::findAll();

    int total = allChargers.size();
    int idle = 0;
    int charging = 0;
    int fault = 0;
    int offline = 0;

    for (const auto& charger : allChargers) {
        switch (charger.status) {
        case protocol::ChargerStatusIdle:
            ++idle;
            break;
        case protocol::ChargerStatusCharging:
            ++charging;
            break;
        case protocol::ChargerStatusFault:
            ++fault;
            break;
        case protocol::ChargerStatusOffline:
            ++offline;
            break;
        }
    }

    // 计算百分比（避免除零）
    double idlePercent = 0.0;
    double chargingPercent = 0.0;
    double faultPercent = 0.0;
    double offlinePercent = 0.0;

    if (total > 0) {
        idlePercent = static_cast<double>(idle) / total * 100.0;
        chargingPercent = static_cast<double>(charging) / total * 100.0;
        faultPercent = static_cast<double>(fault) / total * 100.0;
        offlinePercent = static_cast<double>(offline) / total * 100.0;
    }

    QJsonObject result;
    result.insert(QStringLiteral("total"), total);
    result.insert(QStringLiteral("idle"), idle);
    result.insert(QStringLiteral("charging"), charging);
    result.insert(QStringLiteral("fault"), fault);
    result.insert(QStringLiteral("offline"), offline);
    result.insert(QStringLiteral("idlePercent"), idlePercent);
    result.insert(QStringLiteral("chargingPercent"), chargingPercent);
    result.insert(QStringLiteral("faultPercent"), faultPercent);
    result.insert(QStringLiteral("offlinePercent"), offlinePercent);
    result.insert(QStringLiteral("updatedAt"), QDateTime::currentMSecsSinceEpoch());

    return protocol::makeSuccessResponse(result);
}

// ============================================================================
// admin.chargers.list：充电桩管理列表
// ============================================================================

QJsonObject AdminService::handleChargersList(const QJsonObject& data, QTcpSocket* socket)
{
    Q_UNUSED(data);

    // 检查管理员登录
    if (!net::SessionManager::instance().isAdminLoggedIn(socket)) {
        return protocol::makeErrorResponse(
            protocol::CodeNotLoggedIn,
            QStringLiteral("请先登录"));
    }

    auto allChargers = repository::ChargerRepository::findAll();
    QJsonArray chargersArray;

    for (const auto& charger : allChargers) {
        // 关联查询站点名称
        auto station = repository::StationRepository::findById(charger.stationId);
        QString stationName = station ? station->name : QStringLiteral("未知站点");

        // 统计该桩累计数据
        auto stats = repository::OrderRepository::getChargerStats(charger.chargerId);

        QJsonObject chargerObj;
        chargerObj.insert(QStringLiteral("chargerId"), charger.chargerId);
        chargerObj.insert(QStringLiteral("code"), charger.code);
        chargerObj.insert(QStringLiteral("stationId"), charger.stationId);
        chargerObj.insert(QStringLiteral("stationName"), stationName);
        chargerObj.insert(QStringLiteral("type"), charger.type);
        chargerObj.insert(QStringLiteral("powerKw"), charger.powerKw);
        chargerObj.insert(QStringLiteral("status"), charger.status);
        chargerObj.insert(QStringLiteral("totalChargeCount"), stats.totalCount);
        chargerObj.insert(QStringLiteral("totalChargeDurationSeconds"), stats.totalDurationSeconds);

        chargersArray.append(chargerObj);
    }

    QJsonObject result;
    result.insert(QStringLiteral("chargers"), chargersArray);

    return protocol::makeSuccessResponse(result);
}

// ============================================================================
// admin.chargers.restart：充电桩远程重启
// ============================================================================

QJsonObject AdminService::handleChargersRestart(const QJsonObject& data, QTcpSocket* socket)
{
    // 检查管理员登录
    if (!net::SessionManager::instance().isAdminLoggedIn(socket)) {
        return protocol::makeErrorResponse(
            protocol::CodeNotLoggedIn,
            QStringLiteral("请先登录"));
    }

    int chargerId = data.value(QStringLiteral("chargerId")).toInt();
    if (chargerId <= 0) {
        return protocol::makeErrorResponse(
            protocol::CodeBadRequest,
            QStringLiteral("chargerId 无效"));
    }

    // 查询充电桩
    auto charger = repository::ChargerRepository::findById(chargerId);
    if (!charger) {
        return protocol::makeErrorResponse(
            protocol::CodeBadRequest,
            QStringLiteral("充电桩不存在"));
    }

    // 检查充电桩状态：status=1（充电中）不能重启
    if (charger->status == protocol::ChargerStatusCharging) {
        return protocol::makeErrorResponse(
            2101,  // CodeChargerOperationRejected
            QStringLiteral("充电桩正在服务订单，禁止重启操作"));
    }

    // 模拟重启成功（实际硬件结果由外部系统决定）
    qInfo() << "[AdminService] Charger" << chargerId << "restart accepted (simulated)";

    QJsonObject result;
    result.insert(QStringLiteral("chargerId"), chargerId);
    result.insert(QStringLiteral("restartedAt"), QDateTime::currentMSecsSinceEpoch());

    return protocol::makeSuccessResponse(result);
}

// ============================================================================
// admin.chargers.status.update：充电桩运维状态变更
// ============================================================================

QJsonObject AdminService::handleChargersStatusUpdate(const QJsonObject& data, QTcpSocket* socket)
{
    // 检查管理员登录
    if (!net::SessionManager::instance().isAdminLoggedIn(socket)) {
        return protocol::makeErrorResponse(
            protocol::CodeNotLoggedIn,
            QStringLiteral("请先登录"));
    }

    int chargerId = data.value(QStringLiteral("chargerId")).toInt();
    int expectedStatus = data.value(QStringLiteral("expectedStatus")).toInt();
    int targetStatus = data.value(QStringLiteral("targetStatus")).toInt();
    QString reason = data.value(QStringLiteral("reason")).toString().trimmed();

    // 参数校验
    if (chargerId <= 0) {
        return protocol::makeErrorResponse(
            protocol::CodeBadRequest,
            QStringLiteral("chargerId 无效"));
    }

    // targetStatus 只允许 0/2/3（空闲/故障/离线），不允许手动设为 1（在用）
    if (targetStatus != protocol::ChargerStatusIdle
        && targetStatus != protocol::ChargerStatusFault
        && targetStatus != protocol::ChargerStatusOffline) {
        return protocol::makeErrorResponse(
            protocol::CodeBadRequest,
            QStringLiteral("targetStatus 只能为 0(空闲)/2(故障)/3(离线)"));
    }

    // 原因长度校验
    if (reason.length() < 2 || reason.length() > 200) {
        return protocol::makeErrorResponse(
            protocol::CodeBadRequest,
            QStringLiteral("原因长度必须在 2-200 字符之间"));
    }

    // 查询充电桩
    auto charger = repository::ChargerRepository::findById(chargerId);
    if (!charger) {
        return protocol::makeErrorResponse(
            protocol::CodeBadRequest,
            QStringLiteral("充电桩不存在"));
    }

    // 检查 expectedStatus 是否匹配（防止并发冲突）
    if (charger->status != expectedStatus) {
        return protocol::makeErrorResponse(
            2103,  // CodeChargerStateConflict
            QStringLiteral("充电桩状态已变化，请刷新后重试"));
    }

    // 如果当前状态为 1（充电中），拒绝变更
    if (charger->status == protocol::ChargerStatusCharging) {
        return protocol::makeErrorResponse(
            2101,  // CodeChargerOperationRejected
            QStringLiteral("充电桩正在服务订单，禁止状态变更"));
    }

    // 检查是否有未完成订单（双重保险）
    auto activeOrder = repository::OrderRepository::findActiveOrderByCharger(chargerId);
    if (activeOrder) {
        return protocol::makeErrorResponse(
            2101,
            QStringLiteral("充电桩关联有未完成订单，禁止状态变更"));
    }

    // 相同状态无效变更
    if (charger->status == targetStatus) {
        return protocol::makeErrorResponse(
            protocol::CodeBadRequest,
            QStringLiteral("当前状态与目标状态相同，无需变更"));
    }

    // 更新状态
    int previousStatus = charger->status;
    if (!repository::ChargerRepository::updateStatus(chargerId, targetStatus)) {
        return protocol::makeErrorResponse(
            protocol::CodeServerError,
            QStringLiteral("状态更新失败"));
    }

    qInfo() << "[AdminService] Charger" << chargerId << "status changed from"
            << previousStatus << "to" << targetStatus << "reason:" << reason;

    QJsonObject result;
    result.insert(QStringLiteral("chargerId"), chargerId);
    result.insert(QStringLiteral("previousStatus"), previousStatus);
    result.insert(QStringLiteral("status"), targetStatus);
    result.insert(QStringLiteral("changedAt"), QDateTime::currentMSecsSinceEpoch());

    return protocol::makeSuccessResponse(result);
}

// ============================================================================
// admin.stations.list：充电站列表
// ============================================================================

QJsonObject AdminService::handleStationsList(const QJsonObject& data, QTcpSocket* socket)
{
    Q_UNUSED(data);

    // 检查管理员登录
    if (!net::SessionManager::instance().isAdminLoggedIn(socket)) {
        return protocol::makeErrorResponse(
            protocol::CodeNotLoggedIn,
            QStringLiteral("请先登录"));
    }

    auto allStations = repository::StationRepository::findAll();
    QJsonArray stationsArray;

    for (const auto& station : allStations) {
        // 统计该站点的充电桩数量和在线率
        auto chargers = repository::ChargerRepository::findByStationId(station.stationId);

        int totalCount = chargers.size();
        int onlineCount = 0;

        for (const auto& charger : chargers) {
            // 在线 = idle + charging + fault（只有 offline 不算在线）
            if (charger.status != protocol::ChargerStatusOffline) {
                ++onlineCount;
            }
        }

        double onlineRate = 0.0;
        if (totalCount > 0) {
            onlineRate = static_cast<double>(onlineCount) / totalCount * 100.0;
        }

        QJsonObject stationObj;
        stationObj.insert(QStringLiteral("stationId"), station.stationId);
        stationObj.insert(QStringLiteral("name"), station.name);
        stationObj.insert(QStringLiteral("address"), station.address);
        stationObj.insert(QStringLiteral("latitude"), station.latitude);
        stationObj.insert(QStringLiteral("longitude"), station.longitude);
        stationObj.insert(QStringLiteral("pricePerKwh"), station.pricePerKwh);
        stationObj.insert(QStringLiteral("totalCount"), totalCount);
        stationObj.insert(QStringLiteral("onlineRate"), onlineRate);
        stationObj.insert(QStringLiteral("version"), station.version);

        stationsArray.append(stationObj);
    }

    QJsonObject result;
    result.insert(QStringLiteral("stations"), stationsArray);

    return protocol::makeSuccessResponse(result);
}

// ============================================================================
// admin.stations.create：新增充电站
// ============================================================================

QJsonObject AdminService::handleStationsCreate(const QJsonObject& data, QTcpSocket* socket)
{
    // 检查管理员登录
    if (!net::SessionManager::instance().isAdminLoggedIn(socket)) {
        return protocol::makeErrorResponse(
            protocol::CodeNotLoggedIn,
            QStringLiteral("请先登录"));
    }

    QString name = data.value(QStringLiteral("name")).toString().trimmed();
    QString address = data.value(QStringLiteral("address")).toString().trimmed();
    double latitude = data.value(QStringLiteral("latitude")).toDouble();
    double longitude = data.value(QStringLiteral("longitude")).toDouble();
    double pricePerKwh = data.value(QStringLiteral("pricePerKwh")).toDouble();
    int chargerCount = data.value(QStringLiteral("chargerCount")).toInt();

    // 参数校验
    if (name.length() < 1 || name.length() > 60) {
        return protocol::makeErrorResponse(
            protocol::CodeBadRequest,
            QStringLiteral("站点名称长度必须在 1-60 字符之间"));
    }

    if (address.length() < 1 || address.length() > 200) {
        return protocol::makeErrorResponse(
            protocol::CodeBadRequest,
            QStringLiteral("站点地址长度必须在 1-200 字符之间"));
    }

    if (latitude < -90.0 || latitude > 90.0) {
        return protocol::makeErrorResponse(
            protocol::CodeBadRequest,
            QStringLiteral("纬度范围必须在 -90 到 90 之间"));
    }

    if (longitude < -180.0 || longitude > 180.0) {
        return protocol::makeErrorResponse(
            protocol::CodeBadRequest,
            QStringLiteral("经度范围必须在 -180 到 180 之间"));
    }

    if (pricePerKwh <= 0.0 || !qIsFinite(pricePerKwh)) {
        return protocol::makeErrorResponse(
            protocol::CodeBadRequest,
            QStringLiteral("电价必须为有限正数"));
    }

    if (chargerCount < 0 || chargerCount > 100) {
        return protocol::makeErrorResponse(
            protocol::CodeBadRequest,
            QStringLiteral("充电桩数量必须在 0-100 之间"));
    }

    // 创建站点（事务保护）
    repository::Station newStation;
    newStation.name = name;
    newStation.address = address;
    newStation.latitude = latitude;
    newStation.longitude = longitude;
    newStation.pricePerKwh = pricePerKwh;

    int stationId = repository::StationRepository::create(newStation);
    if (stationId <= 0) {
        return protocol::makeErrorResponse(
            protocol::CodeServerError,
            QStringLiteral("站点创建失败"));
    }

    // 创建初始充电桩
    int createdCount = 0;
    for (int i = 0; i < chargerCount; ++i) {
        repository::Charger newCharger;
        newCharger.stationId = stationId;
        newCharger.code = QStringLiteral("CP-%1").arg(stationId * 100 + i + 1, 3, 10, QChar('0'));
        newCharger.type = (i % 2 == 0) ? protocol::ChargerTypeFast : protocol::ChargerTypeSlow;
        newCharger.powerKw = (newCharger.type == protocol::ChargerTypeFast) ? 120.0 : 7.0;
        newCharger.status = protocol::ChargerStatusIdle;

        if (repository::ChargerRepository::create(newCharger) > 0) {
            ++createdCount;
        }
    }

    qInfo() << "[AdminService] Created station" << stationId << "with" << createdCount << "chargers";

    QJsonObject result;
    result.insert(QStringLiteral("stationId"), stationId);
    result.insert(QStringLiteral("createdChargerCount"), createdCount);

    return protocol::makeSuccessResponse(result);
}

// ============================================================================
// admin.stations.update：编辑充电站资料
// ============================================================================

QJsonObject AdminService::handleStationsUpdate(const QJsonObject& data, QTcpSocket* socket)
{
    // 检查管理员登录
    if (!net::SessionManager::instance().isAdminLoggedIn(socket)) {
        return protocol::makeErrorResponse(
            protocol::CodeNotLoggedIn,
            QStringLiteral("请先登录"));
    }

    int stationId = data.value(QStringLiteral("stationId")).toInt();
    int expectedVersion = data.value(QStringLiteral("expectedVersion")).toInt();
    QString name = data.value(QStringLiteral("name")).toString().trimmed();
    QString address = data.value(QStringLiteral("address")).toString().trimmed();
    double latitude = data.value(QStringLiteral("latitude")).toDouble();
    double longitude = data.value(QStringLiteral("longitude")).toDouble();
    double pricePerKwh = data.value(QStringLiteral("pricePerKwh")).toDouble();

    // 参数校验
    if (stationId <= 0) {
        return protocol::makeErrorResponse(
            protocol::CodeBadRequest,
            QStringLiteral("stationId 无效"));
    }

    if (name.length() < 1 || name.length() > 60) {
        return protocol::makeErrorResponse(
            protocol::CodeBadRequest,
            QStringLiteral("站点名称长度必须在 1-60 字符之间"));
    }

    if (address.length() < 1 || address.length() > 200) {
        return protocol::makeErrorResponse(
            protocol::CodeBadRequest,
            QStringLiteral("站点地址长度必须在 1-200 字符之间"));
    }

    if (latitude < -90.0 || latitude > 90.0) {
        return protocol::makeErrorResponse(
            protocol::CodeBadRequest,
            QStringLiteral("纬度范围必须在 -90 到 90 之间"));
    }

    if (longitude < -180.0 || longitude > 180.0) {
        return protocol::makeErrorResponse(
            protocol::CodeBadRequest,
            QStringLiteral("经度范围必须在 -180 到 180 之间"));
    }

    if (pricePerKwh <= 0.0 || !qIsFinite(pricePerKwh)) {
        return protocol::makeErrorResponse(
            protocol::CodeBadRequest,
            QStringLiteral("电价必须为有限正数"));
    }

    // 查询站点
    auto station = repository::StationRepository::findById(stationId);
    if (!station) {
        return protocol::makeErrorResponse(
            protocol::CodeBadRequest,
            QStringLiteral("充电站不存在"));
    }

    // 检查版本（防止并发冲突）
    if (station->version != expectedVersion) {
        return protocol::makeErrorResponse(
            2102,  // CodeStationVersionConflict
            QStringLiteral("站点资料已被其他管理员修改，请刷新后重试"));
    }

    // 更新站点
    repository::Station updatedStation = *station;
    updatedStation.name = name;
    updatedStation.address = address;
    updatedStation.latitude = latitude;
    updatedStation.longitude = longitude;
    updatedStation.pricePerKwh = pricePerKwh;

    if (!repository::StationRepository::update(updatedStation)) {
        return protocol::makeErrorResponse(
            protocol::CodeServerError,
            QStringLiteral("站点更新失败"));
    }

    qInfo() << "[AdminService] Updated station" << stationId;

    QJsonObject result;
    result.insert(QStringLiteral("stationId"), stationId);
    result.insert(QStringLiteral("version"), updatedStation.version);
    result.insert(QStringLiteral("updatedAt"), QDateTime::currentMSecsSinceEpoch());

    return protocol::makeSuccessResponse(result);
}

// ============================================================================
// admin.users.list：用户列表
// ============================================================================

QJsonObject AdminService::handleUsersList(const QJsonObject& data, QTcpSocket* socket)
{
    // 检查管理员登录
    if (!net::SessionManager::instance().isAdminLoggedIn(socket)) {
        return protocol::makeErrorResponse(
            protocol::CodeNotLoggedIn,
            QStringLiteral("请先登录"));
    }

    int page = data.value(QStringLiteral("page")).toInt(1);
    int pageSize = data.value(QStringLiteral("pageSize")).toInt(20);
    QString phoneKeyword = data.value(QStringLiteral("phoneKeyword")).toString().trimmed();
    int status = data.value(QStringLiteral("status")).toInt(-1);
    QString activityFilter = data.value(QStringLiteral("activityFilter")).toString();

    // 参数校验
    if (page < 1) page = 1;
    if (pageSize < 1 || pageSize > 100) {
        return protocol::makeErrorResponse(
            protocol::CodeBadRequest,
            QStringLiteral("pageSize 必须在 1-100 之间"));
    }

    if (phoneKeyword.length() > 11) {
        return protocol::makeErrorResponse(
            protocol::CodeBadRequest,
            QStringLiteral("phoneKeyword 最多 11 位"));
    }

    if (activityFilter != QStringLiteral("ALL")
        && activityFilter != QStringLiteral("ACTIVE")
        && activityFilter != QStringLiteral("IDLE")) {
        return protocol::makeErrorResponse(
            protocol::CodeBadRequest,
            QStringLiteral("activityFilter 必须为 ALL/ACTIVE/IDLE"));
    }

    // 构建查询（简化版：先查全部用户，客户端过滤）
    // 生产环境应在SQL中实现分页和筛选
    auto allUsers = repository::UserRepository::findAll();
    QJsonArray usersArray;
    int total = 0;

    for (const auto& user : allUsers) {
        // 筛选：手机号关键词
        if (!phoneKeyword.isEmpty() && !user.phone.contains(phoneKeyword)) {
            continue;
        }

        // 筛选：用户状态
        if (status >= 0 && user.status != status) {
            continue;
        }

        // 查询活跃订单
        auto activeOrder = repository::OrderRepository::findActiveByUser(user.userId);
        QString activityStatus = QStringLiteral("IDLE");
        if (activeOrder) {
            activityStatus = activeOrder->status;
        }

        // 筛选：活跃状态
        if (activityFilter == QStringLiteral("ACTIVE") && !activeOrder) {
            continue;
        }
        if (activityFilter == QStringLiteral("IDLE") && activeOrder) {
            continue;
        }

        ++total;

        // 分页
        if (total <= (page - 1) * pageSize) {
            continue;
        }
        if (usersArray.size() >= pageSize) {
            continue;
        }

        QJsonObject userObj;
        userObj.insert(QStringLiteral("userId"), user.userId);
        userObj.insert(QStringLiteral("phone"), user.phone);
        userObj.insert(QStringLiteral("nickname"), user.nickname);
        userObj.insert(QStringLiteral("balance"), user.balance);
        userObj.insert(QStringLiteral("createdAt"), user.createdAt);
        userObj.insert(QStringLiteral("status"), user.status);
        userObj.insert(QStringLiteral("activityStatus"), activityStatus);

        if (activeOrder) {
            auto station = repository::StationRepository::findById(activeOrder->stationId);
            auto charger = repository::ChargerRepository::findById(activeOrder->chargerId);

            QJsonObject activeOrderObj;
            activeOrderObj.insert(QStringLiteral("orderId"), activeOrder->orderId);
            activeOrderObj.insert(QStringLiteral("status"), activeOrder->status);
            activeOrderObj.insert(QStringLiteral("stationId"), activeOrder->stationId);
            activeOrderObj.insert(QStringLiteral("stationName"),
                                station ? station->name : QStringLiteral("未知站点"));
            activeOrderObj.insert(QStringLiteral("chargerId"), activeOrder->chargerId);
            activeOrderObj.insert(QStringLiteral("chargerCode"),
                                charger ? charger->code : QStringLiteral(""));

            userObj.insert(QStringLiteral("activeOrder"), activeOrderObj);
        } else {
            userObj.insert(QStringLiteral("activeOrder"), QJsonValue::Null);
        }

        usersArray.append(userObj);
    }

    QJsonObject result;
    result.insert(QStringLiteral("users"), usersArray);
    result.insert(QStringLiteral("total"), total);
    result.insert(QStringLiteral("page"), page);
    result.insert(QStringLiteral("pageSize"), pageSize);

    return protocol::makeSuccessResponse(result);
}

// ============================================================================
// admin.users.freeze：冻结/解冻用户
// ============================================================================

QJsonObject AdminService::handleUsersFreeze(const QJsonObject& data, QTcpSocket* socket)
{
    // 检查管理员登录
    if (!net::SessionManager::instance().isAdminLoggedIn(socket)) {
        return protocol::makeErrorResponse(
            protocol::CodeNotLoggedIn,
            QStringLiteral("请先登录"));
    }

    int userId = data.value(QStringLiteral("userId")).toInt();
    int expectedStatus = data.value(QStringLiteral("expectedStatus")).toInt();
    int targetStatus = data.value(QStringLiteral("targetStatus")).toInt();
    QString reason = data.value(QStringLiteral("reason")).toString().trimmed();

    // 参数校验
    if (userId <= 0) {
        return protocol::makeErrorResponse(
            protocol::CodeBadRequest,
            QStringLiteral("userId 无效"));
    }

    if (targetStatus != 0 && targetStatus != 1) {
        return protocol::makeErrorResponse(
            protocol::CodeBadRequest,
            QStringLiteral("targetStatus 只能为 0(正常)/1(冻结)"));
    }

    if (reason.length() < 2 || reason.length() > 200) {
        return protocol::makeErrorResponse(
            protocol::CodeBadRequest,
            QStringLiteral("原因长度必须在 2-200 字符之间"));
    }

    // 查询用户
    auto user = repository::UserRepository::findById(userId);
    if (!user) {
        return protocol::makeErrorResponse(
            protocol::CodeBadRequest,
            QStringLiteral("用户不存在"));
    }

    // 检查 expectedStatus（防止并发冲突）
    if (user->status != expectedStatus) {
        return protocol::makeErrorResponse(
            2104,  // CodeUserStateConflict
            QStringLiteral("用户状态已变化，请刷新后重试"));
    }

    // 相同状态无效变更
    if (user->status == targetStatus) {
        return protocol::makeErrorResponse(
            protocol::CodeBadRequest,
            QStringLiteral("当前状态与目标状态相同，无需变更"));
    }

    // 检查是否有未完成订单（安全策略：有活跃订单时拒绝冻结）
    auto activeOrder = repository::OrderRepository::findActiveByUser(userId);
    if (activeOrder && targetStatus == 1) {
        return protocol::makeErrorResponse(
            protocol::CodeOrderConflict,
            QStringLiteral("用户有未完成订单，禁止冻结操作"));
    }

    // 更新状态
    int previousStatus = user->status;
    if (!repository::UserRepository::updateStatus(userId, targetStatus)) {
        return protocol::makeErrorResponse(
            protocol::CodeServerError,
            QStringLiteral("状态更新失败"));
    }

    qInfo() << "[AdminService] User" << userId << "status changed from"
            << previousStatus << "to" << targetStatus << "reason:" << reason;

    QJsonObject result;
    result.insert(QStringLiteral("userId"), userId);
    result.insert(QStringLiteral("previousStatus"), previousStatus);
    result.insert(QStringLiteral("status"), targetStatus);
    result.insert(QStringLiteral("changedAt"), QDateTime::currentMSecsSinceEpoch());

    return protocol::makeSuccessResponse(result);
}

// ============================================================================
// admin.revenue.summary：营收汇总
// ============================================================================

QJsonObject AdminService::handleRevenueSummary(const QJsonObject& data, QTcpSocket* socket)
{
    Q_UNUSED(data);

    // 检查管理员登录
    if (!net::SessionManager::instance().isAdminLoggedIn(socket)) {
        return protocol::makeErrorResponse(
            protocol::CodeNotLoggedIn,
            QStringLiteral("请先登录"));
    }

    // 获取所有已完成订单
    auto finishedOrders = repository::OrderRepository::findFinished();

    // 计算今日、本月、总计营收
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    QDateTime nowDt = QDateTime::fromMSecsSinceEpoch(now);
    QDateTime todayStart = QDateTime(nowDt.date(), QTime(0, 0, 0));
    QDateTime monthStart = QDateTime(QDate(nowDt.date().year(), nowDt.date().month(), 1), QTime(0, 0, 0));

    qint64 todayStartMs = todayStart.toMSecsSinceEpoch();
    qint64 monthStartMs = monthStart.toMSecsSinceEpoch();

    double todayRevenue = 0.0;
    double monthRevenue = 0.0;
    double totalRevenue = 0.0;

    for (const auto& order : finishedOrders) {
        totalRevenue += order.amount;

        if (order.settleTime >= monthStartMs) {
            monthRevenue += order.amount;
        }

        if (order.settleTime >= todayStartMs) {
            todayRevenue += order.amount;
        }
    }

    QJsonObject result;
    result.insert(QStringLiteral("todayRevenue"), todayRevenue);
    result.insert(QStringLiteral("monthRevenue"), monthRevenue);
    result.insert(QStringLiteral("totalRevenue"), totalRevenue);
    result.insert(QStringLiteral("currency"), QStringLiteral("CNY"));
    result.insert(QStringLiteral("timezone"), QStringLiteral("Asia/Shanghai"));
    result.insert(QStringLiteral("generatedAt"), nowDt.toString(Qt::ISODate));

    return protocol::makeSuccessResponse(result);
}

// ============================================================================
// admin.revenue.trend：营收趋势
// ============================================================================

QJsonObject AdminService::handleRevenueTrend(const QJsonObject& data, QTcpSocket* socket)
{
    // 检查管理员登录
    if (!net::SessionManager::instance().isAdminLoggedIn(socket)) {
        return protocol::makeErrorResponse(
            protocol::CodeNotLoggedIn,
            QStringLiteral("请先登录"));
    }

    int days = data.value(QStringLiteral("days")).toInt();

    // 参数校验
    if (days != 7 && days != 30) {
        return protocol::makeErrorResponse(
            protocol::CodeBadRequest,
            QStringLiteral("days 只能为 7 或 30"));
    }

    // 获取所有已完成订单
    auto finishedOrders = repository::OrderRepository::findFinished();

    // 计算每日营收
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    QDateTime nowDt = QDateTime::fromMSecsSinceEpoch(now);
    QDate today = nowDt.date();

    QMap<QString, double> dailyRevenue;

    // 初始化所有日期为 0.0
    for (int i = days - 1; i >= 0; --i) {
        QDate date = today.addDays(-i);
        dailyRevenue[date.toString(Qt::ISODate)] = 0.0;
    }

    // 累计每日营收
    for (const auto& order : finishedOrders) {
        QDateTime settleDt = QDateTime::fromMSecsSinceEpoch(order.settleTime);
        QDate settleDate = settleDt.date();
        QString dateStr = settleDate.toString(Qt::ISODate);

        if (dailyRevenue.contains(dateStr)) {
            dailyRevenue[dateStr] += order.amount;
        }
    }

    // 构建 points 数组
    QJsonArray pointsArray;
    for (int i = days - 1; i >= 0; --i) {
        QDate date = today.addDays(-i);
        QString dateStr = date.toString(Qt::ISODate);

        QJsonObject point;
        point.insert(QStringLiteral("date"), dateStr);
        point.insert(QStringLiteral("revenue"), dailyRevenue[dateStr]);

        pointsArray.append(point);
    }

    QJsonObject result;
    result.insert(QStringLiteral("days"), days);
    result.insert(QStringLiteral("timezone"), QStringLiteral("Asia/Shanghai"));
    result.insert(QStringLiteral("generatedAt"), nowDt.toString(Qt::ISODate));
    result.insert(QStringLiteral("points"), pointsArray);

    return protocol::makeSuccessResponse(result);
}

// ============================================================================
// admin.orders.list：订单列表
// ============================================================================

QJsonObject AdminService::handleOrdersList(const QJsonObject& data, QTcpSocket* socket)
{
    // 检查管理员登录
    if (!net::SessionManager::instance().isAdminLoggedIn(socket)) {
        return protocol::makeErrorResponse(
            protocol::CodeNotLoggedIn,
            QStringLiteral("请先登录"));
    }

    int page = data.value(QStringLiteral("page")).toInt(1);
    int pageSize = data.value(QStringLiteral("pageSize")).toInt(20);
    QString keyword = data.value(QStringLiteral("keyword")).toString().trimmed();
    QString statusFilter = data.value(QStringLiteral("status")).toString();
    QString paymentStatusFilter = data.value(QStringLiteral("paymentStatus")).toString();

    // 参数校验
    if (page < 1) page = 1;
    if (pageSize < 1 || pageSize > 100) {
        return protocol::makeErrorResponse(
            protocol::CodeBadRequest,
            QStringLiteral("pageSize 必须在 1-100 之间"));
    }

    if (keyword.length() > 50) {
        return protocol::makeErrorResponse(
            protocol::CodeBadRequest,
            QStringLiteral("keyword 最多 50 字符"));
    }

    // 查询所有订单（简化版：客户端过滤）
    // 生产环境应在SQL中实现高效筛选和分页
    QSqlQuery query(repository::Database::instance().db());
    QString sql = R"(
        SELECT o.orderId, o.userId, o.chargerId, o.stationId, o.status,
               o.startTime, o.stopTime, o.settleTime, o.duration, o.energyKwh, o.amount, o.createdAt,
               u.phone, u.nickname,
               s.name as stationName, s.pricePerKwh,
               c.code as chargerCode, c.type, c.powerKw
        FROM orders o
        JOIN users u ON o.userId = u.userId
        JOIN stations s ON o.stationId = s.stationId
        JOIN chargers c ON o.chargerId = c.chargerId
        ORDER BY o.createdAt DESC
    )";

    if (!query.exec(sql)) {
        qWarning() << "Failed to query orders:" << query.lastError().text();
        return protocol::makeErrorResponse(
            protocol::CodeServerError,
            QStringLiteral("订单查询失败"));
    }

    QJsonArray ordersArray;
    int total = 0;
    int reservedCount = 0;
    int chargingCount = 0;
    int waitSettlementCount = 0;
    int finishedCount = 0;
    double paidAmount = 0.0;

    while (query.next()) {
        QString status = query.value(4).toString();

        // 统计全平台数据
        if (status == protocol::kOrderStatusReserved) ++reservedCount;
        else if (status == protocol::kOrderStatusCharging) ++chargingCount;
        else if (status == protocol::kOrderStatusWaitSettlement) ++waitSettlementCount;
        else if (status == protocol::kOrderStatusFinished) {
            ++finishedCount;
            paidAmount += query.value(10).toDouble();
        }

        // 筛选：关键词（订单号、手机号、昵称、站点名、桩编号）
        if (!keyword.isEmpty()) {
            QString orderId = QString::number(query.value(0).toInt());
            QString phone = query.value(12).toString();
            QString nickname = query.value(13).toString();
            QString stationName = query.value(14).toString();
            QString chargerCode = query.value(16).toString();

            if (!orderId.contains(keyword) && !phone.contains(keyword)
                && !nickname.contains(keyword) && !stationName.contains(keyword)
                && !chargerCode.contains(keyword)) {
                continue;
            }
        }

        // 筛选：订单状态
        if (statusFilter != QStringLiteral("ALL") && status != statusFilter) {
            continue;
        }

        // 筛选：支付状态
        QString paymentStatus;
        QString amountKind;
        if (status == protocol::kOrderStatusReserved) {
            paymentStatus = QStringLiteral("NOT_DUE");
            amountKind = QStringLiteral("NONE");
        } else if (status == protocol::kOrderStatusCharging) {
            paymentStatus = QStringLiteral("NOT_DUE");
            amountKind = QStringLiteral("ESTIMATED");
        } else if (status == protocol::kOrderStatusWaitSettlement) {
            paymentStatus = QStringLiteral("UNPAID");
            amountKind = QStringLiteral("FINAL");
        } else {
            paymentStatus = QStringLiteral("PAID");
            amountKind = QStringLiteral("FINAL");
        }

        if (paymentStatusFilter != QStringLiteral("ALL") && paymentStatus != paymentStatusFilter) {
            continue;
        }

        ++total;

        // 分页
        if (total <= (page - 1) * pageSize) {
            continue;
        }
        if (ordersArray.size() >= pageSize) {
            continue;
        }

        // 构建订单对象
        QJsonObject orderObj;
        orderObj.insert(QStringLiteral("orderId"), query.value(0).toInt());
        orderObj.insert(QStringLiteral("userId"), query.value(1).toInt());
        orderObj.insert(QStringLiteral("phone"), query.value(12).toString());
        orderObj.insert(QStringLiteral("nickname"), query.value(13).toString());
        orderObj.insert(QStringLiteral("status"), status);
        orderObj.insert(QStringLiteral("paymentStatus"), paymentStatus);
        orderObj.insert(QStringLiteral("amountKind"), amountKind);
        orderObj.insert(QStringLiteral("stationId"), query.value(3).toInt());
        orderObj.insert(QStringLiteral("stationName"), query.value(14).toString());
        orderObj.insert(QStringLiteral("chargerId"), query.value(2).toInt());
        orderObj.insert(QStringLiteral("chargerCode"), query.value(16).toString());
        orderObj.insert(QStringLiteral("type"), query.value(17).toInt());
        orderObj.insert(QStringLiteral("powerKw"), query.value(18).toDouble());
        orderObj.insert(QStringLiteral("pricePerKwh"), query.value(15).toDouble());
        orderObj.insert(QStringLiteral("energyKwh"), query.value(9).toDouble());
        orderObj.insert(QStringLiteral("amount"), query.value(10).toDouble());
        orderObj.insert(QStringLiteral("createdAt"), query.value(11).toLongLong());
        orderObj.insert(QStringLiteral("startTime"), query.value(5).toLongLong());
        orderObj.insert(QStringLiteral("stopTime"), query.value(6).toLongLong());
        orderObj.insert(QStringLiteral("settleTime"), query.value(7).toLongLong());
        orderObj.insert(QStringLiteral("durationSeconds"), query.value(8).toInt());

        ordersArray.append(orderObj);
    }

    // 平台统计
    QJsonObject platformSummary;
    platformSummary.insert(QStringLiteral("totalOrders"), reservedCount + chargingCount + waitSettlementCount + finishedCount);
    platformSummary.insert(QStringLiteral("reservedCount"), reservedCount);
    platformSummary.insert(QStringLiteral("chargingCount"), chargingCount);
    platformSummary.insert(QStringLiteral("waitSettlementCount"), waitSettlementCount);
    platformSummary.insert(QStringLiteral("finishedCount"), finishedCount);
    platformSummary.insert(QStringLiteral("paidAmount"), paidAmount);

    QJsonObject result;
    result.insert(QStringLiteral("orders"), ordersArray);
    result.insert(QStringLiteral("total"), total);
    result.insert(QStringLiteral("page"), page);
    result.insert(QStringLiteral("pageSize"), pageSize);
    result.insert(QStringLiteral("generatedAt"), QDateTime::currentMSecsSinceEpoch());
    result.insert(QStringLiteral("platformSummary"), platformSummary);

    return protocol::makeSuccessResponse(result);
}

} // namespace service
