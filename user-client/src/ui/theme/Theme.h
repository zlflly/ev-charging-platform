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
        QFrame#Divider { background: #DCE5E8; min-height: 1px; max-height: 1px; }
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
        QScrollArea { border: none; background: transparent; }
        QScrollArea > QWidget > QWidget { background: transparent; }
        QScrollBar:vertical { background: transparent; width: 6px; margin: 2px; }
        QScrollBar::handle:vertical { background: #C7D3D8; border-radius: 3px; min-height: 32px; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
        QToolTip { background: #14243A; color: white; border: none; padding: 7px 9px; }
    )QSS");
}

} // namespace Theme
