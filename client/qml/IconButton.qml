import QtQuick

// 带文字的图标动作，未接通的功能保持不可用。
FocusScope {
    id: root

    property string kind: "chat"
    property string label: ""
    property bool available: true
    signal clicked()

    width: 88
    height: 76
    activeFocusOnTab: available
    Accessible.role: Accessible.Button
    Accessible.name: label
    Accessible.onPressAction: if (available) clicked()
    Keys.onReturnPressed: if (available) clicked()
    Keys.onSpacePressed: if (available) clicked()

    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        width: 44
        height: 42
        radius: 8
        color: root.available && area.containsMouse ? Theme.selected : Theme.panelAlt
        border.width: root.activeFocus ? 1 : 0
        border.color: Theme.primary
        opacity: root.available ? 1 : 0.45

        AppIcon {
            anchors.centerIn: parent
            name: root.kind === "voice" ? "phone" : root.kind
            size: 21
            color: root.available ? Theme.primary : Theme.textHint
        }
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 51
        text: root.label
        color: root.available ? Theme.textSecondary : Theme.textHint
        font.pixelSize: Theme.fontSmall
    }

    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        enabled: root.available
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            root.forceActiveFocus()
            root.clicked()
        }
    }
}
