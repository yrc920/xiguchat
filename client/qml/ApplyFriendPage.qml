import QtQuick

/**
 * 好友申请页（「新的朋友」）。
 *
 * 每条是：头像 + 申请人名字 + 申请信息，右侧是「添加」按钮或「已添加」。
 * 点「添加」不在这里处理——抛给外层弹验证框（要填备注名），
 * 真正变成「已添加」要等服务端回执（UserMgr 把申请状态改成 1 后模型刷新）。
 *
 * 模型由外部传入，需要提供 name / desc / status / uid 四个角色。
 */
Item {
    id: root

    // ApplyListModel：uid / name / nick / icon / desc / status
    property var applies: null

    // 点「添加」时抛出，参数是申请人信息和申请留言
    signal authRequested(int uid, string name, string desc)

    Rectangle {
        anchors.fill: parent
        color: Theme.chatBg
    }

    // 标题栏
    Rectangle {
        id: header
        width: parent.width
        height: 56
        color: Theme.chatBg

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 22
            anchors.verticalCenter: parent.verticalCenter
            text: "新的朋友"
            color: Theme.textPrimary
            font.pixelSize: 15
            font.bold: true
        }

        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: Theme.fieldBorder
        }
    }

    // 申请列表：宽度也设上限，避免「添加」被甩到窗口最右边
    ListView {
        width: Math.min(parent.width - 32, 520)
        x: 16
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.topMargin: 8
        clip: true
        spacing: 2
        model: root.applies
        boundsBehavior: Flickable.StopAtBounds

        delegate: Rectangle {
            width: ListView.view.width
            height: 76
            radius: 10
            color: "transparent"

            Avatar {
                id: avatar
                x: 10
                anchors.verticalCenter: parent.verticalCenter
                width: 42
                height: 42
                source: model.icon
                name: model.name
                textSize: 15
            }

            // 名字 + 申请留言。留言为空时给一句默认文案，避免右边空一块
            Column {
                anchors.left: avatar.right
                anchors.leftMargin: 12
                anchors.right: actionArea.left
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                spacing: 4

                Text {
                    width: parent.width
                    text: model.name
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontBody
                    elide: Text.ElideRight
                }

                Text {
                    width: parent.width
                    text: model.desc.length > 0 ? model.desc : "请求添加你为好友"
                    color: Theme.textHint
                    font.pixelSize: Theme.fontSmall
                    elide: Text.ElideRight
                }
            }

            // 右侧动作：待处理显示按钮，处理完换成文字
            Item {
                id: actionArea
                anchors.right: parent.right
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                width: 66
                height: 30

                Rectangle {
                    anchors.fill: parent
                    radius: 8
                    visible: model.status === 0
                    color: addArea.containsMouse ? Theme.primaryHover : Theme.primary

                    Text {
                        anchors.centerIn: parent
                        text: "添加"
                        color: "#FFFFFF"
                        font.pixelSize: Theme.fontSmall
                    }

                    MouseArea {
                        id: addArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.authRequested(model.uid, model.name, model.desc)
                    }
                }

                Text {
                    anchors.centerIn: parent
                    visible: model.status !== 0
                    text: "已添加"
                    color: Theme.textHint
                    font.pixelSize: Theme.fontSmall
                }
            }
        }
    }

    // 空态：一条申请都没有时提示
    Text {
        anchors.centerIn: parent
        visible: !root.applies || root.applies.count === 0
        text: "还没有收到好友申请"
        color: Theme.textHint
        font.pixelSize: Theme.fontSmall
    }
}
