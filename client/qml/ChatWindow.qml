import QtQuick
import XiguChat

// 聊天工作区：左侧导航 72、会话/联系人栏 280、右侧内容自适应。
// 这里只负责布局与焦点，导航状态和业务动作都交给 ChatBridge。
Item {
    id: root
    onEnabledChanged: if (!enabled) avatarEditor.dismiss()

    readonly property int sidebarWidth: 72
    readonly property int indexWidth: 280
    readonly property int listHeaderHeight: 78

    ChatBridge {
        id: bridge
        objectName: "chatBridge"
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.chatBg
    }

    Row {
        anchors.fill: parent
        enabled: !dialogs.visible

        // 一级导航：从上到下依次是聊天、联系人、设置；底部固定外观切换与本人头像
        Rectangle {
            width: root.sidebarWidth
            height: parent.height
            color: Theme.sidebarBg

            BrandMark {
                y: 26
                anchors.horizontalCenter: parent.horizontalCenter
                size: 32
            }

            // 三个入口共用 SideButton，选中态由 bridge.mode 决定
            Column {
                y: 112
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 8
                SideButton {
                    kind: "chat"
                    active: bridge.mode === ChatBridge.ChatMode
                    onClicked: bridge.showChatMode()
                }
                SideButton {
                    kind: "contact"
                    active: bridge.mode === ChatBridge.ContactMode
                    badge: bridge.applies.pendingCount
                    onClicked: bridge.showContactMode()
                }
                SideButton {
                    kind: "settings"
                    active: bridge.mode === ChatBridge.SettingsMode
                    onClicked: bridge.showSettings()
                }
            }

            // 外观切换放在头像上方，与列表栏保持同一条底部基线
            Rectangle {
                id: appearanceButton
                width: 40
                height: 40
                radius: 8
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: ownAvatar.top
                anchors.bottomMargin: 17
                color: appearanceArea.containsMouse || activeFocus ? Theme.selected : "transparent"
                activeFocusOnTab: true
                Accessible.role: Accessible.Button
                Accessible.name: Theme.dark ? "切换到日间模式" : "切换到夜间模式"
                Accessible.onPressAction: Theme.toggle()
                Keys.onSpacePressed: Theme.toggle()
                Keys.onReturnPressed: Theme.toggle()
                AppIcon {
                    anchors.centerIn: parent
                    name: Theme.dark ? "sun" : "moon"
                    color: Theme.textSecondary
                    size: 20
                }
                MouseArea {
                    id: appearanceArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: Theme.toggle()
                }
            }

            // 本人头像：点击等同于进入设置里的个人资料
            Avatar {
                id: ownAvatar
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 24
                anchors.horizontalCenter: parent.horizontalCenter
                width: 36
                height: 36
                radius: 11
                source: bridge.myIcon
                name: bridge.myName
                activeFocusOnTab: true
                border.width: activeFocus ? 2 : 0
                border.color: Theme.primary
                Accessible.role: Accessible.Button
                Accessible.name: "打开个人资料"
                Accessible.onPressAction: bridge.showSettings()
                Keys.onSpacePressed: bridge.showSettings()
                Keys.onReturnPressed: bridge.showSettings()
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: bridge.showSettings()
                }
            }
            Rectangle {
                anchors.right: parent.right
                width: 1
                height: parent.height
                color: Theme.line
            }
        }

        // 第二栏：标题、搜索框与列表。设置模式下整栏让位给 SettingsPanel
        Rectangle {
            id: indexPanel
            visible: bridge.mode !== ChatBridge.SettingsMode
            width: root.indexWidth
            height: parent.height
            color: Theme.panelBg

            Item {
                id: listHeader
                width: parent.width
                height: root.listHeaderHeight

                // 标题与统计随模式切换，右侧加号只负责把焦点交给搜索框
                Text {
                    x: 22
                    y: 21
                    text: bridge.mode === ChatBridge.ChatMode ? "聊天" : "联系人"
                    color: Theme.textPrimary
                    font.pixelSize: 22
                    font.bold: true
                }
                Text {
                    x: 23
                    y: 51
                    text: bridge.mode === ChatBridge.ChatMode
                        ? bridge.friends.chatCount + " 个会话"
                        : bridge.friends.count + " 位好友"
                    color: Theme.textSecondary
                    font.pixelSize: 11
                }

                Rectangle {
                    anchors.right: parent.right
                    anchors.rightMargin: 18
                    anchors.verticalCenter: parent.verticalCenter
                    width: 30
                    height: 30
                    radius: 15
                    color: searchEntry.containsMouse || activeFocus ? Theme.selected : Theme.panelAlt
                    activeFocusOnTab: true
                    Accessible.role: Accessible.Button
                    Accessible.name: "搜索添加好友"
                    Accessible.onPressAction: searchInput.forceActiveFocus()
                    Keys.onSpacePressed: searchInput.forceActiveFocus()
                    Keys.onReturnPressed: searchInput.forceActiveFocus()
                    AppIcon {
                        anchors.centerIn: parent
                        name: "plus"
                        size: 16
                        color: Theme.primary
                    }
                    MouseArea {
                        id: searchEntry
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: searchInput.forceActiveFocus()
                    }
                }
            }

            // 搜索用户名或 UID：输入只同步文本，回车才发出请求，Esc 清空
            Rectangle {
                id: searchBox
                x: 16
                y: listHeader.height + 2
                width: parent.width - 32
                height: 38
                radius: 7
                color: searchInput.activeFocus ? Theme.fieldBg : Theme.panelAlt
                border.width: 1
                border.color: searchInput.activeFocus ? Theme.primary : "transparent"

                AppIcon {
                    x: 11
                    anchors.verticalCenter: parent.verticalCenter
                    name: "search"
                    size: 16
                    color: Theme.textHint
                }
                TextInput {
                    id: searchInput
                    objectName: "searchInput"
                    anchors.left: parent.left
                    anchors.leftMargin: 35
                    anchors.right: parent.right
                    anchors.rightMargin: clearSearch.visible ? 32 : 10
                    anchors.verticalCenter: parent.verticalCenter
                    height: 28
                    verticalAlignment: TextInput.AlignVCenter
                    text: bridge.searchText
                    font.pixelSize: 12
                    color: Theme.textPrimary
                    selectionColor: Theme.selected
                    selectedTextColor: Theme.textPrimary
                    clip: true
                    selectByMouse: true
                    activeFocusOnTab: true
                    Accessible.name: "搜索用户名或 UID"
                    onTextEdited: bridge.searchText = text
                    onAccepted: bridge.search()
                    Keys.onEscapePressed: bridge.clearSearch()
                }
                Text {
                    x: 35
                    anchors.verticalCenter: parent.verticalCenter
                    visible: searchInput.length === 0 && !searchInput.inputMethodComposing
                    text: "搜索用户名 / UID"
                    color: Theme.textHint
                    font.pixelSize: 12
                }
                Rectangle {
                    id: clearSearch
                    visible: searchInput.length > 0
                    anchors.right: parent.right
                    anchors.rightMargin: 5
                    anchors.verticalCenter: parent.verticalCenter
                    width: 26
                    height: 26
                    radius: 5
                    color: clearArea.containsMouse || activeFocus ? Theme.selected : "transparent"
                    activeFocusOnTab: visible
                    Accessible.role: Accessible.Button
                    Accessible.name: "清空搜索"
                    Accessible.onPressAction: bridge.clearSearch()
                    Keys.onSpacePressed: bridge.clearSearch()
                    Keys.onReturnPressed: bridge.clearSearch()
                    Keys.onEnterPressed: bridge.clearSearch()
                    AppIcon {
                        anchors.centerIn: parent
                        name: "close"
                        size: 13
                        color: Theme.textSecondary
                    }
                    MouseArea {
                        id: clearArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: bridge.clearSearch()
                    }
                }
            }

            // 会话与联系人共用同一个好友模型；聊天模式下没有 thread_id 的条目高度为0，
            // 所以统计会话数量用 friends.chatCount，而不是列表行数
            ListView {
                id: friendsList
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: searchBox.bottom
                anchors.topMargin: 17
                anchors.bottom: parent.bottom
                clip: true
                model: bridge.friends
                spacing: 2
                boundsBehavior: Flickable.StopAtBounds

                // 联系人模式在列表顶部固定「新的朋友」，未处理数量来自申请模型
                header: Item {
                    width: friendsList.width
                    height: bridge.mode === ChatBridge.ContactMode ? 94 : 0
                    visible: bridge.mode === ChatBridge.ContactMode

                    Rectangle {
                        id: newFriendButton
                        x: 12
                        width: parent.width - 24
                        height: 56
                        radius: 7
                        color: bridge.pane === ChatBridge.ApplyFriendPage ? Theme.selected
                               : (newFriendArea.containsMouse || activeFocus ? Theme.panelAlt : "transparent")
                        border.width: 1
                        border.color: activeFocus ? Theme.primary : "transparent"
                        activeFocusOnTab: visible
                        Accessible.role: Accessible.Button
                        Accessible.name: "新的朋友"
                        Accessible.onPressAction: bridge.openApplyList()
                        Keys.onSpacePressed: bridge.openApplyList()
                        Keys.onReturnPressed: bridge.openApplyList()
                        Keys.onEnterPressed: bridge.openApplyList()
                        onActiveFocusChanged: if (activeFocus) friendsList.positionViewAtBeginning()
                        Rectangle {
                            x: 10
                            anchors.verticalCenter: parent.verticalCenter
                            width: 36
                            height: 36
                            radius: 9
                            color: Theme.primarySoft
                            AppIcon {
                                anchors.centerIn: parent
                                name: "mail"
                                size: 19
                                color: Theme.primary
                            }
                        }
                        Text {
                            x: 58
                            anchors.verticalCenter: parent.verticalCenter
                            text: "新的朋友"
                            font.pixelSize: 14
                            color: Theme.textPrimary
                        }
                        Rectangle {
                            visible: bridge.applies.pendingCount > 0
                            anchors.right: parent.right
                            anchors.rightMargin: 12
                            anchors.verticalCenter: parent.verticalCenter
                            width: Math.max(18, pendingLabel.implicitWidth + 10)
                            height: 18
                            radius: 9
                            color: Theme.accent
                            Text {
                                id: pendingLabel
                                anchors.centerIn: parent
                                text: bridge.applies.pendingCount
                                font.pixelSize: 10
                                color: Theme.primaryText
                            }
                        }
                        MouseArea {
                            id: newFriendArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                newFriendButton.forceActiveFocus(Qt.MouseFocusReason)
                                bridge.openApplyList()
                            }
                        }
                    }
                    Text {
                        x: 22
                        y: 71
                        text: "全部联系人"
                        font.pixelSize: 11
                        color: Theme.textHint
                    }
                }

                // 一行两用：聊天模式打开会话并显示最后一条摘要，联系人模式打开资料页；
                // 高度为0的条目在此不可见也不可聚焦
                delegate: Item {
                    id: friendRow
                    width: friendsList.width
                    //聊天模式只显示已有会话的好友；联系人模式全部显示
                    readonly property bool chatMode: bridge.mode === ChatBridge.ChatMode
                    readonly property bool hasThread: model.threadId > 0
                    readonly property bool shown: !chatMode || hasThread
                    height: !shown ? 0 : (chatMode ? 76 : 60)
                    visible: shown
                    readonly property bool chosen: chatMode
                        ? bridge.pane === ChatBridge.ChatPage && bridge.currentUid === model.uid
                        : bridge.pane === ChatBridge.FriendInfoPage && bridge.profile.uid === model.uid
                    activeFocusOnTab: true
                    Accessible.role: Accessible.Button
                    Accessible.name: model.name
                    Accessible.onPressAction: activate()
                    Keys.onSpacePressed: activate()
                    Keys.onReturnPressed: activate()
                    Keys.onEnterPressed: activate()
                    onActiveFocusChanged: {
                        if (activeFocus)
                            friendsList.positionViewAtIndex(index, ListView.Contain)
                    }

                    function activate() {
                        if (chatMode)
                            bridge.openChat(model.uid)
                        else
                            bridge.openProfile(model.uid)
                    }

                    Rectangle {
                        x: 10
                        width: parent.width - 20
                        height: parent.height - 2
                        radius: 7
                        color: friendRow.chosen ? Theme.selected
                               : (friendArea.containsMouse || friendRow.activeFocus ? Theme.panelAlt : "transparent")
                        border.width: 1
                        border.color: friendRow.activeFocus ? Theme.primary : "transparent"
                        Behavior on color { ColorAnimation { duration: 130 } }
                    }
                    Rectangle {
                        x: 10
                        width: 3
                        height: 18
                        radius: 1
                        anchors.verticalCenter: parent.verticalCenter
                        visible: friendRow.chosen
                        color: Theme.accent
                    }
                    Avatar {
                        id: friendAvatar
                        x: 22
                        width: 40
                        height: 40
                        radius: 11
                        anchors.verticalCenter: parent.verticalCenter
                        source: model.icon
                        name: model.name
                    }
                    Column {
                        anchors.left: friendAvatar.right
                        anchors.leftMargin: 12
                        anchors.right: parent.right
                        anchors.rightMargin: 24
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 6
                        Text {
                            width: parent.width
                            text: model.name
                            textFormat: Text.PlainText
                            color: Theme.textPrimary
                            font.pixelSize: 14
                            font.bold: friendRow.chosen
                            elide: Text.ElideRight
                        }
                        Text {
                            visible: friendRow.chatMode
                            width: parent.width
                            text: model.lastText.length > 0 ? model.lastText : "还没有消息"
                            color: Theme.textSecondary
                            font.pixelSize: 12
                            elide: Text.ElideRight
                            maximumLineCount: 1
                            textFormat: Text.PlainText
                        }
                    }
                    MouseArea {
                        id: friendArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            friendRow.forceActiveFocus(Qt.MouseFocusReason)
                            friendRow.activate()
                        }
                    }
                }
            }

            // 空态按模式区分：聊天看真实会话数，联系人看好友总数
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                y: bridge.mode === ChatBridge.ContactMode ? 278 : 210
                visible: bridge.mode === ChatBridge.ChatMode
                    ? bridge.friends.chatCount === 0
                    : bridge.friends.count === 0
                text: bridge.mode === ChatBridge.ChatMode ? "还没有会话" : "还没有好友"
                font.pixelSize: 12
                color: Theme.textHint
            }
            Rectangle {
                anchors.right: parent.right
                width: 1
                height: parent.height
                color: Theme.line
            }
        }

        //设置栏与列表栏同宽，切换模式时左侧不跳动
        SettingsPanel {
            width: root.indexWidth
            height: parent.height
            visible: bridge.mode === ChatBridge.SettingsMode
            controller: bridge
        }

        // 右侧内容区：按 pane 切换页面，实例常驻以保留输入草稿和滚动位置
        Item {
            width: parent.width - root.sidebarWidth - root.indexWidth
            height: parent.height

            Column {
                anchors.centerIn: parent
                spacing: 20
                visible: bridge.pane === ChatBridge.EmptyPane
                PaperCompanion {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 160
                    height: 112
                    compact: true
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "选一位朋友，聊聊吧"
                    color: Theme.textSecondary
                    font.pixelSize: 14
                }
            }

            // 四个页面共用这块区域，只切 visible；聊天页负责提交，清空由外层在成功后调用
            ChatPage {
                id: chatPage
                anchors.fill: parent
                visible: bridge.pane === ChatBridge.ChatPage
                currentUid: bridge.currentUid
                title: bridge.currentName
                messages: bridge.messages
                onProfileRequested: bridge.openProfile(bridge.currentUid)
                onSendRequested: (parts) => {
                    if (bridge.sendParts(parts))
                        chatPage.clearInput()
                }
            }
            FriendInfoPage {
                anchors.fill: parent
                visible: bridge.pane === ChatBridge.FriendInfoPage
                profile: bridge.profile
                onChatRequested: bridge.openChat(bridge.profile.uid)
            }
            ApplyFriendPage {
                anchors.fill: parent
                visible: bridge.pane === ChatBridge.ApplyFriendPage
                applies: bridge.applies
                onAuthRequested: (uid, name, desc) => dialogs.openAuthDialog(uid, name, desc)
            }
            UserInfoPage {
                id: userInfoPage
                anchors.fill: parent
                visible: bridge.pane === ChatBridge.UserInfoPage
                controller: bridge
                onEditAvatarRequested: avatarEditor.chooseImage()
            }
        }
    }

    // 好友表单是覆盖层，打开时禁用底层 Row；头像编辑器独立于页面，避免切页被销毁
    FriendDialogs {
        id: dialogs
        anchors.fill: parent
        controller: bridge
    }

    AvatarEditor {
        id: avatarEditor
        onAvatarSaved: userInfoPage.avatarSaved = true
    }
}
