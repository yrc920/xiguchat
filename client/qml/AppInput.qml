import QtQuick

// 表单输入只暴露焦点与组合态，装饰角色不会读取密码内容。
Rectangle {
    id: root
    property alias text: input.text
    property string placeholder: ""
    property bool password: false
    property bool reveal: false
    property bool invalid: false
    readonly property bool inputFocused: input.activeFocus
    readonly property bool composing: input.inputMethodComposing
    signal textEdited()
    signal accepted()
    signal revealToggled()
    function forceInputFocus() { input.forceActiveFocus() }

    height: Theme.fieldHeight
    radius: Theme.radius
    color: Theme.fieldBg
    border.width: input.activeFocus ? 1.5 : 1
    border.color: invalid ? Theme.danger : input.activeFocus ? Theme.primary : Theme.fieldBorder
    Behavior on border.color { ColorAnimation { duration: 130 } }

    TextInput {
        id: input
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 14
        anchors.rightMargin: root.password ? 48 : 14
        anchors.verticalCenter: parent.verticalCenter
        height: parent.height
        verticalAlignment: TextInput.AlignVCenter
        font.pixelSize: Theme.fontBody
        color: Theme.textPrimary
        selectionColor: Theme.selected
        selectedTextColor: Theme.textPrimary
        clip: true
        selectByMouse: true
        activeFocusOnTab: true
        echoMode: root.password && !root.reveal ? TextInput.Password : TextInput.Normal
        passwordCharacter: "•"
        passwordMaskDelay: 0
        inputMethodHints: root.password ? Qt.ImhHiddenText | Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase : Qt.ImhNone
        Accessible.name: root.placeholder
        onTextEdited: root.textEdited()
        onAccepted: root.accepted()
    }
    Text {
        anchors.left: parent.left
        anchors.leftMargin: 14
        anchors.verticalCenter: parent.verticalCenter
        visible: input.length === 0 && !input.inputMethodComposing
        text: root.placeholder
        color: Theme.textHint
        font.pixelSize: Theme.fontBody
    }
    Item {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        width: 44
        height: parent.height
        visible: root.password
        activeFocusOnTab: visible
        Accessible.role: Accessible.Button
        Accessible.name: root.reveal ? "隐藏密码" : "显示密码"
        Keys.onReturnPressed: root.revealToggled()
        Keys.onSpacePressed: root.revealToggled()
        AppIcon { anchors.centerIn: parent; size: 19; name: root.reveal ? "eye" : "eyeOff"; color: eyeArea.containsMouse || parent.activeFocus ? Theme.textPrimary : Theme.textSecondary }
        MouseArea { id: eyeArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.revealToggled() }
    }
}
