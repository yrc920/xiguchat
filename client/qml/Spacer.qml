import QtQuick

// 竖向留白。
// 必须显式给 width：Item 默认宽度为 0，而 Column/Row 这类 positioner
// 会跳过零宽元素，只写 height 的留白会被静默吃掉（不报错，只是不占位）。
Item {
    property int gap: 8

    width: 1
    height: gap
}
