cmake_minimum_required(VERSION 3.16)

project(user-client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network Widgets WebEngineWidgets)

qt_standard_project_setup()

# 主程序（登录页演示）
qt_add_executable(user-client
    src/main.cpp
    src/net/NetworkClient.cpp
    src/session/Session.cpp
    src/geo/Geocoder.cpp
    src/ui/LoginPage.cpp
    src/ui/HomePage.cpp
    src/ui/StationCard.cpp
    src/ui/StationDetailPage.cpp
    src/ui/ChargerCard.cpp
    src/ui/ChargingHeroWidget.cpp
    src/ui/ChargingPage.cpp
    src/ui/SettlementPage.cpp
    src/config/AppConfig.h
    src/model/Station.h
    src/model/Order.h
    src/ui/theme/Theme.h
)

qt_add_resources(user-client "app_resources"
    PREFIX "/"
    FILES
        resources/images/login_bg.png
        resources/images/station_pile_generated.png
        resources/images/charging_bg.png
        resources/home-map.html
)

target_include_directories(user-client PRIVATE src)
target_link_libraries(user-client PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Widgets
    Qt6::WebEngineWidgets
)
