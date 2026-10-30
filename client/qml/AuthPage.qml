import QtQuick

// 认证页共用的品牌栏与表单容器，较长的提示可随内容滚动。
Item {
    id: root

    default property alias content: column.data
    property int formWidth: 320

    data: [
        Rectangle {
            anchors.fill: parent
            color: Theme.panelBg
        },
        BrandMark {
            x: 28
            y: 22
            size: 28
        },
        Flickable {
            id: scroll
            anchors.fill: parent
            anchors.topMargin: 56
            clip: true
            contentWidth: width
            contentHeight: Math.max(height, column.y + column.height + 24)
            interactive: contentHeight > height
            boundsBehavior: Flickable.StopAtBounds

            Column {
                id: column
                width: Math.min(root.formWidth, scroll.width - 64)
                x: (scroll.width - width) / 2
                y: Math.max(12, (scroll.height - height - 24) / 2)
                spacing: 0
            }
        }
    ]
}
