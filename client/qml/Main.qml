import QtQuick
import XiguChat

// 窗口负责页面装载、窗口尺寸和会话中断提示。
Window {
    id: window
    visible: true
    title: "xiguchat"
    color: Theme.bgTop

    ShellBridge { id: shell }

    function pageFile(name) {
        if (name === "chat") return "ChatWindow.qml"
        if (name === "register") return "Register.qml"
        if (name === "reset") return "Reset.qml"
        return "Login.qml"
    }
    function showPage(name) {
        maximumWidth = 16777215
        maximumHeight = 16777215
        if (name === "chat") {
            minimumWidth = 960
            minimumHeight = 680
            width = Math.max(width, 1060)
            height = Math.max(height, 740)
        } else {
            const h = name === "register" ? 740 : name === "reset" ? 680 : 560
            minimumWidth = 420
            minimumHeight = h
            maximumWidth = 420
            maximumHeight = h
            width = 420
            height = h
        }
        loader.setSource(pageFile(name))
    }
    Component.onCompleted: showPage(shell.page)
    Connections { target: shell; function onPageChanged() { window.showPage(shell.page) } }

    Loader {
        id: loader
        anchors.fill: parent
        enabled: shell.notice.length === 0
    }
    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 16
        width: 36
        height: 36
        radius: 18
        visible: shell.page !== "chat"
        enabled: shell.notice.length === 0
        color: modeArea.containsMouse || activeFocus ? Theme.panelAlt : "transparent"
        activeFocusOnTab: visible
        Accessible.role: Accessible.Button
        Accessible.name: Theme.dark ? "切换浅色外观" : "切换深色外观"
        Keys.onReturnPressed: Theme.toggle()
        Keys.onSpacePressed: Theme.toggle()
        AppIcon { anchors.centerIn: parent; name: Theme.dark ? "sun" : "moon"; color: Theme.ink; size: 19 }
        MouseArea { id: modeArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: Theme.toggle() }
    }

    FocusScope {
        anchors.fill: parent
        visible: shell.notice.length > 0
        focus: visible
        z: 100
        Rectangle { anchors.fill: parent; color: Theme.overlay }
        MouseArea { anchors.fill: parent; acceptedButtons: Qt.AllButtons; onWheel: (wheel) => { wheel.accepted = true } }
        Rectangle {
            anchors.centerIn: parent
            width: 352
            height: content.height + 56
            color: Theme.panelBg
            radius: 12
            border.color: Theme.line
            MouseArea { anchors.fill: parent }
            Column {
                id: content
                x: 28; y: 28
                width: parent.width - 56
                spacing: 16
                AppIcon { name: "mail"; size: 28; color: Theme.stamp }
                Text { text: "连接中断了"; font.pixelSize: 22; font.weight: Font.DemiBold; color: Theme.textPrimary }
                Text { width: parent.width; text: shell.notice; font.pixelSize: 14; color: Theme.textSecondary; wrapMode: Text.WordWrap }
                AppButton { width: parent.width; text: "返回登录"; iconName: "arrowRight"; onClicked: shell.dismissNotice() }
            }
        }
    }
}
