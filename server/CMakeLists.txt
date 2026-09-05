cmake_minimum_required(VERSION 3.16)

project(ev-server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# 供 IDE（CLion/clangd）索引头文件用，不影响构建产物。
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Qt 安装路径未在环境中时，允许通过 -DQt6_ROOT=... 或环境变量 Qt6_DIR 指定。
# 目标运行环境是 Ubuntu + Qt6（apt 装的 Qt 会被自动找到，无需额外配置）。
find_package(Qt6 REQUIRED COMPONENTS Core Network Sql Test)

qt_standard_project_setup()

enable_testing()

# ---------------------------------------------------------------------------
# 服务端主程序
# ---------------------------------------------------------------------------
qt_add_executable(ev-server
    src/main.cpp
    src/protocol/Protocol.h
    src/config/ServerConfig.h
    src/net/FrameCodec.h
    src/net/TcpConnection.h
    src/net/TcpServer.h
    src/net/RequestDispatcher.h
    src/net/SessionManager.h
    src/net/SessionManager.cpp
    src/repository/Database.h
    src/repository/Database.cpp
    src/repository/UserRepository.h
    src/repository/UserRepository.cpp
    src/repository/AdminRepository.h
    src/repository/AdminRepository.cpp
    src/repository/StationRepository.h
    src/repository/StationRepository.cpp
    src/repository/ChargerRepository.h
    src/repository/ChargerRepository.cpp
    src/repository/OrderRepository.h
    src/repository/OrderRepository.cpp
    src/service/UserService.h
    src/service/UserService.cpp
    src/service/StationService.h
    src/service/StationService.cpp
    src/service/OrderService.h
    src/service/OrderService.cpp
    src/service/AdminService.h
    src/service/AdminService.cpp
)

target_include_directories(ev-server PRIVATE src)
target_link_libraries(ev-server PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Sql
)

# ---------------------------------------------------------------------------
# 单元测试：FrameCodec
# ---------------------------------------------------------------------------
qt_add_executable(FrameCodecTest
    tests/FrameCodecTest.cpp
    src/net/FrameCodec.h
    src/protocol/Protocol.h
)

target_include_directories(FrameCodecTest PRIVATE src)
target_link_libraries(FrameCodecTest PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME FrameCodecTest COMMAND FrameCodecTest)

# ---------------------------------------------------------------------------
# 冒烟测试工具
# ---------------------------------------------------------------------------
qt_add_executable(ev-smoke-test
    tools/SmokeTest.cpp
    src/net/FrameCodec.h
    src/protocol/Protocol.h
)

target_include_directories(ev-smoke-test PRIVATE src)
target_link_libraries(ev-smoke-test PRIVATE
    Qt6::Core
    Qt6::Network
)
