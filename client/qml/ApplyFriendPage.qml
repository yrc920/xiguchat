import QtQuick

// 好友申请只在服务端确认后变为已添加。
Item {
    id: root

    property var applies: null
    signal authRequested(int uid, string name, string desc)

    Rectangle { anchors.fill: parent; color: Theme.chatBg }

    Item {
        id: header
        width: parent.width
        height: 74

        Column {
            x: 30
            anchors.verticalCenter: parent.verticalCenter
            spacing: 5
            Text {
                text: "新的朋友"
                color: Theme.textPrimary
                font.pixelSize: 16
                font.weight: Font.DemiBold
            }
            Text {
                text: root.applies && root.applies.pendingCount > 0
                      ? root.applies.pendingCount + " 条申请待处理" : "好友申请"
                color: Theme.textSecondary
                font.pixelSize: 11
            }
        }
        AppIcon {
            anchors.right: parent.right
            anchors.rightMargin: 30
            anchors.verticalCenter: parent.verticalCenter
            name: "contacts"
            color: Theme.textSecondary
            size: 22
        }
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: Theme.line
        }
    }

    ListView {
        id: requests
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.topMargin: 14
        anchors.bottomMargin: 16
        x: 22
        width: parent.width - 44
        model: root.applies
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        delegate: Rectangle {
            width: requests.width
            height: 92
            color: hover.hovered ? Theme.panelAlt : "transparent"
            HoverHandler { id: hover }

            Avatar {
                id: avatar
                x: 10
                anchors.verticalCenter: parent.verticalCenter
                width: 44
                height: 44
                source: model.icon
                name: model.name
                textSize: 16
            }
            Column {
                anchors.left: avatar.right
                anchors.leftMargin: 14
                anchors.right: action.left
                anchors.rightMargin: 18
                anchors.verticalCenter: parent.verticalCenter
                spacing: 6

                Text {
                    width: parent.width
                    text: model.name
                    textFormat: Text.PlainText
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontBody
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                }
                Text {
                    width: parent.width
                    text: model.desc.length > 0 ? model.desc : "请求添加你为好友"
                    textFormat: Text.PlainText
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSmall
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    maximumLineCount: 2
                    elide: Text.ElideRight
                }
            }
            Item {
                id: action
                anchors.right: parent.right
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                width: 78
                height: 34

                AppButton {
                    anchors.fill: parent
                    visible: model.status === 0
                    text: "添加"
                    onClicked: root.authRequested(model.uid, model.name, model.desc)
                }
                Row {
                    anchors.centerIn: parent
                    visible: model.status !== 0
                    spacing: 5
                    AppIcon { name: "check"; size: 14; color: Theme.textHint }
                    Text { text: "已添加"; color: Theme.textHint; font.pixelSize: Theme.fontSmall }
                }
            }
            Rectangle {
                anchors.bottom: parent.bottom
                x: 10
                width: parent.width - 20
                height: 1
                color: Theme.line
            }
        }
    }

    Column {
        anchors.centerIn: parent
        width: parent.width - 60
        spacing: 14
        visible: !root.applies || root.applies.count === 0
        PaperCompanion {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 150
            height: 108
            compact: true
        }
        Text {
            width: parent.width
            text: "还没有好友申请"
            horizontalAlignment: Text.AlignHCenter
            color: Theme.textSecondary
            font.pixelSize: Theme.fontBody
        }
    }
}
