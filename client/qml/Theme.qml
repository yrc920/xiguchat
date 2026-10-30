pragma Singleton
import QtQuick
import QtCore

// 纸白与墨绿共用一套语义色；外观选择保存在本机。
QtObject {
    id: theme
    property bool dark: false
    property QtObject preferences: Settings {
        category: "Appearance"
        property alias dark: theme.dark
    }
    function toggle() { dark = !dark }

    readonly property color bgTop: dark ? "#17231F" : "#F5F5EE"
    readonly property color panelBg: dark ? "#202E28" : "#FDFDF8"
    readonly property color panelAlt: dark ? "#293830" : "#EBEFE5"
    readonly property color chatBg: dark ? "#192720" : "#F2F4ED"
    readonly property color sidebarBg: dark ? "#132019" : "#E3E9DC"
    readonly property color selected: dark ? "#34493B" : "#DDE8D6"
    readonly property color line: dark ? "#35443B" : "#DCE3D5"

    readonly property color textPrimary: dark ? "#E7EDDF" : "#233D30"
    readonly property color textSecondary: dark ? "#A8B7AA" : "#566B58"
    readonly property color textHint: dark ? "#84978A" : "#60705E"
    readonly property color ink: dark ? "#B7CFAB" : "#294836"
    readonly property color danger: dark ? "#F09B87" : "#B64937"
    readonly property color success: dark ? "#ACCDA4" : "#4B7950"
    readonly property color accent: dark ? "#ED9D80" : "#B4543C"
    readonly property color stamp: accent

    readonly property color primary: dark ? "#BCD2AD" : "#2C503B"
    readonly property color primaryHover: dark ? "#CDE1BF" : "#3D634A"
    readonly property color primaryPressed: dark ? "#A7BF97" : "#1E3D2C"
    readonly property color primaryText: dark ? "#1C3022" : "#F6F9EE"
    readonly property color primarySoft: dark ? "#334739" : "#E2ECD9"

    readonly property color bubbleSelf: dark ? "#3C523B" : "#E0EBCF"
    readonly property color bubbleSelfText: textPrimary
    readonly property color bubbleOther: dark ? "#29382E" : "#FFFFFF"
    readonly property color fieldBg: dark ? "#1A2820" : "#FBFCF5"
    readonly property color fieldBorder: dark ? "#425545" : "#CCD7C4"
    readonly property color overlay: dark ? "#AF080F0B" : "#60253629"
    readonly property color paper: dark ? "#D8D9BD" : "#FFFDF0"
    readonly property color paperFold: dark ? "#ABB99A" : "#E1E8CE"
    readonly property color paperInk: "#344B37"

    readonly property int fontTitle: 26
    readonly property int fontBody: 14
    readonly property int fontSmall: 12
    readonly property int radius: 8
    readonly property real bubbleMaxWidth: 460
    readonly property int fieldHeight: 46
}
