import QtQuick

// 联系人资料按字段排列，所有可见信息来自当前好友。
Item {
    id: root

    property var profile: ({})
    signal chatRequested()

    readonly property int sex: profile.sex === undefined ? 0 : profile.sex

    Rectangle { anchors.fill: parent; color: Theme.chatBg }

    Item {
        id: header
        width: parent.width
        height: 74

        Text {
            x: 30
            anchors.verticalCenter: parent.verticalCenter
            text: "联系人资料"
            color: Theme.textPrimary
            font.pixelSize: 16
            font.weight: Font.DemiBold
        }
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: Theme.line
        }
    }

    Flickable {
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        width: parent.width
        contentWidth: width
        contentHeight: details.height + 84
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        Column {
            id: details
            x: Math.max(30, (parent.width - 480) / 2)
            y: 42
            width: Math.min(parent.width - 60, 480)
            spacing: 0

            Row {
                width: parent.width
                spacing: 20

                Avatar {
                    width: 76
                    height: 76
                    source: root.profile.icon || ""
                    name: root.profile.name || ""
                    textSize: 26
                }

                Column {
                    width: parent.width - 96
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 9

                    Text {
                        width: parent.width
                        text: root.profile.name || ""
                        textFormat: Text.PlainText
                        color: Theme.textPrimary
                        font.pixelSize: 25
                        font.weight: Font.DemiBold
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    }
                    Text {
                        text: root.profile.uid ? "ID  " + root.profile.uid : ""
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSmall
                        font.letterSpacing: 1
                    }
                }
            }

            Spacer { gap: 32 }
            Rectangle {
                width: parent.width
                height: 1
                color: Theme.line
                Rectangle { width: 28; height: 2; color: Theme.accent }
            }
            Spacer { gap: 12 }

            Repeater {
                model: [
                    { label: "昵称", value: root.profile.nick || "未设置" },
                    { label: "备注", value: root.profile.remark || "未设置" },
                    { label: "性别", value: root.sex === 1 ? "男" : (root.sex === 2 ? "女" : "未设置") }
                ]

                delegate: Item {
                    width: details.width
                    height: Math.max(48, valueText.implicitHeight + 26)

                    Text {
                        x: 0
                        y: 13
                        text: modelData.label
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontBody
                    }
                    Text {
                        id: valueText
                        x: 82
                        y: 13
                        width: parent.width - x
                        text: modelData.value
                        textFormat: Text.PlainText
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontBody
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    }
                }
            }

            Column {
                width: parent.width
                visible: (root.profile.desc || "").length > 0
                spacing: 12
                Text {
                    text: "个人说明"
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSmall
                }
                Text {
                    width: parent.width
                    text: root.profile.desc || ""
                    textFormat: Text.PlainText
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontBody
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                }
                Spacer { gap: 12 }
            }

            Spacer { gap: 12 }
            Rectangle { width: parent.width; height: 1; color: Theme.line }
            Spacer { gap: 28 }

            Row {
                width: parent.width
                spacing: 14

                AppButton {
                    width: Math.min(188, parent.width - 180)
                    height: 46
                    text: "发消息"
                    iconName: "chat"
                    onClicked: root.chatRequested()
                }
                IconButton { kind: "voice"; label: "语音"; available: false; width: 70 }
                IconButton { kind: "video"; label: "视频"; available: false; width: 70 }
            }
        }
    }
}
