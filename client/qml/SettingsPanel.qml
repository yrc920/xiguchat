import QtQuick

//设置索引沿用聊天列表的宽度，当前选项对应右侧个人资料。
Rectangle {
    id: root
    property var controller

    color: Theme.panelBg
    implicitWidth: 280

    Text {
        x: 22
        y: 21
        text: "设置"
        color: Theme.textPrimary
        font.pixelSize: 22
        font.bold: true
    }
    Text {
        x: 23
        y: 51
        text: "资料与外观"
        color: Theme.textSecondary
        font.pixelSize: 11
    }

    Rectangle {
        x: 12
        y: 96
        width: parent.width - 24
        height: 64
        radius: 8
        color: Theme.selected

        AppIcon {
            x: 17
            anchors.verticalCenter: parent.verticalCenter
            name: "user"
            size: 22
            color: Theme.primary
        }
        Column {
            x: 54
            width: parent.width - x - 28
            anchors.verticalCenter: parent.verticalCenter
            spacing: 5
            Text {
                text: "个人资料"
                color: Theme.textPrimary
                font.pixelSize: Theme.fontBody
                font.weight: Font.DemiBold
            }
            Text {
                text: "头像与账号信息"
                color: Theme.textSecondary
                font.pixelSize: 11
            }
        }
    }

    Rectangle {
        x: 22
        y: 184
        width: parent.width - 44
        height: 1
        color: Theme.line
    }

    Rectangle {
        id: appearance
        x: 12
        y: 201
        width: parent.width - 24
        height: 58
        radius: 8
        color: appearanceArea.containsMouse || activeFocus ? Theme.panelAlt : "transparent"
        border.width: activeFocus ? 1 : 0
        border.color: Theme.primary
        activeFocusOnTab: true
        Accessible.role: Accessible.Button
        Accessible.name: Theme.dark ? "切换到日间外观" : "切换到夜间外观"
        Accessible.onPressAction: Theme.toggle()
        Keys.onSpacePressed: Theme.toggle()
        Keys.onReturnPressed: Theme.toggle()

        AppIcon {
            x: 17
            anchors.verticalCenter: parent.verticalCenter
            name: Theme.dark ? "moon" : "sun"
            size: 21
            color: Theme.textSecondary
        }
        Text {
            x: 54
            anchors.verticalCenter: parent.verticalCenter
            text: "外观"
            color: Theme.textPrimary
            font.pixelSize: Theme.fontBody
        }
        Text {
            anchors.right: parent.right
            anchors.rightMargin: 17
            anchors.verticalCenter: parent.verticalCenter
            text: Theme.dark ? "夜间" : "日间"
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSmall
        }
        MouseArea {
            id: appearanceArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                appearance.forceActiveFocus()
                Theme.toggle()
            }
        }
    }

    Rectangle {
        anchors.right: parent.right
        width: 1
        height: parent.height
        color: Theme.line
    }
}
