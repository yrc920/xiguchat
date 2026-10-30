import QtQuick

// 品牌图形与桌面应用图标共用同一份图像。
Item {
    id: root
    property real size: 28
    implicitWidth: size
    implicitHeight: size

    Image {
        anchors.fill: parent
        source: "qrc:/res/xiguchat.png"
        fillMode: Image.PreserveAspectFit
        mipmap: true
    }
}
