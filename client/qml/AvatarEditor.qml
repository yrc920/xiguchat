import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Dialogs
import XiguChat

//系统选图与模态裁剪分开，取消选图或裁剪都不会改变当前头像。
Popup {
    id: root
    objectName: "avatarEditor"
    parent: Overlay.overlay
    property Item returnFocusItem: null
    signal avatarSaved()

    width: Math.min(800, parent ? parent.width - 48 : 800)
    height: Math.min(620, parent ? parent.height - 48 : 620)
    x: parent ? (parent.width - width) / 2 : 0
    y: parent ? (parent.height - height) / 2 : 0
    padding: 0
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape
    Overlay.modal: Rectangle { color: Theme.overlay }

    function chooseImage() {
        if (!root.opened && root.parent && root.parent.Window.window)
            root.returnFocusItem = root.parent.Window.window.activeFocusItem
        fileDialog.open()
    }

    function dismiss() {
        fileDialog.close()
        root.close()
        cropper.clear()
    }

    function restoreEditorFocus() {
        if (cropper.ready)
            cropper.forceActiveFocus()
        else
            pickButton.forceActiveFocus()
    }

    onOpened: restoreEditorFocus()
    onClosed: {
        cropper.clear()
        if (returnFocusItem)
            returnFocusItem.forceActiveFocus()
    }

    FileDialog {
        id: fileDialog
        title: "选择头像图片"
        fileMode: FileDialog.OpenFile
        nameFilters: ["图片 (*.png *.jpg *.jpeg *.bmp *.webp)"]
        onAccepted: {
            cropper.loadImage(selectedFile)
            root.open()
            root.restoreEditorFocus()
        }
        onRejected: {
            if (root.opened)
                root.restoreEditorFocus()
            else if (root.returnFocusItem)
                root.returnFocusItem.forceActiveFocus()
        }
    }

    background: Rectangle {
        color: Theme.panelBg
        radius: 12
        border.color: Theme.line
    }

    contentItem: Item {
        Item {
            id: header
            width: parent.width
            height: 80
            Rectangle { x: 28; width: 38; height: 3; color: Theme.accent }
            Text {
                x: 28
                y: 23
                text: "编辑头像"
                color: Theme.textPrimary
                font.pixelSize: 20
                font.weight: Font.DemiBold
            }
            Text {
                x: 28
                y: 51
                text: "调整取景，让喜欢的部分留在框内"
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSmall
            }
            ToolIcon {
                anchors.right: parent.right
                anchors.rightMargin: 23
                y: 22
                kind: "close"
                usable: true
                Accessible.name: "取消编辑头像"
                onClicked: root.close()
            }
        }

        Item {
            id: body
            x: 28
            y: header.height
            width: parent.width - 56
            height: parent.height - y - footer.height

            Row {
                id: imageRow
                width: parent.width
                height: Math.max(160, body.height - 110)
                spacing: 24

                Rectangle {
                    width: parent.width - 164
                    height: parent.height
                    radius: 8
                    color: Theme.panelAlt
                    border.color: cropper.activeFocus ? Theme.primary : Theme.line

                    ImageCropper {
                        id: cropper
                        objectName: "avatarCropper"
                        anchors.fill: parent
                        anchors.margins: 10
                        backgroundColor: Theme.panelAlt
                        maskColor: Theme.overlay
                        frameColor: Theme.primary
                        activeFocusOnTab: true
                        enabled: ready
                        Accessible.role: Accessible.Pane
                        Accessible.name: "头像裁剪区域，拖动选框或使用方向键移动"
                        onSaved: {
                            root.avatarSaved()
                            root.close()
                        }
                    }

                    Column {
                        visible: !cropper.ready
                        anchors.centerIn: parent
                        spacing: 14
                        AppIcon {
                            anchors.horizontalCenter: parent.horizontalCenter
                            name: "image"
                            size: 36
                            color: Theme.textHint
                        }
                        Text {
                            text: "请选择一张图片"
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontBody
                        }
                    }
                }

                Column {
                    width: 140
                    spacing: 18
                    Text {
                        text: "头像预览"
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSmall
                    }
                    Avatar {
                        width: 128
                        height: 128
                        source: cropper.previewSource
                        name: ""
                        textSize: 30
                    }
                    Text {
                        text: "聊天中的样子"
                        color: Theme.textHint
                        font.pixelSize: 11
                    }
                    Avatar { width: 36; height: 36; source: cropper.previewSource; name: "" }
                }
            }

            Row {
                id: tools
                y: imageRow.height + 12
                spacing: 10
                ToolIcon {
                    kind: "minus"
                    usable: cropper.ready
                    Accessible.name: "缩小头像"
                    onClicked: cropper.zoom(1 / 1.15)
                }
                ToolIcon {
                    kind: "plus"
                    usable: cropper.ready
                    Accessible.name: "放大头像"
                    onClicked: cropper.zoom(1.15)
                }
                ToolIcon {
                    kind: "reset"
                    usable: cropper.ready
                    Accessible.name: "重置裁剪"
                    onClicked: cropper.resetCrop()
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "拖动选框移动 · 拖动边角调整大小"
                    color: Theme.textHint
                    font.pixelSize: 11
                }
            }

            Text {
                y: imageRow.height + 57
                width: parent.width
                text: cropper.error
                visible: text.length > 0
                color: Theme.danger
                font.pixelSize: Theme.fontSmall
                wrapMode: Text.WordWrap
                Accessible.role: Accessible.StaticText
                Accessible.name: text
            }
        }

        Item {
            id: footer
            anchors.bottom: parent.bottom
            width: parent.width
            height: 86
            Rectangle { width: parent.width; height: 1; color: Theme.line }
            AppLink {
                id: pickButton
                x: 28
                anchors.verticalCenter: parent.verticalCenter
                text: cropper.ready ? "选择另一张" : "选择图片"
                Accessible.onPressAction: root.chooseImage()
                onClicked: root.chooseImage()
            }
            Row {
                anchors.right: parent.right
                anchors.rightMargin: 28
                anchors.verticalCenter: parent.verticalCenter
                spacing: 24
                AppLink {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "取消"
                    Accessible.onPressAction: root.close()
                    onClicked: root.close()
                }
                AppButton {
                    width: 136
                    height: 42
                    text: "保存头像"
                    iconName: "check"
                    enabled: cropper.ready
                    onClicked: cropper.saveAvatar()
                }
            }
        }
    }
}
