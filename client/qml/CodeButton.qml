import QtQuick

// 次要按钮：描边款，用于「获取验证码」这类非主流程动作
Rectangle {
    id: root

    property string text: ""     // 按钮文字（倒计时期间显示秒数）
    property bool usable: true   // 为假时灰显且不可点

    signal clicked()

    height: Theme.fieldHeight
    radius: Theme.radius
    color: area.containsMouse && root.usable ? "#F7F5F2" : "transparent"
    border.width: 1
    border.color: Theme.fieldBorder

    Behavior on color { ColorAnimation { duration: 120 } }

    Text {
        anchors.centerIn: parent
        text: root.text
        color: root.usable ? Theme.textSecondary : Theme.textHint
        font.pixelSize: Theme.fontSmall
    }

    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        enabled: root.usable
        cursorShape: root.usable ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: root.clicked()
    }
}
