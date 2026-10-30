import QtQuick

// 折纸信封的表情只由交互状态驱动；折盖会在密码输入时遮住眼睛。
Item {
    id: root
    property bool hiding: false
    property bool revealing: false
    property bool attentive: false
    property bool busy: false
    property bool compact: false
    readonly property real lookX: gaze.hovered ? (gaze.point.position.x / Math.max(1, width) - 0.5) * 2 : 0
    readonly property real lookY: gaze.hovered ? (gaze.point.position.y / Math.max(1, height) - 0.5) * 2 : 0
    implicitWidth: 160
    implicitHeight: 112
    property real fold: hiding ? 1 : revealing ? 0.24 : 0
    property real blink: 1
    Behavior on fold { NumberAnimation { duration: 280; easing.type: Easing.InOutCubic } }

    HoverHandler {
        id: gaze
        enabled: !root.compact && !root.hiding
    }

    Timer {
        interval: 4600
        running: root.visible && !root.hiding && !root.compact
        repeat: true
        onTriggered: blinkMotion.start()
    }
    SequentialAnimation {
        id: blinkMotion
        NumberAnimation { target: root; property: "blink"; to: 0.1; duration: 85 }
        NumberAnimation { target: root; property: "blink"; to: 1; duration: 130 }
    }

    Item {
        width: 240
        height: 168
        anchors.centerIn: parent
        scale: Math.min(root.width / width, root.height / height)

        Rectangle {
            x: 41; y: 149; width: 160; height: 8; radius: 4
            color: Theme.line
            opacity: 0.65
        }
        Rectangle { x: 82; y: 135; width: 5; height: 15; radius: 2; rotation: 12; color: Theme.ink }
        Rectangle { x: 155; y: 135; width: 5; height: 15; radius: 2; rotation: -12; color: Theme.ink }

        Item {
            id: letter
            x: 36
            y: root.busy ? 19 : 26
            width: 170
            height: 116
            rotation: root.hiding ? 3 : root.revealing ? -9 : root.attentive ? -3 : -7
            Behavior on rotation { NumberAnimation { duration: 350; easing.type: Easing.OutCubic } }
            Behavior on y { NumberAnimation { duration: 250; easing.type: Easing.OutCubic } }

            Rectangle { x: 3; y: 4; width: parent.width; height: parent.height; radius: 7; color: Theme.ink; opacity: 0.15 }
            Rectangle {
                anchors.fill: parent
                color: Theme.paper
                radius: 7
                border.color: Theme.paperInk
                border.width: 1.6
            }
            Canvas {
                id: seams
                anchors.fill: parent
                onPaint: {
                    const c = getContext("2d")
                    c.reset(); c.strokeStyle = Theme.paperFold; c.lineWidth = 1.5
                    c.beginPath(); c.moveTo(1, 115); c.lineTo(61, 65); c.moveTo(169, 115); c.lineTo(109, 65); c.stroke()
                }
                Connections { target: Theme; function onDarkChanged() { seams.requestPaint() } }
            }

            Item {
                x: 53 + Math.max(-4, Math.min(4, root.lookX * 4))
                y: 46 + (root.attentive ? 2 : 0) + Math.max(-2, Math.min(3, root.lookY * 3))
                width: 64
                height: 36
                Behavior on x { NumberAnimation { duration: 130 } }
                Behavior on y { NumberAnimation { duration: 130 } }
                Repeater {
                    model: 2
                    Rectangle {
                        x: index * 48
                        y: 4
                        width: 7
                        height: 12 * root.blink
                        radius: 3.5
                        color: Theme.paperInk
                    }
                }
                Rectangle { x: 26; y: 23; width: 13; height: 2.5; radius: 1.2; rotation: root.revealing ? -12 : 0; color: Theme.paperInk }
            }

            Canvas {
                id: flap
                anchors.fill: parent
                onPaint: {
                    const c = getContext("2d")
                    c.reset(); c.fillStyle = Theme.paperFold; c.strokeStyle = Theme.paperInk; c.lineWidth = 1.5
                    c.beginPath(); c.moveTo(1, 5); c.quadraticCurveTo(1, 1, 6, 1)
                    c.lineTo(164, 1); c.quadraticCurveTo(169, 1, 169, 5)
                    c.lineTo(89, 23 + root.fold * 70)
                    c.quadraticCurveTo(85, 27 + root.fold * 70, 81, 23 + root.fold * 70)
                    c.closePath(); c.fill(); c.stroke()
                }
                Connections { target: root; function onFoldChanged() { flap.requestPaint() } }
                Connections { target: Theme; function onDarkChanged() { flap.requestPaint() } }
            }
            Rectangle {
                x: 137; y: 88; width: 20; height: 20; radius: 10
                color: Theme.stamp
                rotation: 8
                AppIcon { anchors.centerIn: parent; name: "brand"; size: 13; color: Theme.primaryText }
            }
        }
        // 边角线在角色留意输入时轻轻展开。
        Rectangle { x: 18; y: 52; width: root.attentive ? 12 : 6; height: 2; radius: 1; rotation: 20; color: Theme.stamp; opacity: root.attentive ? 1 : 0; Behavior on opacity { NumberAnimation { duration: 180 } } }
        Rectangle { x: 205; y: 31; width: 12; height: 2; radius: 1; rotation: -35; color: Theme.stamp; opacity: root.revealing ? 1 : 0; Behavior on opacity { NumberAnimation { duration: 180 } } }
    }
}
