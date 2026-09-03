#pragma once

#include <QColor>
#include <QFont>
#include <QString>

// ============================================================================
// 主题令牌（EV 充电座舱 - 自研配色，与 WSL 参考项目无关）
//
// 全部页面从这里取颜色/字体/尺寸，禁止在页面里手写十六进制色值。
// ============================================================================
namespace theme {

// ---- 登录页手机画布（参考图为 2x 导出，逻辑尺寸为 471×836）----
inline constexpr int loginCanvasWidth = 471;
inline constexpr int loginCanvasHeight = 836;
inline constexpr int loginCardTop = 425;
inline constexpr int loginCardHeight = 278;
inline constexpr int loginCardHorizontalInset = 20;
inline constexpr int loginFooterTop = 748;

// ---- 首页手机画布 ----
inline constexpr int homeHeaderTop = 50;
inline constexpr int homeSearchTop = 87;
inline constexpr int homeSearchHeight = 35;
inline constexpr int homeMapTop = 130;
inline constexpr int homeMapHeight = 160;
inline constexpr int homeSortTop = 303;
inline constexpr int homeSortHeight = 35;
inline constexpr int homeCardsTop = 349;
inline constexpr int homeCardHeight = 95;
inline constexpr int homeCardGap = 10;
inline constexpr int homeFooterTop = 758;
inline constexpr int homeNavTop = 782;
inline constexpr int homeNavHeight = 54;

// ---- 站点详情手机画布 ----
inline constexpr int detailCanvasHeight = 790;
inline constexpr int detailHeaderTop = 10;
inline constexpr int detailHeaderHeight = 36;
inline constexpr int detailInfoTop = 48;
inline constexpr int detailInfoHeight = 288;
inline constexpr int detailListTop = 344;
inline constexpr int detailListHeight = 367;
inline constexpr int detailActionsTop = 723;
inline constexpr int detailActionsHeight = 52;

// ---- 调色板 ----
inline QColor background()        { return QColor(0x04, 0x0B, 0x1A); }   // 全局底色（深空蓝）
inline QColor cardFill()          { return QColor(0x09, 0x15, 0x28); }   // 卡片底
inline QColor cardBorder()        { return QColor(0x18, 0x2A, 0x44); }   // 卡片描边
inline QColor inputFill()         { return QColor(0x0B, 0x18, 0x2C); }   // 输入框底
inline QColor inputBorder()       { return QColor(0x1E, 0x2C, 0x44); }   // 输入框描边
inline QColor textPrimary()       { return QColor(0xEC, 0xF2, 0xFF); }   // 主读数
inline QColor textSecondary()     { return QColor(0x8A, 0x9A, 0xB5); }   // 次级
inline QColor textMuted()         { return QColor(0x6A, 0x78, 0x90); }   // 弱化
inline QColor primaryBlue()       { return QColor(0x1F, 0x6F, 0xFF); }   // 主按钮
inline QColor primaryBlueHover()  { return QColor(0x4A, 0x8D, 0xFF); }
inline QColor primaryBluePressed(){ return QColor(0x12, 0x55, 0xCC); }
inline QColor linkBlue()          { return QColor(0x4A, 0x8D, 0xFF); }
inline QColor success()           { return QColor(0x4C, 0xD9, 0x72); }
inline QColor danger()            { return QColor(0xFF, 0x62, 0x68); }
inline QColor chipBg()            { return QColor(0x12, 0x22, 0x3A); }   // 手机 icon 圆芯片
inline QColor priceAmber()        { return QColor(0xFF, 0xA3, 0x0A); }

// ---- 尺寸 ----
inline int cardRadius()        { return 20; }
inline int inputRadius()       { return 14; }
inline int buttonRadius()      { return 14; }

// ---- 字体（手机端用适中像素，便于贴近原图）----
inline QFont brandTitleFont() {
    QFont f("Microsoft YaHei UI");
    f.setPixelSize(48);
    f.setBold(true);
    return f;
}
inline QFont brandSubtitleFont() {
    QFont f("Microsoft YaHei UI");
    f.setPixelSize(22);
    f.setWeight(QFont::DemiBold);
    return f;
}
inline QFont fieldLabelFont() {
    QFont f("Microsoft YaHei UI");
    f.setPixelSize(20);
    f.setBold(true);
    return f;
}
inline QFont inputFont() {
    QFont f("Microsoft YaHei UI");
    f.setPixelSize(18);
    return f;
}
inline QFont buttonFont() {
    QFont f("Microsoft YaHei UI");
    f.setPixelSize(22);
    f.setWeight(QFont::DemiBold);
    return f;
}
inline QFont footerFont() {
    QFont f("Microsoft YaHei UI");
    f.setPixelSize(14);
    return f;
}

} // namespace theme
