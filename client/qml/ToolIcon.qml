import QtQuick

// 输入框上方的小工具图标：表情 / 文件。
// 发送能力还没做（只支持文本），图标先放上，灰显表示暂不可用。
Item {
    id: root

    property string kind: "emoji" // emoji | folder

    signal clicked()

    width: 22
    height: 22

    Canvas {
        anchors.fill: parent
        opacity: area.containsMouse ? 0.95 : 0.55

        Behavior on opacity { NumberAnimation { duration: 120 } }

        onPaint: {
            const ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            ctx.strokeStyle = Theme.textSecondary
            ctx.lineWidth = 1.5
            ctx.lineCap = "round"
            ctx.lineJoin = "round"

            if (root.kind === "emoji") {
                // 笑脸：圆 + 两眼 + 嘴
                ctx.beginPath()
                ctx.arc(11, 11, 7.6, 0, Math.PI * 2)
                ctx.stroke()

                ctx.fillStyle = Theme.textSecondary
                ctx.beginPath()
                ctx.arc(8.4, 9, 0.9, 0, Math.PI * 2)
                ctx.fill()
                ctx.beginPath()
                ctx.arc(13.6, 9, 0.9, 0, Math.PI * 2)
                ctx.fill()

                ctx.beginPath()
                ctx.moveTo(7.6, 13.2)
                ctx.quadraticCurveTo(11, 16.4, 14.4, 13.2)
                ctx.stroke()
            } else {
                // 文件夹：主体 + 顶部标签
                ctx.beginPath()
                ctx.moveTo(3.2, 7.4)
                ctx.lineTo(8.4, 7.4)
                ctx.lineTo(10, 9.6)
                ctx.lineTo(18.8, 9.6)
                ctx.lineTo(18.8, 17.4)
                ctx.lineTo(3.2, 17.4)
                ctx.closePath()
                ctx.stroke()

                ctx.beginPath()
                ctx.moveTo(3.2, 7.4)
                ctx.lineTo(3.2, 5.6)
                ctx.quadraticCurveTo(3.2, 5, 3.9, 5)
                ctx.lineTo(7.6, 5)
                ctx.lineTo(9.2, 7.4)
                ctx.stroke()
            }
        }
    }

    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
