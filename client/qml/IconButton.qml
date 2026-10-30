import QtQuick

// 圆形图标按钮：Canvas 画的线性图标 + 文字标签。
// 语音 / 视频还没接功能，用 available: false 表现，不做成点了没反应的按钮。
Item {
    id: root

    property string kind: "chat" // chat / voice / video
    property string label: ""
    property bool available: true

    signal clicked()

    width: 88
    height: 82

    Rectangle {
        id: disc
        width: 54
        height: 54
        radius: 27
        anchors.horizontalCenter: parent.horizontalCenter
        color: !root.available ? "#F2F0EC"
                                  : (area.containsMouse ? "#EAE7E1" : "#F6F4F0")

        Canvas {
            id: canvas
            anchors.centerIn: parent
            width: 28
            height: 28
            opacity: root.available ? 1.0 : 0.5

            onPaint: {
                const ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                ctx.strokeStyle = Theme.textPrimary
                ctx.lineWidth = 1.6
                ctx.lineCap = "round"
                ctx.lineJoin = "round"

                if (root.kind === "chat") {
                    // 气泡 + 左下角小尾巴 + 三个点
                    const x = 4.5, y = 6.5, w = 19, h = 13, r = 4
                    ctx.beginPath()
                    ctx.moveTo(x + r, y)
                    ctx.lineTo(x + w - r, y)
                    ctx.quadraticCurveTo(x + w, y, x + w, y + r)
                    ctx.lineTo(x + w, y + h - r)
                    ctx.quadraticCurveTo(x + w, y + h, x + w - r, y + h)
                    ctx.lineTo(x + 9, y + h)
                    ctx.lineTo(x + 5, y + h + 4)
                    ctx.lineTo(x + 5, y + h)
                    ctx.lineTo(x + r, y + h)
                    ctx.quadraticCurveTo(x, y + h, x, y + h - r)
                    ctx.lineTo(x, y + r)
                    ctx.quadraticCurveTo(x, y, x + r, y)
                    ctx.stroke()

                    ctx.fillStyle = Theme.textPrimary
                    for (let i = 0; i < 3; ++i) {
                        ctx.beginPath()
                        ctx.arc(x + 7 + i * 4, y + h / 2, 0.85, 0, Math.PI * 2)
                        ctx.fill()
                    }
                } else if (root.kind === "voice") {
                    // 麦克风：胶囊 + 支架弧 + 底座
                    const cx = 14
                    ctx.beginPath()
                    ctx.moveTo(cx - 3.3, 8)
                    ctx.bezierCurveTo(cx - 3.3, 5.6, cx + 3.3, 5.6, cx + 3.3, 8)
                    ctx.lineTo(cx + 3.3, 13.6)
                    ctx.bezierCurveTo(cx + 3.3, 16.6, cx - 3.3, 16.6, cx - 3.3, 13.6)
                    ctx.closePath()
                    ctx.stroke()

                    ctx.beginPath()
                    ctx.moveTo(cx - 6.4, 13)
                    ctx.bezierCurveTo(cx - 6.4, 18.6, cx + 6.4, 18.6, cx + 6.4, 13)
                    ctx.stroke()

                    ctx.beginPath()
                    ctx.moveTo(cx, 18.4)
                    ctx.lineTo(cx, 22)
                    ctx.moveTo(cx - 3.2, 22)
                    ctx.lineTo(cx + 3.2, 22)
                    ctx.stroke()
                } else {
                    // 摄像机：机身 + 右侧镜头
                    const r = 3, x = 4, y = 9, w = 13, h = 10
                    ctx.beginPath()
                    ctx.moveTo(x + r, y)
                    ctx.lineTo(x + w - r, y)
                    ctx.quadraticCurveTo(x + w, y, x + w, y + r)
                    ctx.lineTo(x + w, y + h - r)
                    ctx.quadraticCurveTo(x + w, y + h, x + w - r, y + h)
                    ctx.lineTo(x + r, y + h)
                    ctx.quadraticCurveTo(x, y + h, x, y + h - r)
                    ctx.lineTo(x, y + r)
                    ctx.quadraticCurveTo(x, y, x + r, y)
                    ctx.stroke()

                    ctx.beginPath()
                    ctx.moveTo(x + w + 0.5, 12.4)
                    ctx.lineTo(24, 9.6)
                    ctx.lineTo(24, 18.4)
                    ctx.lineTo(x + w + 0.5, 15.6)
                    ctx.stroke()
                }
            }
        }
    }

    Text {
        anchors.top: disc.bottom
        anchors.topMargin: 8
        anchors.horizontalCenter: parent.horizontalCenter
        text: root.label
        color: root.available ? Theme.textSecondary : Theme.textHint
        font.pixelSize: Theme.fontSmall
    }

    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        enabled: root.available
        cursorShape: root.available ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: root.clicked()
    }
}
