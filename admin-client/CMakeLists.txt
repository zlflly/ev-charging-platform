cmake_minimum_required(VERSION 3.16)

project(ev-admin-client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network Widgets)

qt_add_executable(admin-client
    src/main.cpp
    src/net/NetworkClient.cpp
    src/ui/LoginWindow.cpp
    src/ui/MainWindow.cpp
    src/ui/widgets/EntityTableView.cpp
    src/ui/theme/Theme.h
)

target_include_directories(admin-client PRIVATE src)
target_link_libraries(admin-client PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Widgets
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
