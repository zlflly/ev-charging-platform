cmake_minimum_required(VERSION 3.16)

project(ev-admin-client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network Widgets)

qt_add_executable(admin-client
    src/main.cpp
    src/app/ApplicationController.cpp
    src/api/AdminApiClient.cpp
    src/model/Charger.cpp
    src/model/ChargerStatusOverview.cpp
    src/model/Station.cpp
    src/model/User.cpp
    src/net/NetworkClient.cpp
    src/session/AdminSession.cpp
    src/ui/LoginWindow.cpp
    src/ui/ChargerManagementPage.cpp
    src/ui/StationManagementPage.cpp
    src/ui/UserManagementPage.cpp
    src/ui/MainWindow.cpp
    src/ui/widgets/ChargerStatusOverviewWidget.cpp
    src/ui/widgets/EntityTableView.cpp
    src/ui/theme/Theme.h
)

target_include_directories(admin-client PRIVATE src)
target_link_libraries(admin-client PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Widgets
)

qt_add_resources(admin-client "admin-client-resources"
    PREFIX "/"
    FILES
        resources/admin-login-hero.png
)

# 成员 1 的服务端就绪前，仅用于验证 framing、requestId 和并发响应匹配。
qt_add_executable(admin-mock-server
    tools/mock-server/MockServer.cpp
)
target_include_directories(admin-mock-server PRIVATE src)
target_link_libraries(admin-mock-server PRIVATE Qt6::Core Qt6::Network)

qt_add_executable(admin-network-smoke-test
    tests/NetworkSmokeTest.cpp
    src/net/NetworkClient.cpp
)
target_include_directories(admin-network-smoke-test PRIVATE src)
target_link_libraries(admin-network-smoke-test PRIVATE Qt6::Core Qt6::Network)

qt_add_executable(admin-auth-smoke-test
    tests/AdminAuthSmokeTest.cpp
    src/net/NetworkClient.cpp
)
target_include_directories(admin-auth-smoke-test PRIVATE src)
target_link_libraries(admin-auth-smoke-test PRIVATE Qt6::Core Qt6::Network)

qt_add_executable(admin-charger-overview-smoke-test
    tests/AdminChargerOverviewSmokeTest.cpp
    src/api/AdminApiClient.cpp
    src/model/Charger.cpp
    src/model/ChargerStatusOverview.cpp
    src/model/Station.cpp
    src/model/User.cpp
    src/net/NetworkClient.cpp
    src/session/AdminSession.cpp
)
target_include_directories(admin-charger-overview-smoke-test PRIVATE src)
target_link_libraries(admin-charger-overview-smoke-test PRIVATE Qt6::Core Qt6::Network)

qt_add_executable(admin-charger-management-smoke-test
    tests/AdminChargerManagementSmokeTest.cpp
    src/api/AdminApiClient.cpp
    src/model/Charger.cpp
    src/model/ChargerStatusOverview.cpp
    src/model/Station.cpp
    src/model/User.cpp
    src/net/NetworkClient.cpp
    src/session/AdminSession.cpp
)
target_include_directories(admin-charger-management-smoke-test PRIVATE src)
target_link_libraries(admin-charger-management-smoke-test PRIVATE Qt6::Core Qt6::Network)

qt_add_executable(admin-station-management-smoke-test
    tests/AdminStationManagementSmokeTest.cpp
    src/api/AdminApiClient.cpp
    src/model/Charger.cpp
    src/model/ChargerStatusOverview.cpp
    src/model/Station.cpp
    src/model/User.cpp
    src/net/NetworkClient.cpp
    src/session/AdminSession.cpp
)
target_include_directories(admin-station-management-smoke-test PRIVATE src)
target_link_libraries(admin-station-management-smoke-test PRIVATE Qt6::Core Qt6::Network)

qt_add_executable(admin-station-operations-smoke-test
    tests/AdminStationOperationsSmokeTest.cpp
    src/api/AdminApiClient.cpp
    src/model/Charger.cpp
    src/model/ChargerStatusOverview.cpp
    src/model/Station.cpp
    src/model/User.cpp
    src/net/NetworkClient.cpp
    src/session/AdminSession.cpp
)
target_include_directories(admin-station-operations-smoke-test PRIVATE src)
target_link_libraries(admin-station-operations-smoke-test PRIVATE Qt6::Core Qt6::Network)

qt_add_executable(admin-user-management-smoke-test
    tests/AdminUserManagementSmokeTest.cpp
    src/api/AdminApiClient.cpp
    src/model/Charger.cpp
    src/model/ChargerStatusOverview.cpp
    src/model/Station.cpp
    src/model/User.cpp
    src/net/NetworkClient.cpp
    src/session/AdminSession.cpp
)
target_include_directories(admin-user-management-smoke-test PRIVATE src)
target_link_libraries(admin-user-management-smoke-test PRIVATE Qt6::Core Qt6::Network)
