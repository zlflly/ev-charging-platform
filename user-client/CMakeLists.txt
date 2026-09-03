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
    src/geo/RoutePlanner.cpp
    src/ui/LoginPage.cpp
    src/ui/HomePage.cpp
    src/ui/StationCard.cpp
    src/ui/StationDetailPage.cpp
    src/ui/ChargerCard.cpp
    src/ui/ChargingHeroWidget.cpp
    src/ui/ChargingPage.cpp
    src/ui/SettlementPage.cpp
    src/ui/NavigationPage.cpp
    src/ui/ProfilePage.cpp
    src/ui/OrderPage.cpp
    src/config/AppConfig.h
    src/model/Station.h
    src/model/Order.h
    src/ui/ProfilePage.h
    src/ui/OrderPage.h
    src/ui/theme/Theme.h
)

qt_add_resources(user-client "app_resources"
    PREFIX "/"
    FILES
        resources/images/login_bg.png
        resources/images/station_pile_generated.png
        resources/images/charging_bg.png
        resources/images/navigation_arrival_generated.png
        resources/icons/arrow.svg
        resources/icons/back.svg
        resources/icons/charge.svg
        resources/icons/chevron.svg
        resources/icons/copy.svg
        resources/icons/coupon.svg
        resources/icons/destination.svg
        resources/icons/drive.svg
        resources/icons/favorite.svg
        resources/icons/fee.svg
        resources/icons/home.svg
        resources/icons/info.svg
        resources/icons/layers.svg
        resources/icons/locate.svg
        resources/icons/minus.svg
        resources/icons/navigate_button.svg
        resources/icons/navigate.svg
        resources/icons/order.svg
        resources/icons/overview.svg
        resources/icons/phone.svg
        resources/icons/plus.svg
        resources/icons/profile.svg
        resources/icons/riding.svg
        resources/icons/search.svg
        resources/icons/settings.svg
        resources/icons/station.svg
        resources/icons/straight.svg
        resources/icons/traffic.svg
        resources/icons/turn_left.svg
        resources/icons/turn_right.svg
        resources/icons/walk.svg
        resources/home-map.html
        resources/navigation-map.html
)

target_include_directories(user-client PRIVATE src)
target_link_libraries(user-client PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Widgets
    Qt6::WebEngineWidgets
)
