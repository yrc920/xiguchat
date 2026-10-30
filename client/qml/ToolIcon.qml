import QtQuick

// 编辑区工具入口，未实现的动作保持不可用。
Rectangle {
    id: root

    property string kind: "emoji"
    property bool usable: false
    signal clicked()

    width: 30
    height: 30
    radius: 6
    activeFocusOnTab: usable
    color: usable && (area.containsMouse || activeFocus) ? Theme.panelAlt : "transparent"
    opacity: usable ? 1 : 0.42
    Accessible.role: Accessible.Button
    Accessible.name: kind === "emoji" ? "表情" : "文件"
    Accessible.onPressAction: if (root.usable) root.clicked()
    Keys.onSpacePressed: if (usable) clicked()
    Keys.onReturnPressed: if (usable) clicked()

    AppIcon {
        anchors.centerIn: parent
        size: 19
        name: root.kind
        color: Theme.textSecondary
    }

    MouseArea {
        id: area
        anchors.fill: parent
        enabled: root.usable
        hoverEnabled: true
        cursorShape: root.usable ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: root.clicked()
    }
}
