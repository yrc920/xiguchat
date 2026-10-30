import QtQuick
import QtQuick.Effects

// 有头像时显示图片，路径为空或加载失败时回退到名字首字。
Rectangle {
    id: root
    property string source: ""
    property string name: ""
    property color textColor: Theme.ink
    property int textSize: 13

    radius: width * 0.24
    color: Theme.selected

    Text {
        anchors.centerIn: parent
        visible: picture.status !== Image.Ready
        text: root.name.length > 0 ? root.name.substring(0, 1) : "?"
        color: root.textColor
        font.pixelSize: root.textSize
    }

    Image {
        id: picture
        anchors.fill: parent
        source: root.source.startsWith(":/") ? "qrc" + root.source : root.source
        fillMode: Image.PreserveAspectCrop
        visible: status === Image.Ready
        layer.enabled: GraphicsInfo.api !== GraphicsInfo.Software
        layer.effect: MultiEffect {
            maskEnabled: true
            maskSource: mask
        }
    }

    Rectangle {
        id: mask
        anchors.fill: parent
        radius: root.radius
        visible: false
        layer.enabled: true
    }
}
