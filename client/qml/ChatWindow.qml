import QtQuick
import QtQuick.Layouts
import XiguChat

/**
 * 聊天主界面：侧栏 + 左列（会话 / 联系人）+ 右侧页面栈。
 *
 * 布局分三块，全部由 ChatBridge 驱动，界面自己不保存导航状态：
 *   侧栏   —— chat / contact 两个入口，另有未处理申请的红点计数；
 *   左列   —— 标题 + 搜索框 + 列表（会话列表 / 联系人列表二选一）；
 *   右侧   —— 聊天页 / 好友资料页 / 好友申请页，用 visible 切换，不重建。
 *
 * 搜索、加好友、同意申请都走弹层，不跳页。
 */
Item {
    id: root

    ChatBridge {
        id: bridge
        objectName: "chatBridge"
    }


    Rectangle {
        anchors.fill: parent
        color: Theme.chatBg
    }

    Row {
        enabled: !dialogs.visible
        anchors.fill: parent
        spacing: 0

        // ---------------- 侧栏 ----------------
        Rectangle {
            width: 64
            height: parent.height
            color: Theme.sidebarBg

            // 两个入口：会话 / 联系人。点谁不仅切左列，右侧也跟着换页
            Column {
                anchors.top: parent.top
                anchors.topMargin: 20
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 6

                SideButton {
                    kind: "chat"
                    active: bridge.mode === 0
                    onClicked: bridge.showChatMode()
                }

                SideButton {
                    kind: "contact"
                    active: bridge.mode === 1
                    badge: bridge.applies.pendingCount
                    onClicked: bridge.showContactMode()
                }
            }

            Avatar {
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 18
                anchors.horizontalCenter: parent.horizontalCenter
                width: 34
                height: 34
                color: "#2C2E31"
                textColor: "#FFFFFF"
                source: bridge.myIcon
                name: bridge.myName
            }
        }

        // ---------------- 左列 ----------------
        Rectangle {
            width: 280
            height: parent.height
            color: Theme.panelBg

            Column {
                anchors.fill: parent

                // 标题
                Rectangle {
                    width: parent.width
                    height: 56
                    color: Theme.panelBg

                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: 18
                        anchors.verticalCenter: parent.verticalCenter
                        text: bridge.mode === 0 ? "聊天" : "联系人"
                        color: Theme.textPrimary
                        font.pixelSize: 16
                        font.bold: true
                    }

                    Rectangle {
                        anchors.bottom: parent.bottom
                        width: parent.width
                        height: 1
                        color: Theme.fieldBorder
                    }
                }

                // 搜索框：回车发起查找，结果走弹层；Esc 退出搜索态
                Rectangle {
                    width: parent.width
                    height: 52
                    color: Theme.panelBg

                    Rectangle {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        anchors.topMargin: 8
                        anchors.bottomMargin: 8
                        radius: 9
                        color: searchInput.activeFocus ? "#FFFFFF" : "#F4F2EE"
                        border.width: 1
                        border.color: searchInput.activeFocus ? Theme.primary : "transparent"

                        TextInput {
                            id: searchInput
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            verticalAlignment: TextInput.AlignVCenter
                            font.pixelSize: Theme.fontSmall
                            color: Theme.textPrimary
                            selectionColor: Theme.accentSoft
                            text: bridge.searchText
                            onTextEdited: bridge.searchText = text
                            onAccepted: bridge.search()
                            clip: true

                            Keys.onEscapePressed: bridge.clearSearch()
                        }

                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: 13
                            anchors.verticalCenter: parent.verticalCenter
                            visible: searchInput.text.length === 0
                            text: "搜索用户名或 uid，回车查找"
                            color: Theme.textHint
                            font.pixelSize: Theme.fontSmall
                        }
                    }
                }

                // 列表区
                Item {
                    width: parent.width
                    height: parent.height - 56 - 52

                    // 会话列表：点条目进聊天页。选中态同时要求右侧确实停在这一页
                    ListView {
                        anchors.fill: parent
                        visible: bridge.mode === 0
                        clip: true
                        model: bridge.friends
                        boundsBehavior: Flickable.StopAtBounds

                        delegate: Rectangle {
                            width: ListView.view.width
                            height: 64
                            color: (bridge.pane === 1 && bridge.currentUid === model.uid)
                                   ? "#F1EFEB" : "transparent"

                            Avatar {
                                id: avatar
                                x: 14
                                anchors.verticalCenter: parent.verticalCenter
                                width: 34
                                height: 34
                                radius: 10
                                color: "#E3E0DA"
                                source: model.icon
                                name: model.name
                            }

                            Column {
                                anchors.left: avatar.right
                                anchors.leftMargin: 10
                                anchors.right: parent.right
                                anchors.rightMargin: 12
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 3

                                Text {
                                    width: parent.width
                                    text: model.name
                                    color: Theme.textPrimary
                                    font.pixelSize: Theme.fontBody
                                    elide: Text.ElideRight
                                }
                                Text {
                                    width: parent.width
                                    text: model.lastText.length > 0 ? model.lastText : "还没有消息"
                                    color: Theme.textHint
                                    font.pixelSize: Theme.fontSmall
                                    elide: Text.ElideRight
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: bridge.openChat(model.uid)
                            }
                        }
                    }


                    // 联系人列表：分组标题 + 「新的朋友」条目 + 好友。
                    // 点好友进资料页，点「新的朋友」进申请页
                    ListView {
                        id: contactList
                        anchors.fill: parent
                        visible: bridge.mode === 1
                        clip: true
                        model: bridge.friends
                        boundsBehavior: Flickable.StopAtBounds

                        header: Column {
                            width: contactList.width

                            // 分组标题：把搜索框、「新的朋友」和已有联系人分开
                            Rectangle {
                                width: parent.width
                                height: 28
                                color: Theme.panelBg

                                Text {
                                    anchors.left: parent.left
                                    anchors.leftMargin: 18
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: "新的朋友"
                                    color: Theme.textHint
                                    font.pixelSize: Theme.fontSmall
                                }
                            }

                            Rectangle {
                                width: parent.width
                                height: 56
                                color: bridge.pane === 3
                                       ? "#F1EFEB"
                                       : (newFriendArea.containsMouse ? "#F7F5F2" : Theme.panelBg)

                                Rectangle {
                                    x: 14
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: 32
                                    height: 32
                                    radius: 10
                                    color: "#E8E2D8"
                                    Text {
                                        anchors.centerIn: parent
                                        text: "+"
                                        color: "#8A6A4A"
                                        font.pixelSize: 17
                                    }
                                }

                                Text {
                                    anchors.left: parent.left
                                    anchors.leftMargin: 56
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: bridge.applies.pendingCount > 0
                                          ? "新的朋友（" + bridge.applies.pendingCount + "）"
                                          : "新的朋友"
                                    color: Theme.textPrimary
                                    font.pixelSize: Theme.fontBody
                                }

                                MouseArea {
                                    id: newFriendArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: bridge.openApplyList()
                                }
                            }

                            Rectangle {
                                width: parent.width
                                height: 28
                                color: Theme.panelBg

                                Text {
                                    anchors.left: parent.left
                                    anchors.leftMargin: 18
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: "联系人"
                                    color: Theme.textHint
                                    font.pixelSize: Theme.fontSmall
                                }
                            }
                        }


                        delegate: Rectangle {
                            width: ListView.view.width
                            height: 56
                            color: (bridge.pane === 2 && bridge.profile.uid === model.uid)
                                   ? "#F1EFEB" : "transparent"

                            Avatar {
                                id: avatar2
                                x: 14
                                anchors.verticalCenter: parent.verticalCenter
                                width: 32
                                height: 32
                                radius: 10
                                color: "#E3E0DA"
                                source: model.icon
                                name: model.name
                            }

                            Text {
                                anchors.left: avatar2.right
                                anchors.leftMargin: 10
                                anchors.verticalCenter: parent.verticalCenter
                                text: model.name
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontBody
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: bridge.openProfile(model.uid)
                            }
                        }
                    }

                    // 空态：好友列表为空时提示，避免整列空白
                    Text {
                        anchors.centerIn: parent
                        visible: bridge.friends.count === 0
                        text: "还没有好友"
                        color: Theme.textHint
                        font.pixelSize: Theme.fontSmall
                    }
                }
            }
        }

        // ---------------- 右侧页面栈 ----------------
        Item {
            width: parent.width - 64 - 280
            height: parent.height

            // 右侧空态：还没有选中会话时给一句提示
            Text {
                anchors.centerIn: parent
                visible: bridge.pane === 0
                text: "选择一位好友开始聊天"
                color: Theme.textHint
                font.pixelSize: Theme.fontBody
            }

            // 三页常驻，只切 visible：切走再回来时滚动位置和输入内容都还在
            ChatPage {
                id: chatPage
                currentUid: bridge.currentUid
                anchors.fill: parent
                visible: bridge.pane === 1
                title: bridge.currentName
                messages: bridge.messages
                onSendRequested: (parts) => {
                    if (bridge.sendParts(parts))
                        chatPage.clearInput()
                }
            }

            FriendInfoPage {
                anchors.fill: parent
                visible: bridge.pane === 2
                profile: bridge.profile
                onChatRequested: bridge.openChat(bridge.profile.uid)
            }

            ApplyFriendPage {
                anchors.fill: parent
                visible: bridge.pane === 3
                applies: bridge.applies
                onAuthRequested: (uid, name, desc) => dialogs.openAuthDialog(uid, name, desc)
            }
        }
    }

    FriendDialogs {
        id: dialogs
        anchors.fill: parent
        controller: bridge
    }
}
