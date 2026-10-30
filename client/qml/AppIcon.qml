import QtQuick

// 共用 24 单位画布与圆头线条，在任意缩放和主题下保持一致。
Canvas {
    id: root
    property string name: ""
    property color color: Theme.textPrimary
    property real size: 20
    implicitWidth: size
    implicitHeight: size
    antialiasing: true
    onNameChanged: requestPaint()
    onColorChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()

    onPaint: {
        const c = getContext("2d")
        c.reset()
        c.scale(width / 24, height / 24)
        c.strokeStyle = root.color
        c.fillStyle = root.color
        c.lineWidth = 1.65
        c.lineCap = "round"
        c.lineJoin = "round"
        function line(x1, y1, x2, y2) {
            c.beginPath(); c.moveTo(x1, y1); c.lineTo(x2, y2); c.stroke()
        }
        function circle(x, y, r) {
            c.beginPath(); c.arc(x, y, r, 0, Math.PI * 2); c.stroke()
        }
        function poly(points, closed) {
            c.beginPath(); c.moveTo(points[0], points[1])
            for (let i = 2; i < points.length; i += 2) c.lineTo(points[i], points[i + 1])
            if (closed) c.closePath()
            c.stroke()
        }
        function rounded(x, y, w, h, r) {
            c.moveTo(x + r, y); c.lineTo(x + w - r, y); c.quadraticCurveTo(x + w, y, x + w, y + r)
            c.lineTo(x + w, y + h - r); c.quadraticCurveTo(x + w, y + h, x + w - r, y + h)
            c.lineTo(x + r, y + h); c.quadraticCurveTo(x, y + h, x, y + h - r)
            c.lineTo(x, y + r); c.quadraticCurveTo(x, y, x + r, y)
        }
        switch (name) {
        case "brand":
            c.scale(24 / 88, 24 / 88); c.translate(-19, -24)
            c.beginPath(); c.moveTo(44, 31); c.lineTo(84, 31)
            c.bezierCurveTo(95, 31, 102, 38, 102, 49); c.lineTo(102, 71)
            c.bezierCurveTo(102, 82, 95, 89, 84, 89); c.lineTo(54, 89)
            c.lineTo(33, 104); c.lineTo(33, 87)
            c.bezierCurveTo(27, 84, 24, 78, 24, 71); c.lineTo(24, 49)
            c.bezierCurveTo(24, 38, 31, 31, 44, 31); c.closePath(); c.fill()
            c.globalCompositeOperation = "destination-out"; c.lineWidth = 7
            line(43, 52, 76, 52); line(43, 65, 65, 65); break
        case "chat":
            c.beginPath(); rounded(3, 4, 18, 14, 4); c.stroke()
            poly([7, 18, 7, 21, 12, 18], false)
            line(7, 9, 17, 9); line(7, 13, 13, 13); break
        case "contacts":
            c.beginPath(); rounded(5, 3, 15, 18, 2); c.stroke()
            line(3, 7, 6, 7); line(3, 12, 6, 12); line(3, 17, 6, 17)
            circle(12.5, 8.5, 2.3)
            c.beginPath(); c.arc(12.5, 16, 4, Math.PI, 0); c.stroke(); break
        case "search": circle(10.5, 10.5, 6.5); line(15.5, 15.5, 21, 21); break
        case "plus": line(12, 5, 12, 19); line(5, 12, 19, 12); break
        case "close": line(6, 6, 18, 18); line(18, 6, 6, 18); break
        case "back": poly([14, 5, 7, 12, 14, 19], false); break
        case "arrowRight": line(4, 12, 20, 12); poly([14, 6, 20, 12, 14, 18], false); break
        case "chevronRight": poly([9, 5, 16, 12, 9, 19], false); break
        case "send": poly([3, 4, 21, 12, 3, 20, 6, 12, 3, 4], false); line(6, 12, 21, 12); break
        case "mail":
            c.beginPath(); rounded(3, 5, 18, 14, 2); c.stroke()
            poly([3, 6, 12, 13, 21, 6], false); break
        case "lock":
            c.beginPath(); rounded(5, 10, 14, 11, 2); c.stroke()
            c.beginPath(); c.arc(12, 9, 4, Math.PI, 0); c.stroke()
            line(8, 9, 8, 10); line(16, 9, 16, 10); line(12, 14, 12, 17); break
        case "user":
            circle(12, 7, 4)
            c.beginPath(); c.arc(12, 21, 8, Math.PI, 0); c.stroke(); break
        case "eye": case "eyeOff":
            c.beginPath(); c.moveTo(2, 12); c.bezierCurveTo(7, 3, 17, 3, 22, 12)
            c.bezierCurveTo(17, 21, 7, 21, 2, 12); c.stroke()
            circle(12, 12, 3)
            if (name === "eyeOff") line(4, 3, 20, 21)
            break
        case "moon":
            c.beginPath(); c.moveTo(18.5, 15.5)
            c.bezierCurveTo(11, 18, 5.5, 11.5, 9, 4)
            c.bezierCurveTo(-1, 8, 4, 23, 14, 20)
            c.bezierCurveTo(16.5, 19.5, 18, 18, 18.5, 15.5); c.closePath(); c.stroke()
            line(18, 3, 18, 7); line(16, 5, 20, 5); break
        case "sun":
            circle(12, 12, 4)
            for (let i = 0; i < 8; ++i) {
                const a = i * Math.PI / 4
                line(12 + 7 * Math.cos(a), 12 + 7 * Math.sin(a), 12 + 10 * Math.cos(a), 12 + 10 * Math.sin(a))
            }
            break
        case "emoji":
            circle(12, 12, 9); circle(8.5, 9, .5); circle(15.5, 9, .5)
            c.beginPath(); c.arc(12, 12, 4.8, .25, Math.PI - .25); c.stroke(); break
        case "folder":
            poly([3, 8, 3, 5, 9, 5, 12, 8, 21, 8, 19, 19, 3, 19, 3, 8, 21, 8], false); break
        case "image":
            c.beginPath(); rounded(3, 3, 18, 18, 2); c.stroke()
            circle(8, 8, 2); poly([4, 19, 11, 12, 15, 16, 18, 13, 21, 16], false); break
        case "check": poly([5, 12, 10, 17, 20, 6], false); break
        case "tag": poly([3, 3, 12, 3, 21, 12, 12, 21, 3, 12], true); circle(8, 8, 1.4); break
        case "phone":
            poly([6, 3, 10, 7, 7, 10, 14, 17, 17, 14, 21, 18, 18, 21], false)
            c.beginPath(); c.moveTo(18, 21); c.bezierCurveTo(9, 21, 3, 14, 3, 6); c.lineTo(6, 3); c.stroke(); break
        case "video":
            c.beginPath(); rounded(3, 6, 12, 12, 2); c.stroke()
            poly([15, 10, 21, 7, 21, 17, 15, 14], false); break
        }
    }
}
