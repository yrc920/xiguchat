import QtQuick

// 带占位符与聚焦态的输入框。
// 状态由输入框自身持有，内容变化通过 textEdited / accepted 抛给页面。
Rectangle {
    id: root

    property alias text: input.text
    property string placeholder: ""      // 空内容时显示的灰字
    property bool password: false        // 密码框：默认密文，右侧带可见性开关
    property bool reveal: false          // 密码是否明文显示
    property bool invalid: false         // 校验失败：边框转红

    signal textEdited()
    signal accepted()
    signal revealToggled()

    function forceInputFocus() { input.forceActiveFocus() }

    height: Theme.fieldHeight
    radius: Theme.radius
    color: Theme.fieldBg
    border.width: input.activeFocus ? 1.5 : 1
    // 聚焦态用近黑而非强调色：整页只让按钮和链接带颜色
    border.color: input.activeFocus ? Theme.textPrimary
                                    : (root.invalid ? Theme.danger : Theme.fieldBorder)

    Behavior on border.color { ColorAnimation { duration: 120 } }

    TextInput {
        id: input
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 15
        anchors.rightMargin: root.password ? 44 : 15
        anchors.verticalCenter: parent.verticalCenter

        verticalAlignment: TextInput.AlignVCenter
        font.pixelSize: Theme.fontBody
        color: Theme.textPrimary
        selectionColor: Theme.accentSoft
        selectedTextColor: Theme.textPrimary
        clip: true

        echoMode: root.password && !root.reveal ? TextInput.Password : TextInput.Normal
        passwordCharacter: "●"
        passwordMaskDelay: 0

        onTextEdited: root.textEdited()
        onAccepted: root.accepted()
    }

    Text {
        anchors.left: parent.left
        anchors.leftMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        text: root.placeholder
        color: Theme.textHint
        font.pixelSize: Theme.fontBody
        // 中文输入法组合期间 text 仍为空，用 inputMethodComposing 判断，避免提示压住拼音
        // （不要用 preeditText：它在 Qt 6.5 是带 REVISION 的属性，AOT 生成的代码会拿到空元对象并崩溃）
        visible: input.text.length === 0 && !input.inputMethodComposing
    }

    // 密码可见性切换
    Item {
        visible: root.password
        anchors.right: parent.right
        anchors.rightMargin: 13
        anchors.verticalCenter: parent.verticalCenter
        width: 18
        height: 18

        Image {
            anchors.fill: parent
            source: root.reveal ? "qrc:/res/visible.png" : "qrc:/res/unvisible.png"
            fillMode: Image.PreserveAspectFit
            opacity: area.containsMouse ? 0.75 : 0.35
            Behavior on opacity { NumberAnimation { duration: 120 } }
        }

        MouseArea {
            id: area
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.revealToggled()
        }
    }
}
