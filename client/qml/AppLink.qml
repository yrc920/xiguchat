import QtQuick

// 文字链接：默认次级灰，悬停转强调色
Text {
    id: root

    // true 用正文色（主链接），false 用次级灰
    property bool emphasize: false
    signal clicked()

    font.pixelSize: Theme.fontSmall
    color: area.containsMouse ? Theme.accent
                              : (root.emphasize ? Theme.textPrimary : Theme.textSecondary)
    Behavior on color { ColorAnimation { duration: 120 } }

    MouseArea {
        id: area
        anchors.fill: parent
        anchors.margins: -5          // 放大点击热区，文字本体不动
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
