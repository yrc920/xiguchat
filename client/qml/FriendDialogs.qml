import QtQuick
import XiguChat

// 搜索结果、添加好友和验证申请的弹层；临时表单状态不进入聊天主布局。
FocusScope {
    id: root
    objectName: "friendDialogs"
    required property ChatBridge controller
    visible: overlay !== ""
    focus: visible
    z: 10

    Keys.onEscapePressed: {
        if (overlay === "found" || overlay === "notfound")
            controller.clearSearch()
        overlay = ""
    }

    function openAuthDialog(uid, name, desc) {
        pendingUid = uid
        pendingName = name
        pendingDesc = desc
        authRemarkInput.text = ""
        overlay = "auth"
        authRemarkInput.forceInputFocus()
    }

    // 当前弹层："" 无 / apply 申请添加好友 / auth 同意申请
    //           found 搜到陌生用户 / notfound 没搜到
    property string overlay: ""
    property int pendingUid: 0
    property string pendingName: ""
    property string pendingDesc: ""

    // 加好友弹窗的标签：presetTags 是可选标签云，selectedTags 是已选中的。
    // 协议里没有标签字段，这两个数组只服务于界面，不参与上报。
    property var presetTags: ["同学", "家人", "同事", "朋友", "游戏伙伴",
                              "客户", "读书", "运动", "旅行", "音乐", "电影", "其他"]
    property var selectedTags: []

    /**
     * 打开「申请添加好友」弹层，并把默认值填好：
     * 申请信息默认填自己的用户名，备注名默认填对方名字，标签清空。
     */
    function openApplyDialog(uid, name) {
        pendingUid = uid
        pendingName = name
        selectedTags = []
        verifyInput.text = controller.myUserName
        applyRemarkInput.text = name
        tagInput.text = ""
        overlay = "apply"
        verifyInput.forceInputFocus()
    }

    /** 把输入框里的文字加进已选标签（去重、去空白），不改变标签云。 */
    function addTag(text) {
        const value = text.trim()
        if (value.length === 0)
            return
        if (selectedTags.indexOf(value) < 0)
            selectedTags = selectedTags.concat([value])
    }

    /** 点标签云里的标签：没选过就加入，已选过就移除。 */
    function toggleTag(text) {
        const at = selectedTags.indexOf(text)
        if (at >= 0) {
            const next = selectedTags.slice()
            next.splice(at, 1)
            selectedTags = next
        } else {
            selectedTags = selectedTags.concat([text])
        }
    }

    /** 点已选标签上的 × 移除。 */
    function removeTag(text) {
        const at = selectedTags.indexOf(text)
        if (at < 0)
            return
        const next = selectedTags.slice()
        next.splice(at, 1)
        selectedTags = next
    }


    Rectangle {
        anchors.fill: parent
        color: "#59000000"
        MouseArea {
            anchors.fill: parent
            onClicked: {
                if (root.overlay === "found" || root.overlay === "notfound")
                    controller.clearSearch()
                root.overlay = ""
            }
        }
    }

    // 申请添加好友：申请信息 + 备注名 + 标签
    Rectangle {
        visible: root.overlay === "apply"
        width: 420
        height: applyBody.height + 52
        anchors.centerIn: parent
        radius: 14
        color: Theme.panelBg

        MouseArea { anchors.fill: parent }

        Column {
            id: applyBody
            x: 24
            y: 26
            width: parent.width - 48
            spacing: 0

            Text {
                text: "申请添加好友"
                color: Theme.textPrimary
                font.pixelSize: 18
                font.bold: true
            }

            Spacer { gap: 4 }

            Text {
                width: parent.width
                text: "向 " + root.pendingName + " 发送好友申请"
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSmall
                elide: Text.ElideRight
            }

            Spacer { gap: 16 }

            Text {
                text: "发送添加朋友申请"
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSmall
            }

            Spacer { gap: 7 }

            AppInput {
                id: verifyInput
                width: parent.width
                placeholder: "说点什么"
            }

            Spacer { gap: 14 }

            Text {
                text: "备注名"
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSmall
            }

            Spacer { gap: 7 }

            AppInput {
                id: applyRemarkInput
                width: parent.width
                placeholder: "给对方的备注"
            }

            Spacer { gap: 14 }

            Text {
                text: "标签"
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSmall
            }

            Spacer { gap: 7 }

            // 已选标签
            Flow {
                width: parent.width
                spacing: 6
                visible: root.selectedTags.length > 0

                Repeater {
                    model: root.selectedTags

                    delegate: Rectangle {
                        height: 26
                        width: chipText.implicitWidth + 34
                        radius: 7
                        color: "#F1EFEB"

                        Text {
                            id: chipText
                            anchors.left: parent.left
                            anchors.leftMargin: 10
                            anchors.verticalCenter: parent.verticalCenter
                            text: modelData
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSmall
                        }

                        Text {
                            anchors.right: parent.right
                            anchors.rightMargin: 9
                            anchors.verticalCenter: parent.verticalCenter
                            text: "×"
                            color: Theme.textHint
                            font.pixelSize: 13
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.removeTag(modelData)
                        }
                    }
                }
            }

            Spacer { gap: root.selectedTags.length > 0 ? 8 : 0 }

            AppInput {
                id: tagInput
                width: parent.width
                placeholder: "输入后回车添加标签"
                onAccepted: {
                    root.addTag(tagInput.text)
                    tagInput.text = ""
                }
            }

            Spacer { gap: 14 }

            Rectangle {
                width: parent.width
                height: 1
                color: Theme.fieldBorder
            }

            Spacer { gap: 12 }

            // 预设标签，超过四行可以滚动
            Flickable {
                width: parent.width
                height: Math.min(presetFlow.height, 116)
                contentHeight: presetFlow.height
                clip: true
                interactive: contentHeight > height
                boundsBehavior: Flickable.StopAtBounds

                Flow {
                    id: presetFlow
                    width: parent.width
                    spacing: 6

                    Repeater {
                        model: root.presetTags

                        delegate: Rectangle {
                            readonly property bool picked: root.selectedTags.indexOf(modelData) >= 0

                            height: 26
                            width: presetText.implicitWidth + 20
                            radius: 7
                            color: picked ? Theme.primary : "#F6F4F0"
                            border.width: 1
                            border.color: picked ? Theme.primary : "transparent"

                            Text {
                                id: presetText
                                anchors.centerIn: parent
                                text: modelData
                                color: picked ? "#FFFFFF" : Theme.textSecondary
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
            }

            Spacer { gap: 20 }

            Row {
                width: parent.width
                spacing: 10

                Rectangle {
                    width: (parent.width - 10) / 2
                    height: 40
                    radius: 10
                    color: cancelArea.containsMouse ? "#F1EFEB" : "transparent"
                    border.width: 1
                    border.color: Theme.fieldBorder

                    Text {
                        anchors.centerIn: parent
                        text: "取消"
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontBody
                    }

                    MouseArea {
                        id: cancelArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.overlay = ""
                    }
                }

                Rectangle {
                    width: (parent.width - 10) / 2
                    height: 40
                    radius: 10
                    color: okArea.containsMouse ? Theme.primaryHover : Theme.primary

                    Text {
                        anchors.centerIn: parent
                        text: "确认"
                        color: "#FFFFFF"
                        font.pixelSize: Theme.fontBody
                    }

                    MouseArea {
                        id: okArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            const verify = verifyInput.text.length > 0
                                         ? verifyInput.text
                                         : controller.myUserName
                            const remark = applyRemarkInput.text.length > 0
                                         ? applyRemarkInput.text
                                         : root.pendingName
                            controller.applyTo(root.pendingUid, verify, remark)
                            root.overlay = ""
                        }
                    }
                }
            }
        }
    }

    // 同意好友申请：填备注
    Rectangle {
        visible: root.overlay === "auth"
        width: 340
        height: authBody.height + 52
        anchors.centerIn: parent
        radius: 14
        color: Theme.panelBg

        MouseArea { anchors.fill: parent }

        Column {
            id: authBody
            x: 24
            y: 26
            width: parent.width - 48
            spacing: 0

            Text {
                text: "验证申请"
                color: Theme.textPrimary
                font.pixelSize: 18
                font.bold: true
            }

            Spacer { gap: 6 }

            Text {
                width: parent.width
                text: root.pendingName + " 请求加你为好友"
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSmall
                wrapMode: Text.WordWrap
            }

            Spacer { gap: 18 }

            AppInput {
                id: authRemarkInput
                width: parent.width
                placeholder: "备注名（可选）"
            }

            Spacer { gap: 20 }

            Row {
                width: parent.width
                spacing: 10

                Rectangle {
                    width: (parent.width - 10) / 2
                    height: 40
                    radius: 10
                    color: authCancelArea.containsMouse ? "#F1EFEB" : "transparent"
                    border.width: 1
                    border.color: Theme.fieldBorder

                    Text {
                        anchors.centerIn: parent
                        text: "取消"
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontBody
                    }

                    MouseArea {
                        id: authCancelArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.overlay = ""
                    }
                }

                Rectangle {
                    width: (parent.width - 10) / 2
                    height: 40
                    radius: 10
                    color: authOkArea.containsMouse ? Theme.primaryHover : Theme.primary

                    Text {
                        anchors.centerIn: parent
                        text: "同意"
                        color: "#FFFFFF"
                        font.pixelSize: Theme.fontBody
                    }

                    MouseArea {
                        id: authOkArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            controller.authApply(root.pendingUid, authRemarkInput.text)
                            root.overlay = ""
                        }
                    }
                }
            }
        }
    }

    // 搜到用户：添加到通讯录
    Rectangle {
        visible: root.overlay === "found"
        width: 300
        height: foundBody.height + 48
        anchors.centerIn: parent
        radius: 14
        color: Theme.panelBg

        MouseArea { anchors.fill: parent }

        Column {
            id: foundBody
            x: 24
            y: 24
            width: parent.width - 48
            spacing: 0

            Rectangle {
                width: 56
                height: 56
                radius: 18
                anchors.horizontalCenter: parent.horizontalCenter
                color: "#E3E0DA"

                Text {
                    anchors.centerIn: parent
                    text: root.pendingName.length > 0
                          ? root.pendingName.substring(0, 1) : "?"
                    color: "#6B6A66"
                    font.pixelSize: 20
                }
            }

            Spacer { gap: 12 }

            Text {
                width: parent.width
                text: root.pendingName
                color: Theme.textPrimary
                font.pixelSize: 16
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
            }

            Spacer { gap: 4 }

            Text {
                width: parent.width
                text: root.pendingDesc.length > 0 ? root.pendingDesc
                                                  : ("uid " + root.pendingUid)
                color: Theme.textHint
                font.pixelSize: Theme.fontSmall
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
            }

            Spacer { gap: 20 }

            Rectangle {
                width: parent.width
                height: 40
                radius: 10
                color: addContactArea.containsMouse ? Theme.primaryHover : Theme.primary

                Text {
                    anchors.centerIn: parent
                    text: "添加到通讯录"
                    color: "#FFFFFF"
                    font.pixelSize: Theme.fontBody
                }

                MouseArea {
                    id: addContactArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        const uid = root.pendingUid
                        const name = root.pendingName
                        controller.clearSearch()
                        root.openApplyDialog(uid, name)
                    }
                }
            }

            Spacer { gap: 10 }

            Rectangle {
                width: parent.width
                height: 40
                radius: 10
                color: closeFoundArea.containsMouse ? "#F1EFEB" : "transparent"

                Text {
                    anchors.centerIn: parent
                    text: "取消"
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontBody
                }

                MouseArea {
                    id: closeFoundArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        controller.clearSearch()
                        root.overlay = ""
                    }
                }
            }
        }
    }

    // 没搜到
    Rectangle {
        visible: root.overlay === "notfound"
        width: 300
        height: notFoundBody.height + 48
        anchors.centerIn: parent
        radius: 14
        color: Theme.panelBg

        MouseArea { anchors.fill: parent }

        Column {
            id: notFoundBody
            x: 24
            y: 26
            width: parent.width - 48
            spacing: 0

            Text {
                width: parent.width
                text: "无法找到该用户"
                color: Theme.textPrimary
                font.pixelSize: 16
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
            }

            Spacer { gap: 8 }

            Text {
                width: parent.width
                text: controller.searchMessage || "请检查你填写的账号是否正确"
                color: Theme.textHint
                font.pixelSize: Theme.fontSmall
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }

            Spacer { gap: 20 }

            Rectangle {
                width: parent.width
                height: 40
                radius: 10
                color: notFoundOkArea.containsMouse ? Theme.primaryHover : Theme.primary

                Text {
                    anchors.centerIn: parent
                    text: "确定"
                    color: "#FFFFFF"
                    font.pixelSize: Theme.fontBody
                }

                MouseArea {
                    id: notFoundOkArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        controller.clearSearch()
                        root.overlay = ""
                    }
                }
            }
        }
    }

    // 搜索结果分派：2 → 弹「添加到通讯录」，3 → 弹「无法找到该用户」。
    // 搜到自己或已经是好友的情况，ChatBridge 内部已经消化掉了，这里收不到
    Connections {
        target: controller

        function onSearchStateChanged() {
            if (controller.searchState === 2) {
                root.pendingUid = controller.searchResult.uid
                root.pendingName = controller.searchResult.name || ""
                root.pendingDesc = controller.searchResult.desc || ""
                root.overlay = "found"
            } else if (controller.searchState === 3) {
                root.pendingUid = 0
                root.pendingName = ""
                root.pendingDesc = ""
                root.overlay = "notfound"
            } else if (controller.searchState === 0
                       && (root.overlay === "found" || root.overlay === "notfound")) {
                root.overlay = ""
            }
        }
    }
}
