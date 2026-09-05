#pragma once

#include <QString>

namespace Theme {

inline constexpr const char* Canvas = "#F4F7F6";
inline constexpr const char* Surface = "#FFFFFF";
inline constexpr const char* Ink = "#14243A";
inline constexpr const char* Muted = "#647487";
inline constexpr const char* Line = "#DCE5E8";
inline constexpr const char* Blue = "#176CFF";
inline constexpr const char* BlueSoft = "#EAF1FF";
inline constexpr const char* Green = "#149B68";
inline constexpr const char* GreenSoft = "#E5F6EE";
inline constexpr const char* Amber = "#B66B08";
inline constexpr const char* AmberSoft = "#FFF2D8";
inline constexpr const char* Red = "#D84C4C";
inline constexpr const char* RedSoft = "#FDEAEA";

inline QString globalStyleSheet()
{
    return QStringLiteral(R"QSS(
        * { font-family: "Noto Sans CJK SC", "Microsoft YaHei UI", sans-serif; color: #14243A; }
        QMainWindow, QWidget#AppRoot, QWidget#Page { background: #F4F7F6; }
        QLabel { background: transparent; }
        QLabel#PageTitle { font-size: 28px; font-weight: 700; }
        QLabel#HomeTitle { font-size: 26px; font-weight: 750; color: #102744; }
        QLabel#HomeLocation { color: #43566E; font-size: 13px; }
        QLabel#HomeSectionTitle { font-size: 18px; font-weight: 700; color: #102744; }
        QLabel#HomeSectionCount { color: #647487; font-size: 12px; }
        QLabel#StationName { color: #102744; font-size: 15px; font-weight: 700; }
        QLabel#StationDistance { color: #2E4562; font-size: 14px; font-weight: 700; }
        QLabel#StationEta { color: #718198; font-size: 11px; }
        QLabel#StationCaption { color: #7A889B; font-size: 10px; }
        QLabel#StationPrice { color: #102744; font-size: 18px; font-weight: 750; }
        QLabel#StationAvailable { color: #149B68; font-size: 20px; font-weight: 750; }
        QLabel#StationUnavailable { color: #D84C4C; font-size: 20px; font-weight: 750; }
        QLabel#StationTag { color: #43566E; background: #F1F5FA; border-radius: 6px; padding: 2px 5px; font-size: 10px; }
        QLabel#StationTagBlue { color: #176CFF; background: #EAF1FF; border-radius: 6px; padding: 2px 5px; font-size: 10px; }
        QLabel#StationTagAmber { color: #B66B08; background: #FFF2D8; border-radius: 6px; padding: 2px 5px; font-size: 10px; }
        QLabel#SectionTitle { font-size: 19px; font-weight: 650; }
        QLabel#Muted { color: #647487; font-size: 13px; }
        QLabel#Metric { font-size: 40px; font-weight: 750; }
        QLabel#ProfileName { font-size: 23px; font-weight: 700; }
        QLabel#Amount { font-size: 24px; font-weight: 700; }
        QLabel#StatusOk { color: #147D59; background: #E5F6EE; border-radius: 9px; padding: 3px 8px; font-weight: 650; }
        QLabel#StatusWarn { color: #935609; background: #FFF2D8; border-radius: 9px; padding: 3px 8px; font-weight: 650; }
        QLabel#StatusBad { color: #B83C3C; background: #FDEAEA; border-radius: 9px; padding: 3px 8px; font-weight: 650; }
        QLabel#StatusInfo { color: #1558C7; background: #EAF1FF; border-radius: 9px; padding: 3px 8px; font-weight: 650; }
        QFrame#Surface { background: #FFFFFF; border-radius: 16px; }
        QFrame#SoftSurface { background: #EAF1FF; border-radius: 16px; }
        QFrame#RouteInstructionCard { background: #FFFFFF; border-radius: 16px; }
        QFrame#NavigationStatsCard { background: #FFFFFF; border-radius: 14px; }
        QFrame#NavigationPanel { background: #FFFFFF; border-radius: 22px; }
        QFrame#NavigationHandle { background: #B9C8D2; border-radius: 2px; min-height: 4px; max-height: 4px; }
        QFrame#Divider { background: #DCE5E8; min-height: 1px; max-height: 1px; }
        QFrame#VerticalDivider { background: #DCE5E8; min-width: 1px; max-width: 1px; }
        QFrame#HomeSearchShell { background: #FFFFFF; border: 1px solid #DCE5E8; border-radius: 14px; }
        QLabel#NavigationHeader { font-size: 20px; font-weight: 700; }
        QLabel#NavigationStepLead { color: #43566E; font-size: 14px; }
        QLabel#NavigationRoad { color: #14243A; font-size: 20px; font-weight: 700; }
        QLabel#MapSummaryDuration { color: #176CFF; font-size: 27px; font-weight: 750; }
        QLabel#MapSummaryDistance { color: #14243A; font-size: 14px; font-weight: 650; }
        QLabel#MapSummaryLabel { color: #647487; font-size: 11px; }
        QLabel#MapSummaryEta { color: #14243A; font-size: 18px; font-weight: 700; }
        QLabel#NavigationStationTitle { color: #14243A; font-size: 18px; font-weight: 700; }
        QLabel#NavigationStationMeta { color: #647487; font-size: 12px; }
        QLabel#NavigationMetricLabel { color: #647487; font-size: 11px; }
        QLabel#NavigationMetricValue { color: #14243A; font-size: 18px; font-weight: 700; }
        QLabel#NavigationMetricValueGreen { color: #149B68; font-size: 18px; font-weight: 700; }
        QLineEdit { background: #FFFFFF; border: 1px solid #DCE5E8; border-radius: 14px; padding: 0 15px; min-height: 50px; font-size: 15px; selection-background-color: #176CFF; }
        QLineEdit:focus { border: 2px solid #176CFF; padding: 0 13px; }
        QPushButton { border: none; border-radius: 14px; min-height: 50px; padding: 0 18px; font-size: 15px; font-weight: 650; }
        QPushButton#Primary { background: #176CFF; color: white; }
        QPushButton#Primary:hover { background: #0F5FE9; }
        QPushButton#Primary:pressed { background: #0B4FCA; }
        QPushButton#Primary:disabled { background: #B9C8DA; color: #EDF2F5; }
        QPushButton#Secondary { background: #EAF1FF; color: #1558C7; }
        QPushButton#Secondary:hover { background: #DDE9FF; }
        QPushButton#Quiet { background: transparent; color: #1558C7; padding: 0 8px; }
        QPushButton#Danger { background: #FDEAEA; color: #B83C3C; }
        QPushButton#NavigationSecondary { background: #FFFFFF; color: #176CFF; border: 1px solid #176CFF; }
        QPushButton#NavigationSecondary:hover { background: #EAF1FF; }
        QPushButton#HomeRefresh { background: #FFFFFF; color: #176CFF; border: 1px solid #E3E9F0; border-radius: 13px; min-height: 40px; padding: 0 10px; font-size: 12px; }
        QPushButton#HomeRefresh:hover { background: #F4F8FF; border-color: #BFD2FF; }
        QPushButton#HomeFilter { background: #FFFFFF; color: #61738B; border: 1px solid #E2E8EF; border-radius: 9px; min-height: 28px; padding: 0 4px; font-size: 11px; }
        QPushButton#HomeFilter:hover { background: #F7FAFE; border-color: #C8D7E8; }
        QPushButton#HomeFilter:checked { background: #F1F6FF; color: #176CFF; border: 1px solid #AFC9FF; font-weight: 650; }
        QPushButton#HomeLocate { background: #F5F8FE; color: #176CFF; border: 1px solid #D4DFEF; border-radius: 14px; min-height: 50px; padding: 0 13px; font-size: 14px; }
        QPushButton#HomeLocate:hover { background: #EAF1FF; border-color: #AFC9FF; }
        QPushButton#StationNavigate { background: #176CFF; color: #FFFFFF; border-radius: 9px; min-height: 30px; padding: 0 8px; font-size: 12px; }
        QPushButton#StationNavigate:hover { background: #0F5FE9; }
        QPushButton#StationDetail { background: #FFFFFF; color: #176CFF; border: 1px solid #AFC9FF; border-radius: 9px; min-height: 28px; padding: 0 8px; font-size: 12px; }
        QPushButton#StationDetail:hover { background: #F1F6FF; }
        QScrollArea { border: none; background: transparent; }
        QScrollArea > QWidget > QWidget { background: transparent; }
        QScrollBar:vertical { background: transparent; width: 6px; margin: 2px; }
        QScrollBar::handle:vertical { background: #C7D3D8; border-radius: 3px; min-height: 32px; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
        QToolTip { background: #14243A; color: white; border: none; padding: 7px 9px; }
    )QSS");
}

} // namespace Theme
