import QtQuick
import XiguChat

// 好友表单共用一个弹层；标题和操作始终可见，内容按需滚动。
FocusScope {
    id: root
    objectName: "friendDialogs"

    required property ChatBridge controller
    property string overlay: ""
    property int pendingUid: 0
    property string pendingName: ""
    property string pendingDesc: ""
    property string pendingIcon: ""
    property var presetTags: ["同学", "家人", "同事", "朋友", "游戏伙伴", "客户",
                              "读书", "运动", "旅行", "音乐", "电影", "其他"]
    property var selectedTags: []

    visible: overlay !== ""
    focus: visible
    z: 10
    Keys.onEscapePressed: closeDialog()

    readonly property string heading: overlay === "apply" ? "添加朋友"
        : overlay === "auth" ? "通过好友申请"
        : overlay === "found" ? "找到这位朋友" : "没有找到这个用户"
    readonly property string actionText: overlay === "apply" ? "发送申请"
        : overlay === "auth" ? "同意添加"
        : overlay === "found" ? "添加到通讯录" : "知道了"

    function closeDialog() {
        if (overlay === "found" || overlay === "notfound")
            controller.clearSearch()
        overlay = ""
    }

    function openAuthDialog(uid, name, desc) {
        pendingUid = uid
        pendingName = name
        pendingDesc = desc
        pendingIcon = ""
        authRemarkInput.text = ""
        overlay = "auth"
        content.contentY = 0
        authRemarkInput.forceInputFocus()
    }

    function openApplyDialog(uid, name) {
        pendingUid = uid
        pendingName = name
        selectedTags = []
        verifyInput.text = controller.myUserName
        applyRemarkInput.text = name
        tagInput.text = ""
        overlay = "apply"
        content.contentY = 0
        verifyInput.forceInputFocus()
    }

    function addTag(text) {
        const value = text.trim()
        if (value.length > 0 && selectedTags.indexOf(value) < 0)
            selectedTags = selectedTags.concat([value])
    }

    function removeTag(text) {
        const index = selectedTags.indexOf(text)
        if (index < 0)
            return
        const tags = selectedTags.slice()
        tags.splice(index, 1)
        selectedTags = tags
    }

    function toggleTag(text) {
        if (selectedTags.indexOf(text) >= 0)
            removeTag(text)
        else
            addTag(text)
    }

    function submit() {
        if (overlay === "found") {
            const uid = pendingUid
            const name = pendingName
            controller.clearSearch()
            openApplyDialog(uid, name)
        } else if (overlay === "apply") {
            controller.applyTo(pendingUid, verifyInput.text || controller.myUserName,
                               applyRemarkInput.text || pendingName)
            overlay = ""
        } else if (overlay === "auth") {
            controller.authApply(pendingUid, authRemarkInput.text)
            overlay = ""
        } else {
            closeDialog()
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.overlay
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
            onClicked: root.closeDialog()
            onWheel: (wheel) => { wheel.accepted = true }
        }
    }

    Rectangle {
        id: panel
        anchors.centerIn: parent
        width: Math.min(440, root.width - 48)
        height: Math.min(root.height - 48, body.height + 170)
        radius: 12
        color: Theme.panelBg
        border.width: 1
        border.color: Theme.line
        clip: true

        MouseArea { anchors.fill: parent; acceptedButtons: Qt.AllButtons }

        Rectangle { x: 28; y: 0; width: 38; height: 3; color: Theme.accent }

        Item {
            id: header
            width: parent.width
            height: 76
            Text {
                x: 28
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - 92
                text: root.heading
                font.pixelSize: 20
                font.weight: Font.DemiBold
                color: Theme.textPrimary
                elide: Text.ElideRight
            }
            FocusScope {
                id: closeButton
                anchors.right: parent.right
                anchors.rightMargin: 20
                anchors.verticalCenter: parent.verticalCenter
                width: 32
                height: 32
                activeFocusOnTab: true
                Accessible.role: Accessible.Button
                Accessible.name: "关闭"
                Accessible.onPressAction: root.closeDialog()
                Keys.onReturnPressed: root.closeDialog()
                Keys.onSpacePressed: root.closeDialog()
                Rectangle {
                    anchors.fill: parent
                    radius: 6
                    color: closeArea.containsMouse || closeButton.activeFocus ? Theme.panelAlt : "transparent"
                }
                AppIcon { anchors.centerIn: parent; name: "close"; size: 18; color: Theme.textSecondary }
                MouseArea {
                    id: closeArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.closeDialog()
                }
            }
        }

        Flickable {
            id: content
            x: 28
            anchors.top: header.bottom
            anchors.bottom: footer.top
            width: parent.width - 56
            contentWidth: width
            contentHeight: body.height
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            Column {
                id: body
                width: content.width
                spacing: 0

                Row {
                    width: parent.width
                    spacing: 14
                    visible: root.overlay !== "notfound"
                    Avatar {
                        width: 52
                        height: 52
                        source: root.pendingIcon
                        name: root.pendingName
                        textSize: 19
                    }
                    Column {
                        width: parent.width - 66
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 6
                        Text {
                            width: parent.width
                            text: root.pendingName
                            textFormat: Text.PlainText
                            color: Theme.textPrimary
                            font.pixelSize: 17
                            font.weight: Font.DemiBold
                            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        }
                        Text {
                            text: "ID  " + root.pendingUid
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSmall
                            font.letterSpacing: 0.5
                        }
                    }
                }

                Column {
                    width: parent.width
                    visible: root.overlay === "found" || root.overlay === "auth"
                    spacing: 16
                    Spacer { gap: 4 }
                    Text {
                        width: parent.width
                        text: root.pendingDesc || (root.overlay === "auth" ? "请求添加你为好友" : "")
                        textFormat: Text.PlainText
                        visible: text.length > 0
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontBody
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    }
                    Rectangle { width: parent.width; height: 1; color: Theme.line }
                    Spacer { gap: 2 }
                }

                Column {
                    width: parent.width
                    visible: root.overlay === "apply"
                    spacing: 0
                    Spacer { gap: 24 }
                    Text { text: "申请留言"; color: Theme.textSecondary; font.pixelSize: Theme.fontSmall }
                    Spacer { gap: 8 }
                    AppInput {
                        id: verifyInput
                        width: parent.width
                        placeholder: "介绍一下自己"
                        onAccepted: applyRemarkInput.forceInputFocus()
                    }
                    Spacer { gap: 18 }
                    Text { text: "备注名"; color: Theme.textSecondary; font.pixelSize: Theme.fontSmall }
                    Spacer { gap: 8 }
                    AppInput {
                        id: applyRemarkInput
                        width: parent.width
                        placeholder: "你习惯怎么称呼对方"
                        onAccepted: tagInput.forceInputFocus()
                    }
                    Spacer { gap: 20 }
                    Text { text: "标签"; color: Theme.textSecondary; font.pixelSize: Theme.fontSmall }
                    Spacer { gap: 10 }

                    Flow {
                        id: selectedFlow
                        width: parent.width
                        spacing: 7
                        visible: root.selectedTags.length > 0
                        Repeater {
                            model: root.selectedTags
                            delegate: Rectangle {
                                width: Math.min(selectedFlow.width, selectedText.implicitWidth + 42)
                                height: 29
                                radius: 5
                                color: Theme.selected
                                Text {
                                    id: selectedText
                                    x: 10
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: parent.width - 35
                                    text: modelData
                                    textFormat: Text.PlainText
                                    color: Theme.textPrimary
                                    font.pixelSize: Theme.fontSmall
                                    elide: Text.ElideRight
                                }
                                AppIcon {
                                    anchors.right: parent.right
                                    anchors.rightMargin: 7
                                    anchors.verticalCenter: parent.verticalCenter
                                    name: "close"
                                    size: 12
                                    color: Theme.textSecondary
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.removeTag(modelData)
                                }
                            }
                        }
                    }
                    Spacer { gap: root.selectedTags.length > 0 ? 10 : 0 }
                    AppInput {
                        id: tagInput
                        width: parent.width
                        placeholder: "输入标签后按回车"
                        onAccepted: {
                            root.addTag(text)
                            text = ""
                        }
                    }
                    Spacer { gap: 12 }
                    Flow {
                        width: parent.width
                        spacing: 7
                        Repeater {
                            model: root.presetTags
                            delegate: Rectangle {
                                readonly property bool chosen: root.selectedTags.indexOf(modelData) >= 0
                                width: tagLabel.implicitWidth + 22
                                height: 29
                                radius: 5
                                color: chosen ? Theme.selected : Theme.panelAlt
                                border.width: 1
                                border.color: chosen ? Theme.primary : "transparent"
                                Text {
                                    id: tagLabel
                                    anchors.centerIn: parent
                                    text: modelData
                                    color: chosen ? Theme.textPrimary : Theme.textSecondary
                                    font.pixelSize: Theme.fontSmall
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.toggleTag(modelData)
                                }
                            }
                        }
                    }
                    Spacer { gap: 4 }
                }

                Column {
                    width: parent.width
                    visible: root.overlay === "auth"
                    spacing: 8
                    Text { text: "备注名"; color: Theme.textSecondary; font.pixelSize: Theme.fontSmall }
                    AppInput {
                        id: authRemarkInput
                        width: parent.width
                        placeholder: "可选"
                        onAccepted: root.submit()
                    }
                }

                Column {
                    width: parent.width
                    visible: root.overlay === "notfound"
                    spacing: 18
                    Rectangle {
                        width: 48
                        height: 48
                        radius: 10
                        color: Theme.panelAlt
                        AppIcon { anchors.centerIn: parent; name: "search"; size: 23; color: Theme.textSecondary }
                    }
                    Text {
                        width: parent.width
                        text: root.controller.searchMessage || "检查一下用户名或 ID，再试一次。"
                        textFormat: Text.PlainText
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontBody
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    }
                }
            }
        }

        Rectangle {
            visible: content.contentHeight > content.height
            width: 2
            x: panel.width - 12
            y: header.height + (content.contentY / Math.max(1, content.contentHeight - content.height))
               * (content.height - height)
            height: Math.max(24, content.height * content.height / Math.max(1, content.contentHeight))
            color: Theme.textHint
            radius: 1
        }

        Item {
            id: footer
            anchors.bottom: parent.bottom
            width: parent.width
            height: 94
            Row {
                anchors.centerIn: parent
                width: parent.width - 56
                spacing: 12
                CodeButton {
                    visible: root.overlay !== "notfound"
                    width: (parent.width - 12) / 2
                    text: "取消"
                    onClicked: root.closeDialog()
                }
                AppButton {
                    width: root.overlay === "notfound" ? parent.width : (parent.width - 12) / 2
                    text: root.actionText
                    onClicked: root.submit()
                }
            }
        }
    }

    Connections {
        target: controller
        function onSearchStateChanged() {
            if (controller.searchState === 2) {
                root.pendingUid = controller.searchResult.uid
                root.pendingName = controller.searchResult.name || ""
                root.pendingDesc = controller.searchResult.desc || ""
                root.pendingIcon = controller.searchResult.icon || ""
                root.overlay = "found"
                content.contentY = 0
                root.forceActiveFocus()
            } else if (controller.searchState === 3) {
                root.pendingUid = 0
                root.pendingName = ""
                root.pendingDesc = ""
                root.pendingIcon = ""
                root.overlay = "notfound"
                content.contentY = 0
                root.forceActiveFocus()
            } else if (controller.searchState === 0
                       && (root.overlay === "found" || root.overlay === "notfound")) {
                root.overlay = ""
            }
        }
    }
}
