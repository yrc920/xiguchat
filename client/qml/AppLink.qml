import QtQuick

// 轻量文字操作：默认次要色，悬停或聚焦时变强调色并加下划线。
Text {
    id: root
    property bool emphasize: false
    signal clicked()
    font.pixelSize: Theme.fontSmall
    color: area.containsMouse || activeFocus ? Theme.accent : emphasize ? Theme.textPrimary : Theme.textSecondary
    font.underline: area.containsMouse || activeFocus
    activeFocusOnTab: true
    Accessible.role: Accessible.Link
    Accessible.name: text
    Keys.onReturnPressed: clicked()
    Keys.onSpacePressed: clicked()
    MouseArea { id: area; anchors.fill: parent; anchors.margins: -4; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.clicked() }
}
