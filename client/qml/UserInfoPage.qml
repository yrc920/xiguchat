import QtQuick

// 账号字段由桥接层提供，头像保存后与侧栏及自己的消息同步刷新。
// 布局与好友资料页一致：标题栏 + 限宽 480 的详情列。
Item {
    id: root
    property var controller
    property bool avatarSaved: false
    readonly property var profile: controller ? controller.myProfile : ({})
    signal editAvatarRequested()

    Rectangle {
        anchors.fill: parent
        color: Theme.chatBg
    }

    // 标题栏
    Item {
        id: header
        width: parent.width
        height: 74
        Text {
            x: 30
            anchors.verticalCenter: parent.verticalCenter
            text: "个人资料"
            color: Theme.textPrimary
            font.pixelSize: 16
            font.weight: Font.DemiBold
        }
        // 标题下用一条细线把页头和内容分开
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

            Row {
                width: parent.width
                spacing: 22
                Avatar {
                    width: 88
                    height: 88
                    source: root.controller ? root.controller.myIcon : ""
                    name: root.controller ? root.controller.myName : ""
                    textSize: 30
                }
                Column {
                    width: parent.width - 110
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 10
                    Text {
                        width: parent.width
                        text: root.controller ? root.controller.myName : ""
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

            Spacer { gap: 24 }
            AppButton {
                id: changeAvatarButton
                objectName: "changeAvatarButton"
                width: 146
                height: 42
                text: "更换头像"
                iconName: "image"
                onClicked: {
                    root.avatarSaved = false
                    forceActiveFocus()
                    root.editAvatarRequested()
                }
            }
            Text {
                width: parent.width
                topPadding: 12
                text: "头像已保存"
                visible: root.avatarSaved
                color: Theme.success
                font.pixelSize: Theme.fontSmall
                wrapMode: Text.WordWrap
                Accessible.role: Accessible.StaticText
                Accessible.name: text
            }

            Spacer { gap: 30 }
            Rectangle {
                width: parent.width
                height: 1
                color: Theme.line
                Rectangle {
                    width: 28
                    height: 2
                    color: Theme.accent
                }
            }
            Spacer { gap: 12 }

            Repeater {
                model: [
                    { label: "用户名", value: root.profile.name || "未设置" },
                    { label: "昵称", value: root.profile.nick || "未设置" },
                    { label: "性别", value: root.profile.sex === 1 ? "男" : (root.profile.sex === 2 ? "女" : "未设置") }
                ]
                delegate: Item {
                    width: details.width
                    height: Math.max(48, valueText.implicitHeight + 26)
                    Text {
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
        }
    }
}
