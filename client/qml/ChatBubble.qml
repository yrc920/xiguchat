import QtQuick
import QtQuick.Layouts

// 文本消息气泡：宽度由文本自然宽度决定（上限 460），高度交给 Text 自身排版。
// 朝头像一侧有一个小三角，用旋转 45° 的同色方块实现，接缝不可见。
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

    readonly property real padH: 13
    readonly property real padV: 8
    readonly property real maxW: Math.min(Theme.bubbleMaxWidth, width * 0.72)
    readonly property real contentMaxW: maxW - padH * 2
    readonly property color bubbleColor: isSelf ? Theme.bubbleSelf : Theme.bubbleOther

    TextMetrics {
        id: metrics
        font: label.font
        text: label.text
    }

    readonly property real contentW: localImage ? Math.min(imageWidth, contentMaxW)
                                    : Math.min(Math.ceil(metrics.advanceWidth), contentMaxW)
    readonly property real bodyW: Math.max(32, contentW + padH * 2)
    readonly property real bodyH: (localImage ? preview.height : label.implicitHeight) + padV * 2
    readonly property real tailW: 7

    implicitHeight: row.implicitHeight

    RowLayout {
        id: row
        width: root.width
        spacing: 11

        Item { Layout.fillWidth: true; visible: root.isSelf }

        Item {
            visible: !root.isSelf
            Layout.alignment: Qt.AlignTop
            width: 32
            height: 32
            Avatar {
                anchors.fill: parent
                source: root.avatar
                name: root.name
            }
        }

        Item {
            id: slot
            Layout.preferredWidth: root.bodyW + root.tailW
            Layout.preferredHeight: root.bodyH
            Layout.alignment: Qt.AlignTop

            Rectangle {
                visible: !root.localImage
                width: 10
                height: 10
                radius: 2
                rotation: 45
                color: root.bubbleColor
                x: root.isSelf ? root.bodyW - 5 : root.tailW - 5
                y: 12
            }

            Rectangle {
                id: body
                x: root.isSelf ? 0 : root.tailW
                width: root.bodyW
                height: root.bodyH
                radius: 9
                color: root.bubbleColor

                Image {
                    id: preview
                    visible: root.localImage
                    x: root.padH
                    y: root.padV
                    width: root.contentW
                    height: root.localImage ? width * root.imageHeight / Math.max(1, root.imageWidth) : 0
                    source: root.imageSource
                    fillMode: Image.PreserveAspectFit
                }
                TextEdit {
                    id: label
                    readOnly: true
                    selectByMouse: true
                    selectionColor: Theme.accentSoft
                    visible: !root.localImage
                    x: root.padH
                    y: root.padV
                    width: root.contentW
                    text: root.text
                    textFormat: TextEdit.PlainText
                    wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
                    font.pixelSize: Theme.fontBody
                    color: root.isSelf ? Theme.bubbleSelfText : Theme.textPrimary
                }
            }
        }

        Item {
            visible: root.isSelf
            Layout.alignment: Qt.AlignTop
            width: 32
            height: 32
            Avatar {
                anchors.fill: parent
                source: root.avatar
                name: root.name
                color: Theme.primary
                textColor: "#FFFFFF"
            }
        }

        Item { Layout.fillWidth: true; visible: !root.isSelf }
    }
}
