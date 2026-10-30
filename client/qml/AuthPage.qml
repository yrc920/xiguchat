import QtQuick

// 认证类页面的统一外壳：浅暖底 + 居中定宽列。
// 页面内容直接写在内部，自动进入中间的 Column。
Item {
    id: root

    default property alias content: column.data
    property int formWidth: 300

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: Theme.bgTop }
            GradientStop { position: 1.0; color: Theme.bgBottom }
        }
    }

    Column {
        id: column
        width: root.formWidth
        anchors.centerIn: parent
        spacing: 0
    }
}
