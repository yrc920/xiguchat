import QtQuick
import XiguChat

// 会话页只管理编辑与呈现，提交内容由外层交给聊天服务。
Item {
    id: root

    property string title: ""
    property var messages: null
    property int currentUid: 0
    signal sendRequested(var parts)
    signal profileRequested()

    ComposerBridge { id: composerBridge }

    onCurrentUidChanged: {
        composerBridge.switchConversation(input.textDocument, currentUid)
        Qt.callLater(messageList.positionViewAtEnd)
    }
    Component.onCompleted: composerBridge.switchConversation(input.textDocument, currentUid)
    onVisibleChanged: if (visible) input.forceActiveFocus()

    Rectangle { anchors.fill: parent; color: Theme.chatBg }

    Rectangle {
        id: header
        width: parent.width
        height: 78
        color: Theme.chatBg

        Column {
            anchors.left: parent.left
            anchors.leftMargin: 28
            anchors.right: profileButton.left
            anchors.rightMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            spacing: 5
            Text {
                width: parent.width
                text: root.title
                textFormat: Text.PlainText
                font.pixelSize: 20
                font.bold: true
                color: Theme.textPrimary
                elide: Text.ElideRight
            }
            Text {
                text: "对话"
                font.pixelSize: 11
                color: Theme.textHint
            }
        }

        Rectangle {
            id: profileButton
            anchors.right: parent.right
            anchors.rightMargin: 24
            anchors.verticalCenter: parent.verticalCenter
            width: 72
            height: 34
            radius: 7
            color: profileArea.containsMouse || activeFocus ? Theme.panelAlt : "transparent"
            activeFocusOnTab: true
            Accessible.role: Accessible.Button
            Accessible.name: "查看好友资料"
            Accessible.onPressAction: root.profileRequested()
            Keys.onSpacePressed: root.profileRequested()
            Keys.onReturnPressed: root.profileRequested()
            AppIcon {
                x: 10
                anchors.verticalCenter: parent.verticalCenter
                name: "user"
                size: 17
                color: Theme.textSecondary
            }
            Text {
                x: 34
                anchors.verticalCenter: parent.verticalCenter
                text: "资料"
                font.pixelSize: 12
                color: Theme.textSecondary
            }
            MouseArea {
                id: profileArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.profileRequested()
            }
        }

        Rectangle {
            x: 28
            anchors.bottom: parent.bottom
            width: parent.width - 56
            height: 1
            color: Theme.line
        }
    }

    ListView {
        id: messageList
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom
        anchors.bottom: composer.top
        anchors.leftMargin: 28
        anchors.rightMargin: 28
        anchors.topMargin: 24
        anchors.bottomMargin: 24
        clip: true
        spacing: 18
        model: root.messages
        boundsBehavior: Flickable.StopAtBounds

        delegate: ChatBubble {
            width: ListView.view.width
            text: model.text
            isSelf: model.isSelf
            avatar: model.avatar
            name: model.name
            imageSource: model.imageSource
            imageWidth: model.imageWidth
            imageHeight: model.imageHeight
        }

        onCountChanged: Qt.callLater(messageList.positionViewAtEnd)
        onContentHeightChanged: Qt.callLater(messageList.positionViewAtEnd)
    }

    Column {
        anchors.centerIn: messageList
        visible: messageList.count === 0
        spacing: 14
        AppIcon {
            anchors.horizontalCenter: parent.horizontalCenter
            name: "chat"
            size: 28
            color: Theme.textHint
        }
        Text {
            text: "还没有消息"
            font.pixelSize: 12
            color: Theme.textHint
        }
    }

    Rectangle {
        id: composer
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 24
        anchors.rightMargin: 24
        anchors.bottomMargin: 22
        height: 174
        radius: 10
        color: Theme.fieldBg
        border.width: 1
        border.color: input.activeFocus ? Theme.primary : Theme.line
        Behavior on border.color { ColorAnimation { duration: 130 } }

        Row {
            id: tools
            x: 10
            y: 7
            spacing: 3
            ToolIcon { kind: "emoji" }
            ToolIcon { kind: "folder" }
        }

        Text {
            anchors.left: tools.right
            anchors.leftMargin: 12
            anchors.right: parent.right
            anchors.rightMargin: 14
            y: 15
            text: composerBridge.errorText
            visible: text.length > 0
            elide: Text.ElideRight
            color: Theme.danger
            font.pixelSize: 11
        }

        Flickable {
            id: inputView
            x: 16
            y: 44
            width: parent.width - 32
            height: parent.height - 96
            contentWidth: width
            contentHeight: Math.max(input.contentHeight, height)
            clip: true
            interactive: false
            boundsBehavior: Flickable.StopAtBounds

            WheelHandler {
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                onWheel: (event) => {
                    const maxY = Math.max(0, inputView.contentHeight - inputView.height)
                    inputView.contentY = Math.min(Math.max(inputView.contentY - event.angleDelta.y, 0), maxY)
                }
            }

            TextEdit {
                id: input
                objectName: "messageInput"
                width: inputView.width
                height: Math.max(contentHeight, inputView.height)
                textFormat: TextEdit.RichText
                selectByMouse: true
                activeFocusOnTab: true
                Accessible.name: "消息输入框"
                wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
                font.pixelSize: Theme.fontBody
                color: Theme.textPrimary
                selectionColor: Theme.selected
                selectedTextColor: Theme.textPrimary
                onCursorRectangleChanged: root.followCursor()

                Keys.onPressed: (event) => {
                    if (event.matches(StandardKey.Paste)) {
                        cursorPosition = composerBridge.paste(textDocument, selectionStart, selectionEnd)
                        event.accepted = true
                        return
                    }
                    if (inputMethodComposing)
                        return
                    if (event.key === Qt.Key_Tab || event.key === Qt.Key_Backtab) {
                        const forward = event.key === Qt.Key_Tab && !(event.modifiers & Qt.ShiftModifier)
                        const target = input.nextItemInFocusChain(forward)
                        if (target)
                            target.forceActiveFocus(forward ? Qt.TabFocusReason : Qt.BacktabFocusReason)
                        event.accepted = true
                        return
                    }
                    if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                        if (event.modifiers & Qt.ShiftModifier)
                            return
                        event.accepted = true
                        root.submit()
                    }
                }
            }
            Text {
                visible: input.length === 0 && !input.inputMethodComposing
                text: "写点什么…"
                color: Theme.textHint
                font.pixelSize: Theme.fontBody
            }
        }

        DropArea {
            anchors.fill: parent
            onEntered: (drag) => { drag.accepted = drag.hasUrls }
            onDropped: (drop) => {
                for (const url of drop.urls)
                    input.cursorPosition = composerBridge.insertImage(input.textDocument, input.cursorPosition, url)
                input.forceActiveFocus()
                drop.acceptProposedAction()
            }
        }

        Text {
            x: 16
            anchors.verticalCenter: sendButton.verticalCenter
            text: "Enter 发送 · Shift+Enter 换行"
            color: Theme.textHint
            font.pixelSize: 10
        }
        AppButton {
            id: sendButton
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 11
            width: 86
            height: 34
            text: "发送"
            iconName: "send"
            enabled: input.length > 0 && !input.inputMethodComposing
            onClicked: {
                root.submit()
                input.forceActiveFocus()
            }
        }
    }

    function followCursor() {
        const rect = input.cursorRectangle
        if (!rect || rect.height <= 0)
            return
        const maxY = Math.max(0, inputView.contentHeight - inputView.height)
        let y = inputView.contentY
        if (rect.y + rect.height > y + inputView.height)
            y = rect.y + rect.height - inputView.height + 4
        else if (rect.y < y)
            y = rect.y - 4
        inputView.contentY = Math.min(Math.max(y, 0), maxY)
    }

    function submit() {
        root.sendRequested(composerBridge.parts(input.textDocument))
    }

    // 仅在外层接受提交后清空，断线时原文仍留在编辑区。
    function clearInput() {
        input.clear()
        inputView.contentY = 0
    }
}
