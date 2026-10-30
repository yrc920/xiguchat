import QtQuick

// 主操作按钮：整页唯一的实色块
Rectangle {
    id: root

    property string text: ""   // 按钮文字
    property bool busy: false  // 请求进行中：按钮变灰且不响应点击

    signal clicked()

    height: Theme.fieldHeight
    radius: Theme.radius
    color: root.busy ? Theme.primaryHover
                     : (mouse.pressed ? Theme.primaryPressed
                                      : (mouse.containsMouse ? Theme.primaryHover : Theme.primary))

    Behavior on color { ColorAnimation { duration: 120 } }

    Text {
        anchors.centerIn: parent
        text: root.text
        color: "#FFFFFF"
        font.pixelSize: 15
        font.bold: true
        font.letterSpacing: 1
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: root.busy ? Qt.ArrowCursor : Qt.PointingHandCursor
        onClicked: if (!root.busy) root.clicked()
        onPressedChanged: scale = pressed ? 0.99 : 1.0
    }

    Behavior on scale { NumberAnimation { duration: 90 } }
}
