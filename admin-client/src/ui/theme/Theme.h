#pragma once

#include <QString>

namespace theme {

inline constexpr int kSidebarWidth = 224;
inline constexpr int kPageMargin = 24;

inline QString globalStyleSheet()
{
    return QStringLiteral(R"QSS(
        * {
            font-family: "Noto Sans CJK SC", "Microsoft YaHei", sans-serif;
        }
        QMainWindow, QWidget#appRoot, QWidget#pageRoot {
            background: #050B14;
            color: #F4F8FC;
        }
        QWidget:window { background: #050B14; color: #F4F8FC; }
        QLabel { background: transparent; }
        QFrame#sidebar {
            background: #07111E;
            border-right: 1px solid #18344E;
        }
        QFrame#topbar {
            background: #081421;
            border-bottom: 1px solid #18344E;
        }
        QFrame#card, QFrame#metricCard, QFrame#sectionCard, QFrame#filterBar {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:1,
                        stop:0 #0D2033, stop:1 #091725);
            border: 1px solid #1C405E;
            border-radius: 14px;
        }
        QFrame#filterBar {
            background: #081827;
            border-color: #173A55;
            border-radius: 11px;
        }
        QLabel#brandMark {
            color: #38E6A5;
            font: 800 24px "Noto Sans Mono CJK SC";
        }
        QLabel#brandTitle { color: #F4F8FC; font-size: 16px; font-weight: 700; }
        QLabel#eyebrow { color: #48C8FF; font-size: 11px; font-weight: 700; }
        QLabel#pageTitle { color: #F4F8FC; font-size: 24px; font-weight: 750; }
        QLabel#pageSubtitle, QLabel#muted { color: #8DA8C0; font-size: 12px; }
        QLabel#metricLabel { color: #8DA8C0; font-size: 12px; }
        QLabel#metricValue {
            color: #F4F8FC;
            font: 700 24px "Noto Sans Mono CJK SC";
        }
        QLabel#metricHint { color: #5E7F9D; font-size: 11px; }
        QLabel#sectionTitle { color: #E9F4FF; font-size: 15px; font-weight: 700; }
        QLabel#stateTitle { color: #DDEEFF; font-size: 16px; font-weight: 700; }
        QLabel#stateMessage { color: #7895AE; font-size: 12px; }
        QLabel#filterCount {
            color: #B5CCE0;
            background: #0D2639;
            border: 1px solid #27506D;
            border-radius: 9px;
            padding: 7px 11px;
            font-weight: 650;
        }
        QLabel#sortHint { color: #6FA8C9; font-size: 11px; }
        QLabel#connectionBadge {
            color: #8DA8C0;
            background: #0C1B2A;
            border: 1px solid #24425E;
            border-radius: 12px;
            padding: 5px 10px;
        }
        QLabel#connectionBadge[connectionState="online"] {
            color: #38E6A5; border-color: #246C5A; background: #0B2824;
        }
        QLabel#connectionBadge[connectionState="checking"] {
            color: #FFC04D; border-color: #735F2E; background: #2A2414;
        }
        QPushButton#navButton {
            background: transparent;
            color: #87A2BA;
            border: 0;
            border-left: 3px solid transparent;
            border-radius: 0;
            min-height: 45px;
            padding: 0 18px;
            text-align: left;
            font-size: 14px;
        }
        QPushButton#navButton:hover { color: #EAF5FF; background: #0D2033; }
        QPushButton#navButton:checked {
            color: #F4F8FC;
            background: #0F2940;
            border-left-color: #38E6A5;
            font-weight: 700;
        }
        QPushButton#primaryButton, QPushButton#secondaryButton,
        QPushButton#dangerButton, QPushButton#ghostButton {
            min-height: 36px;
            border-radius: 9px;
            padding: 0 16px;
            font-weight: 650;
        }
        QPushButton#primaryButton {
            color: #04111D;
            background: #38E6A5;
            border: 1px solid #66F3BC;
        }
        QPushButton#primaryButton:hover { background: #65F0BE; }
        QPushButton#secondaryButton {
            color: #DCEEFF;
            background: #0E2235;
            border: 1px solid #28506E;
        }
        QPushButton#secondaryButton:hover { border-color: #48C8FF; background: #12304A; }
        QPushButton#dangerButton {
            color: #FFDDE0;
            background: #31171E;
            border: 1px solid #7A303B;
        }
        QPushButton#dangerButton:hover { color:#FFFFFF; background:#492029; border-color:#FF6268; }
        QPushButton#ghostButton {
            color: #8DA8C0;
            background: transparent;
            border: 1px solid #24425E;
        }
        QPushButton#ghostButton:hover { color:#EAF5FF; border-color:#48C8FF; background:#10263A; }
        QPushButton#faultAlertButton {
            min-height:36px;
            padding:0 14px;
            color:#FF8B92;
            background:#31171E;
            border:1px solid #7A303B;
            border-radius:9px;
            font-weight:700;
        }
        QPushButton#faultAlertButton:hover {
            color:#FFFFFF; background:#492029; border-color:#FF6268;
        }
        QPushButton:disabled { color:#5E7182; background:#101A25; border-color:#223243; }
        QLineEdit, QComboBox {
            color: #F4F8FC;
            background: #0B1B2A;
            border: 1px solid #24425E;
            border-radius: 9px;
            min-height: 38px;
            padding: 0 12px;
        }
        QLineEdit:focus, QComboBox:focus { border-color: #48C8FF; }
        QComboBox::drop-down { border: 0; width: 24px; }
        QComboBox QAbstractItemView {
            color: #E8F3FC;
            background: #0D2033;
            border: 1px solid #28506E;
            selection-background-color: #12466A;
            selection-color: #FFFFFF;
            outline: 0;
        }
        QTableView {
            color: #DDE9F4;
            background: #091725;
            alternate-background-color: #0C1D2D;
            border: 0;
            gridline-color: #17334C;
            selection-background-color: #12466A;
            selection-color: #FFFFFF;
        }
        QHeaderView::section {
            color: #84A4BD;
            background: #0D2033;
            border: 0;
            border-bottom: 1px solid #24425E;
            padding: 9px 10px;
            font-weight: 650;
        }
        QHeaderView::section:hover { color:#EAF5FF; background:#12304A; }
        QTableCornerButton::section { background: #0D2033; border: 0; }
        QScrollBar:vertical { width: 8px; background: transparent; }
        QScrollBar::handle:vertical { min-height: 28px; border-radius: 4px; background: #24425E; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
        QScrollBar:horizontal { height: 8px; background: transparent; }
        QScrollBar::handle:horizontal { min-width: 28px; border-radius: 4px; background: #24425E; }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }
        QDialog#operationDialog {
            background: #081421;
            color: #F4F8FC;
        }
        QFrame#dialogPanel {
            background: #0D2033;
            border: 1px solid #28506E;
            border-radius: 11px;
        }
        QLabel#dialogEyebrow { color:#FF9A72; font-size:11px; font-weight:700; }
        QLabel#dialogTitle { color:#F4F8FC; font-size:21px; font-weight:750; }
        QLabel#dialogText { color:#9DB5C9; font-size:13px; }
        QLabel#dialogTarget {
            color:#FFFFFF;
            font:700 19px "Noto Sans Mono CJK SC";
        }
        QLabel#dialogDetails { color:#AFC5D7; font-size:13px; }
        QLabel#dialogWarning {
            color:#FFC04D;
            background:#2A2414;
            border:1px solid #735F2E;
            border-radius:9px;
            padding:10px 12px;
        }
        QLabel#dialogStatus {
            min-width:52px; min-height:52px; max-width:52px; max-height:52px;
            border-radius:26px;
            font-size:26px;
            font-weight:800;
        }
        QLabel#dialogStatus[result="success"] {
            color:#38E6A5; background:#0B2824; border:1px solid #246C5A;
        }
        QLabel#dialogStatus[result="error"] {
            color:#FF7D87; background:#31171E; border:1px solid #7A303B;
        }
    )QSS");
}

} // namespace theme
