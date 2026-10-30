import QtQuick
import XiguChat

/**
 * 会话页：标题 + 消息流 + 输入区。
 *
 * 三块从上到下排：顶部是对方名字，中间是消息列表，底部是输入区。
 * 页面本身不持有会话状态，标题和消息模型都由外部传进来，
 * 发送动作也只用信号抛出去——这样它可以被任何地方复用。
 *
 * 消息从上往下排，超过一屏后自动滚到最新一条；消息少时贴着顶部，
 * 不会沉到底部。
 */
Item {
    id: root

    // 顶部显示的对方名字（昵称优先，由 ChatBridge 决定）
    property string title: ""

    // 消息列表模型，需要提供 text / isSelf 两个角色
    property var messages: null

    // 用户按下 Enter 时抛出，由外层负责真正发送
    signal sendRequested(var parts)
    property int currentUid: 0
    ComposerBridge { id: composerBridge }
    onCurrentUidChanged: composerBridge.switchConversation(input.textDocument, currentUid)
    Component.onCompleted: composerBridge.switchConversation(input.textDocument, currentUid)

    // 切到本页就把焦点交给输入框，省一次点击
    onVisibleChanged: if (visible) input.forceActiveFocus()

    // 底色：整页统一用聊天气泡的浅暖底
    Rectangle {
        anchors.fill: parent
        color: Theme.chatBg
    }

    // ---------------- 标题栏 ----------------
    Rectangle {
        id: header
        width: parent.width
        height: 56
        color: Theme.chatBg

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 22
            anchors.verticalCenter: parent.verticalCenter
            text: root.title
            color: Theme.textPrimary
            font.pixelSize: 15
            font.bold: true
        }

        // 1px 分隔线，把标题和消息区分开
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: Theme.fieldBorder
        }
    }

    // ---------------- 消息流 ----------------
    ListView {
        id: messageList
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom
        anchors.bottom: composer.top
        anchors.margins: 20
        clip: true
        spacing: 14
        model: root.messages
        boundsBehavior: Flickable.StopAtBounds

        // 每条消息一个气泡；宽度给满，左右对齐由气泡自己按 isSelf 决定
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

        // 新消息进来后跟到底部。用 callLater 是因为此时布局还没算完，
        // 立即滚动会滚到旧的位置；内容不足一屏时 positionViewAtEnd 不会移动
        onCountChanged: Qt.callLater(messageList.positionViewAtEnd)
        onContentHeightChanged: Qt.callLater(messageList.positionViewAtEnd)
    }

    // ---------------- 输入区 ----------------
    Item {
        id: composer
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 148

        // 图片可拖入或粘贴，仅在本地预览；文本通过聊天服务发送。
        Row {
            id: tools
            x: 22
            y: 0
            height: 24
            spacing: 16

            ToolIcon { kind: "emoji" }
            ToolIcon { kind: "folder" }
            Text {
                visible: composerBridge.errorText.length > 0
                text: composerBridge.errorText
                color: Theme.danger
                font.pixelSize: Theme.fontSmall
            }
        }

        // 输入框外框：聚焦时描边转近黑
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.top: tools.bottom
            anchors.topMargin: 8
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            anchors.bottomMargin: 16
            radius: 12
            color: "#FFFFFF"
            border.width: 1
            border.color: input.activeFocus ? Theme.primary : Theme.fieldBorder

            // TextEdit 自身不会滚动，外面套一层 Flickable：
            // 文字超过可视高度时跟着光标走，光标始终留在可见范围内
            Flickable {
                id: inputView
                anchors.fill: parent
                anchors.margins: 12
                anchors.rightMargin: 16
                clip: true
                contentWidth: width
                contentHeight: Math.max(input.contentHeight, height)
                boundsBehavior: Flickable.StopAtBounds
                interactive: false // 拖拽留给文字选择，不抢手势

                // 内容超长时允许滚轮回看（interactive 关掉后滚轮不再生效，这里补上）
                WheelHandler {
                    acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad

                    onWheel: (event) => {
                        const maxY = Math.max(0, inputView.contentHeight - inputView.height)
                        inputView.contentY = Math.min(
                            Math.max(inputView.contentY - event.angleDelta.y, 0), maxY)
                    }
                }

                TextEdit {
                    id: input
                    width: inputView.width
                    height: Math.max(contentHeight, inputView.height)
                    textFormat: TextEdit.RichText
                    selectByMouse: true
                    wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere // 长英文/数字也要断开
                    font.pixelSize: Theme.fontBody
                    color: Theme.textPrimary
                    selectionColor: Theme.accentSoft
                    selectedTextColor: Theme.textPrimary

                    // 光标移动（输入、换行、点击）都可能跑出可视区，交给 followCursor 处理
                    onCursorRectangleChanged: root.followCursor()

                    Keys.onPressed: (event) => {
                        if (event.matches(StandardKey.Paste)) {
                            cursorPosition = composerBridge.paste(textDocument, selectionStart, selectionEnd)
                            event.accepted = true
                            return
                        }
                        if (inputMethodComposing)
                            return
                        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                            // Shift+Enter 留给换行：不拦截，交给 TextEdit 自己处理
                            if (event.modifiers & Qt.ShiftModifier)
                                return
                            event.accepted = true
                            root.submit()
                        }
                    }
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

            // 提示放右下角：中文输入法的组合串出现在左上角，两者不会重叠。
            // 组合期间（inputMethodComposing）也隐藏，避免压住候选词
            Text {
                anchors.right: parent.right
                anchors.rightMargin: 14
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 9
                visible: input.length === 0 && !input.inputMethodComposing
                text: "Enter 发送 · Shift+Enter 换行"
                color: Theme.textHint
                font.pixelSize: Theme.fontSmall
            }
        }
    }

    /**
     * 让光标保持在可视区内。
     *
     * 光标超过下边界就把视图往下滚（多留 6px，别贴着边），
     * 回退到上边界以上就往上滚。最后夹在 [0, 最大可滚范围] 之间，
     * 否则滚过头会出现空白。
     */
    function followCursor() {
        const rect = input.cursorRectangle
        if (!rect || rect.height <= 0)
            return

        const viewHeight = inputView.height
        const maxY = Math.max(0, inputView.contentHeight - viewHeight)
        let y = inputView.contentY

        if (rect.y + rect.height > y + viewHeight)
            y = rect.y + rect.height - viewHeight + 6
        else if (rect.y < y)
            y = rect.y - 6

        inputView.contentY = Math.min(Math.max(y, 0), maxY)
    }

    /** 只有上层接受了消息才清空输入，断线时保留草稿。 */
    function submit() {
        root.sendRequested(composerBridge.parts(input.textDocument))
    }

    function clearInput() {
        input.clear()
        inputView.contentY = 0
    }
}
