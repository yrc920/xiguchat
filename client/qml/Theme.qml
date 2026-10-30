pragma Singleton
import QtQuick

// 全局设计令牌。极简版式：浅暖底 + 近黑主色 + 一处克制的强调色。
// 所有页面共用这一份，替代 stylesheet.qss。
QtObject {
    // 背景
    readonly property color bgTop: "#FBFAF8"
    readonly property color bgBottom: "#F1EEE9"

    // 文字
    readonly property color textPrimary: "#17181A"
    readonly property color textSecondary: "#8B8D91"
    readonly property color textHint: "#B3B5B9"
    readonly property color danger: "#C0392B"
    readonly property color success: "#3C8C4E"

    // 强调色：只用于聚焦边框、按钮悬停与链接
    readonly property color accent: "#C05A34"
    readonly property color accentSoft: "#1FC05A34"

    // 主按钮
    readonly property color primary: "#1B1C1E"
    readonly property color primaryHover: "#2C2E31"
    readonly property color primaryPressed: "#0E0F10"

    // 聊天界面
    readonly property color chatBg: "#F4F2EE"
    readonly property color panelBg: "#FFFFFF"
    readonly property color bubbleSelf: "#1B1C1E"
    readonly property color bubbleSelfText: "#F7F6F3"
    readonly property color bubbleOther: "#FFFFFF"
    readonly property color sidebarBg: "#1B1C1E"

    // 输入框
    readonly property color fieldBg: "#FFFFFF"
    readonly property color fieldBorder: "#E4E1DC"

    // 字号
    readonly property int fontTitle: 22
    readonly property int fontBody: 14
    readonly property int fontSmall: 12

    // 尺寸
    readonly property int radius: 10
    readonly property real bubbleMaxWidth: 460
    readonly property int fieldHeight: 44
}
