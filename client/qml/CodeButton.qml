import QtQuick

Rectangle {
    id: root
    property string text: ""
    property bool usable: true
    signal clicked()
    height: Theme.fieldHeight
    radius: Theme.radius
    color: area.containsMouse && usable ? Theme.primarySoft : Theme.panelAlt
    border.width: activeFocus ? 1.5 : 1
    border.color: activeFocus ? Theme.primary : Theme.line
    activeFocusOnTab: usable
    opacity: usable ? 1 : 0.6
    Accessible.role: Accessible.Button
    Accessible.name: text
    Keys.onReturnPressed: if (usable) clicked()
    Keys.onSpacePressed: if (usable) clicked()
    Text { anchors.centerIn: parent; text: root.text; color: Theme.textPrimary; font.pixelSize: Theme.fontSmall }
    MouseArea { id: area; anchors.fill: parent; enabled: root.usable; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.clicked() }
}
