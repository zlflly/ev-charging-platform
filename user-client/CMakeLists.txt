cmake_minimum_required(VERSION 3.16)

project(user-client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network Widgets WebEngineWidgets)
qt_standard_project_setup()

qt_add_executable(user-client
    src/main.cpp
    src/ui/MobileApp.cpp
    src/ui/MobileApp.h
    src/ui/theme/Theme.h
    src/net/NetworkClient.cpp
    src/net/NetworkClient.h
    src/session/Session.cpp
    src/session/Session.h
    src/geo/Geocoder.cpp
    src/geo/Geocoder.h
    src/geo/RoutePlanner.cpp
    src/geo/RoutePlanner.h
    src/config/AppConfig.h
    src/model/Station.h
    src/model/Order.h
    src/protocol/Protocol.h
)

target_include_directories(user-client PRIVATE src)
target_link_libraries(user-client PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Widgets
    Qt6::WebEngineWidgets
)

qt_add_resources(user-client "app_resources"
    PREFIX "/"
    FILES
        resources/home-map.html
)
