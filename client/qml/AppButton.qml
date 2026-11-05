import QtQuick

// 主操作按钮：颜色跟随悬停/按下/忙碌，焦点态用朱红描边而不是换色。
Rectangle {
    id: root
    property string text: ""
    property string iconName: ""
    property bool busy: false
    signal clicked()
    implicitHeight: Theme.fieldHeight
    height: implicitHeight
    radius: Theme.radius
    activeFocusOnTab: enabled && !busy
    // 忙碌时按钮仍保持主色，但不再响应点击，避免重复提交
    color: busy ? Theme.primaryHover : area.pressed ? Theme.primaryPressed
                            : area.containsMouse ? Theme.primaryHover : Theme.primary
    border.width: activeFocus ? 2 : 0
    border.color: Theme.accent
    opacity: enabled ? 1 : 0.5
    scale: area.pressed && !busy ? 0.985 : 1
    Accessible.role: Accessible.Button
    Accessible.name: text
    Accessible.onPressAction: if (enabled && !busy) clicked()
    Keys.onReturnPressed: if (!busy) clicked()
    Keys.onSpacePressed: if (!busy) clicked()
    Behavior on color { ColorAnimation { duration: 140 } }
    Behavior on scale { NumberAnimation { duration: 110 } }

    // 文字与图标作为一组居中，图标为空时不占宽度
    Row {
        anchors.centerIn: parent
        spacing: 10
        Text { text: root.text; color: Theme.primaryText; font.pixelSize: 14; font.weight: Font.DemiBold }
        AppIcon { visible: root.iconName.length > 0; name: root.iconName; color: Theme.primaryText; size: 18; anchors.verticalCenter: parent.verticalCenter }
    }
    MouseArea {
        id: area
        anchors.fill: parent
        enabled: !root.busy
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
