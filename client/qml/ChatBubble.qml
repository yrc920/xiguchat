import QtQuick

// 文字按自然宽度排版，图片使用消息中的尺寸；两种内容都能随窗口收窄。
Item {
    id: root

    property string avatar: ""
    property string name: ""
    property string text: ""
    property bool isSelf: false
    property string imageSource: ""
    property int imageWidth: 0
    property int imageHeight: 0
    readonly property bool localImage: imageSource.length > 0
    readonly property real paddingH: localImage ? 5 : 15
    readonly property real paddingV: localImage ? 5 : 11
    readonly property real availableWidth: Math.max(1, Math.min(Theme.bubbleMaxWidth, width - 102))
    readonly property real contentWidth: localImage
        ? Math.max(1, Math.min(imageWidth, availableWidth - paddingH * 2))
        : Math.max(1, Math.min(Math.ceil(metrics.advanceWidth), availableWidth - paddingH * 2))
    readonly property real bodyWidth: Math.max(36, contentWidth + paddingH * 2)
    readonly property real bodyHeight: (localImage ? preview.height : label.implicitHeight) + paddingV * 2
    readonly property color bubbleColor: isSelf ? Theme.bubbleSelf : Theme.bubbleOther

    implicitHeight: Math.max(34, bodyHeight)

    TextMetrics {
        id: metrics
        font: label.font
        text: root.text
    }

    Avatar {
        id: userAvatar
        x: root.isSelf ? root.width - width : 0
        width: 32
        height: 32
        radius: 10
        source: root.avatar
        name: root.name
    }

    Rectangle {
        id: body
        x: root.isSelf ? userAvatar.x - 11 - width : userAvatar.width + 11
        width: root.bodyWidth
        height: root.bodyHeight
        radius: 9
        color: root.bubbleColor
        border.width: root.localImage || root.isSelf ? 0 : 1
        border.color: Theme.line
        Behavior on color { ColorAnimation { duration: 160 } }

        Image {
            id: preview
            visible: root.localImage
            x: root.paddingH
            y: root.paddingV
            width: root.contentWidth
            height: root.localImage ? width * root.imageHeight / Math.max(1, root.imageWidth) : 0
            source: root.imageSource
            fillMode: Image.PreserveAspectFit
        }

        TextEdit {
            id: label
            x: root.paddingH
            y: root.paddingV
            width: root.contentWidth
            visible: !root.localImage
            readOnly: true
            selectByMouse: true
            selectionColor: Theme.selected
            selectedTextColor: Theme.textPrimary
            text: root.text
            textFormat: TextEdit.PlainText
            wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
            font.pixelSize: Theme.fontBody
            color: root.isSelf ? Theme.bubbleSelfText : Theme.textPrimary
        }
    }
}
