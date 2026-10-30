import QtQuick

// 侧栏入口，选中状态用左侧短线与浅底色区分。
Rectangle {
    id: root

    property bool active: false
    property int badge: 0
    property string kind: "chat"
    signal clicked()

    width: 52
    height: 62
    radius: 8
    activeFocusOnTab: true
    color: active ? Theme.selected : (area.containsMouse ? Theme.panelAlt : "transparent")
    border.width: activeFocus ? 1 : 0
    border.color: Theme.primary
    Accessible.role: Accessible.Button
    Accessible.name: kind === "chat" ? "聊天" : "联系人"
    Accessible.onPressAction: root.clicked()
    Keys.onSpacePressed: root.clicked()
    Keys.onReturnPressed: root.clicked()

    Behavior on color { ColorAnimation { duration: 150 } }

    Rectangle {
        x: -10
        y: 21
        width: 3
        height: 20
        radius: 1.5
        color: Theme.accent
        visible: root.active
    }

    AppIcon {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 10
        size: 22
        name: root.kind === "chat" ? "chat" : "contacts"
        color: root.active ? Theme.primary : Theme.textSecondary
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 38
        text: root.kind === "chat" ? "聊天" : "联系人"
        color: root.active ? Theme.textPrimary : Theme.textSecondary
        font.pixelSize: 10
        font.bold: root.active
    }

    Rectangle {
        visible: root.badge > 0
        x: parent.width - width + 1
        y: 3
        width: Math.max(16, countLabel.implicitWidth + 8)
        height: 16
        radius: 8
        color: Theme.accent
        Text {
            id: countLabel
            anchors.centerIn: parent
            text: root.badge > 99 ? "99+" : root.badge
            color: Theme.primaryText
            font.pixelSize: 9
            font.bold: true
        }
    }

    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            root.forceActiveFocus()
            root.clicked()
        }
    }
}
