import QtQuick
import QtQuick.Layouts
import XiguChat

// 登录表单通过 bridge 提交，角色只跟随焦点和请求状态。
// 表单按「角色 → 标题 → 邮箱 → 密码 → 错误位 → 按钮 → 注册入口」排列。
Item {
    id: page

    LoginBridge {
        id: bridge
        objectName: "pageBridge"
    }

    AuthPage {
        anchors.fill: parent
        enabled: !bridge.busy

        // 折纸角色只在输入区给反馈：聚焦、遮眼与忙碌都不影响表单可用性
        Item {
            width: parent.width
            height: 98

            PaperCompanion {
                anchors.horizontalCenter: parent.horizontalCenter
                width: 140
                height: 98
                attentive: emailInput.inputFocused
                hiding: passwordInput.inputFocused && !passwordInput.reveal
                revealing: passwordInput.inputFocused && passwordInput.reveal
                busy: bridge.busy
            }
        }

        Spacer { gap: 6 }

        Text {
            text: "接着聊。"
            color: Theme.textPrimary
            font.pixelSize: Theme.fontTitle
            font.bold: true
        }

        Spacer { gap: 5 }

        Text {
            text: "登录账号，继续聊天"
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSmall
        }

        Spacer { gap: 18 }

        Text {
            text: "邮箱"
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSmall
        }

        Spacer { gap: 7 }

        AppInput {
            id: emailInput
            width: parent.width
            placeholder: "你的邮箱地址"
            onTextEdited: bridge.email = text
            onAccepted: passwordInput.forceInputFocus()
        }

        Spacer { gap: 12 }

        // 密码标题行右侧放找回入口，两者共用一条基线
        RowLayout {
            width: parent.width
            spacing: 0

            Text {
                text: "密码"
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSmall
            }

            Item { Layout.fillWidth: true }

            AppLink {
                text: "忘记了？"
                onClicked: bridge.go("reset")
            }
        }

        Spacer { gap: 7 }

        AppInput {
            id: passwordInput
            width: parent.width
            placeholder: "输入密码"
            password: true
            reveal: bridge.passwordVisible
            onTextEdited: bridge.password = text
            onAccepted: bridge.login()
            onRevealToggled: bridge.togglePasswordVisible()
        }

        Spacer { gap: 8 }

        // 错误位固定最小高度，出现提示时表单不会整体跳动
        Text {
            width: parent.width
            height: Math.max(20, implicitHeight)
            text: bridge.errorText
            color: Theme.danger
            font.pixelSize: Theme.fontSmall
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            opacity: text.length > 0 ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 120 } }
        }

        Spacer { gap: 10 }

        AppButton {
            width: parent.width
            text: bridge.busy ? bridge.busyText : "登录"
            busy: bridge.busy
            onClicked: bridge.login()
        }

        Spacer { gap: 16 }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 6

            Text {
                text: "第一次来？"
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSmall
            }

            AppLink {
                text: "创建账号"
                emphasize: true
                onClicked: bridge.go("register")
            }
        }
    }

    Component.onCompleted: emailInput.forceInputFocus()
}
