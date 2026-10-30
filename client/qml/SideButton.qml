import QtQuick

// 侧栏按钮：图标用 Canvas 画，和资料页的动作图标同一套线条语言，不依赖图片资源
Rectangle {
    id: root

    property bool active: false
    property int badge: 0        // 大于 0 时右上角显示红点计数
    property string kind: "chat" // chat | contact

    signal clicked()

    width: 44
    height: 44
    radius: 12
    color: root.active ? "#2C2E31" : "transparent"

    onActiveChanged: canvas.requestPaint()
    onKindChanged: canvas.requestPaint()

    Canvas {
        id: canvas
        anchors.centerIn: parent
        width: 22
        height: 22

        onPaint: {
            const ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            ctx.fillStyle = root.active ? "#FFFFFF" : "#8E8F92"

            if (root.kind === "chat") {
                // 气泡 + 左下尾巴
                const x = 2.5, y = 3.5, w = 17, h = 12, r = 4
                ctx.beginPath()
                ctx.moveTo(x + r, y)
                ctx.lineTo(x + w - r, y)
                ctx.quadraticCurveTo(x + w, y, x + w, y + r)
                ctx.lineTo(x + w, y + h - r)
                ctx.quadraticCurveTo(x + w, y + h, x + w - r, y + h)
                ctx.lineTo(x + 7.5, y + h)
                ctx.lineTo(x + 3.5, y + h + 4)
                ctx.lineTo(x + 3.5, y + h)
                ctx.lineTo(x + r, y + h)
                ctx.quadraticCurveTo(x, y + h, x, y + h - r)
                ctx.lineTo(x, y + r)
                ctx.quadraticCurveTo(x, y, x + r, y)
                ctx.closePath()
                ctx.fill()
            } else {
                // 头 + 肩
                ctx.beginPath()
                ctx.arc(11, 7, 3.7, 0, Math.PI * 2)
                ctx.fill()

                ctx.beginPath()
                ctx.arc(11, 19.5, 6.4, Math.PI, Math.PI * 2)
                ctx.closePath()
                ctx.fill()
            }
        }
    }

    // 红点计数
    Rectangle {
        visible: root.badge > 0
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.rightMargin: 3
        anchors.topMargin: 3
        width: Math.max(15, text.implicitWidth + 8)
        height: 15
        radius: 7.5
        color: Theme.danger

        Text {
            id: text
            anchors.centerIn: parent
            text: root.badge > 99 ? "99+" : root.badge
            color: "#FFFFFF"
            font.pixelSize: 9
            font.bold: true
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
