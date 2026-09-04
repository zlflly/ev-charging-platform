#pragma once

#include <QString>

namespace theme {

inline constexpr int kSidebarWidth = 224;
inline constexpr int kPageMargin = 24;

// 浅色卡片体系与用户端保持一致；管理端保留桌面侧栏和高密度表格。
inline QString globalStyleSheet()
{
    return QStringLiteral(R"QSS(
        * { font-family: "Noto Sans CJK SC", "Microsoft YaHei", sans-serif; }
        QMainWindow, QWidget#appRoot, QWidget#pageRoot { background:#F3F6F8; color:#10233F; }
        QWidget:window { background:#F3F6F8; color:#10233F; }
        QLabel { background:transparent; color:#10233F; }
        QFrame#sidebar { background:#FFFFFF; border-right:1px solid #DCE5EE; }
        QFrame#topbar { background:#FFFFFF; border-bottom:1px solid #DCE5EE; }
        QFrame#card, QFrame#metricCard, QFrame#sectionCard, QFrame#filterBar {
            background:#FFFFFF; border:1px solid #DCE5EE; border-radius:16px;
        }
        QFrame#filterBar { background:#F8FAFC; border-color:#DCE5EE; border-radius:12px; }
        QLabel#brandMark { color:#1769E8; font:800 24px "Noto Sans Mono CJK SC"; }
        QLabel#brandTitle { color:#10233F; font-size:16px; font-weight:700; }
        QLabel#eyebrow { color:#1769E8; font-size:11px; font-weight:700; }
        QLabel#pageTitle { color:#10233F; font-size:24px; font-weight:750; }
        QLabel#pageSubtitle, QLabel#muted { color:#64778B; font-size:12px; }
        QLabel#metricLabel { color:#64778B; font-size:12px; }
        QLabel#metricValue { color:#10233F; font:700 24px "Noto Sans Mono CJK SC"; }
        QLabel#metricHint { color:#8191A3; font-size:11px; }
        QLabel#sectionTitle { color:#10233F; font-size:15px; font-weight:700; }
        QLabel#stateTitle { color:#183153; font-size:16px; font-weight:700; }
        QLabel#stateMessage { color:#6E8195; font-size:12px; }
        QLabel#filterCount, QLabel#metricChip {
            color:#24527F; background:#EEF5FF; border:1px solid #CFE0F5;
            border-radius:9px; padding:7px 11px; font-weight:650;
        }
        QLabel#sortHint { color:#6683A2; font-size:11px; }
        QLabel#connectionBadge {
            color:#64778B; background:#F5F8FB; border:1px solid #D8E2EC;
            border-radius:12px; padding:5px 10px;
        }
        QLabel#connectionBadge[connectionState="online"] {
            color:#14865A; border-color:#BEE8D5; background:#EAF8F1;
        }
        QLabel#connectionBadge[connectionState="checking"] {
            color:#9A6810; border-color:#F0D99F; background:#FFF8E7;
        }
        QPushButton#navButton {
            background:transparent; color:#667A90; border:0;
            border-left:3px solid transparent; border-radius:0; min-height:45px;
            padding:0 18px; text-align:left; font-size:14px;
        }
        QPushButton#navButton:hover { color:#10233F; background:#F1F6FC; }
        QPushButton#navButton:checked {
            color:#1769E8; background:#EAF2FF; border-left-color:#1769E8; font-weight:700;
        }
        QPushButton#primaryButton, QPushButton#secondaryButton,
        QPushButton#dangerButton, QPushButton#ghostButton {
            min-height:38px; border-radius:10px; padding:0 16px; font-weight:650;
        }
        QPushButton#primaryButton { color:#FFFFFF; background:#246BFD; border:1px solid #246BFD; }
        QPushButton#primaryButton:hover { background:#1558D6; border-color:#1558D6; }
        QPushButton#secondaryButton { color:#24527F; background:#EEF5FF; border:1px solid #C8DAEE; }
        QPushButton#secondaryButton:hover { color:#1769E8; border-color:#8DB7EA; background:#E4F0FF; }
        QPushButton#dangerButton { color:#C43742; background:#FFF0F1; border:1px solid #F1BEC3; }
        QPushButton#dangerButton:hover { color:#FFFFFF; background:#D94852; border-color:#D94852; }
        QPushButton#ghostButton { color:#64778B; background:transparent; border:1px solid #D1DDE8; }
        QPushButton#ghostButton:hover { color:#1769E8; border-color:#8DB7EA; background:#F3F8FF; }
        QPushButton#faultAlertButton {
            min-height:38px; padding:0 14px; color:#C43742; background:#FFF0F1;
            border:1px solid #F1BEC3; border-radius:10px; font-weight:700;
        }
        QPushButton#faultAlertButton:hover { color:#FFFFFF; background:#D94852; border-color:#D94852; }
        QPushButton:disabled { color:#9AA8B6; background:#EDF1F4; border-color:#DDE4EA; }
        QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox {
            color:#10233F; background:#FFFFFF; border:1px solid #CFDAE5;
            border-radius:10px; min-height:38px; padding:0 12px;
        }
        QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus { border-color:#246BFD; }
        QSpinBox::up-button, QSpinBox::down-button,
        QDoubleSpinBox::up-button, QDoubleSpinBox::down-button { width:22px; background:#EEF5FF; border:0; }
        QComboBox::drop-down { border:0; width:24px; }
        QComboBox QAbstractItemView {
            color:#10233F; background:#FFFFFF; border:1px solid #CFDAE5;
            selection-background-color:#E4F0FF; selection-color:#1769E8; outline:0;
        }
        QTableView {
            color:#183153; background:#FFFFFF; alternate-background-color:#F8FAFC;
            border:0; gridline-color:#E2E9F0;
            selection-background-color:#DCEBFF; selection-color:#10233F;
        }
        QHeaderView::section {
            color:#5E748B; background:#F3F7FB; border:0;
            border-bottom:1px solid #DCE5EE; padding:9px 10px; font-weight:650;
        }
        QHeaderView::section:hover { color:#1769E8; background:#EAF2FF; }
        QTableCornerButton::section { background:#F3F7FB; border:0; }
        QScrollBar:vertical { width:8px; background:transparent; }
        QScrollBar::handle:vertical { min-height:28px; border-radius:4px; background:#C5D1DC; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height:0; }
        QScrollBar:horizontal { height:8px; background:transparent; }
        QScrollBar::handle:horizontal { min-width:28px; border-radius:4px; background:#C5D1DC; }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width:0; }
        QSplitter#stationSplitter::handle { height:8px; background:transparent; }
        QDialog#operationDialog, QMessageBox { background:#F3F6F8; color:#10233F; }
        QMessageBox QLabel { color:#10233F; min-width:300px; }
        QMessageBox QPushButton {
            min-width:78px; min-height:34px; color:#FFFFFF; background:#246BFD;
            border:0; border-radius:9px;
        }
        QFrame#dialogPanel { background:#FFFFFF; border:1px solid #DCE5EE; border-radius:12px; }
        QLabel#dialogEyebrow { color:#1769E8; font-size:11px; font-weight:700; }
        QLabel#dialogTitle { color:#10233F; font-size:21px; font-weight:750; }
        QLabel#dialogText { color:#5F7388; font-size:13px; }
        QLabel#dialogTarget { color:#10233F; font:700 19px "Noto Sans Mono CJK SC"; }
        QLabel#dialogDetails { color:#536A82; font-size:13px; }
        QLabel#dialogWarning {
            color:#855D0B; background:#FFF7E2; border:1px solid #EED28A;
            border-radius:10px; padding:10px 12px;
        }
        QLabel#formError {
            color:#B72E39; background:#FFF0F1; border:1px solid #F1BEC3;
            border-radius:10px; padding:9px 12px;
        }
        QLabel#dialogStatus {
            min-width:52px; min-height:52px; max-width:52px; max-height:52px;
            border-radius:26px; font-size:26px; font-weight:800;
        }
        QLabel#dialogStatus[result="success"] { color:#14865A; background:#EAF8F1; border:1px solid #BEE8D5; }
        QLabel#dialogStatus[result="error"] { color:#C43742; background:#FFF0F1; border:1px solid #F1BEC3; }
        QToolTip { color:#FFFFFF; background:#183153; border:1px solid #183153; padding:6px 8px; }
    )QSS");
}

} // namespace theme
