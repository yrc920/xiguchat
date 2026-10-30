import QtQuick
import QtQuick.Layouts
import XiguChat

// 找回密码通过用户名、邮箱与验证码验证，状态由 ResetBridge 提供。
Item {
    id: page

    ResetBridge {
        id: bridge
        objectName: "pageBridge"
    }

    AuthPage {
        anchors.fill: parent
        enabled: !(bridge.busy || bridge.codeBusy || bridge.done)

        Item {
            width: parent.width
            height: 70

            PaperCompanion {
                anchors.horizontalCenter: parent.horizontalCenter
                width: 100
                height: 70
                compact: true
                attentive: userInput.inputFocused || emailInput.inputFocused || codeInput.inputFocused
                hiding: passwordInput.inputFocused && !passwordInput.reveal
                revealing: passwordInput.inputFocused && passwordInput.reveal
                busy: bridge.busy || bridge.codeBusy
            }
        }

        Spacer { gap: 8 }

        Text {
            text: "找回钥匙。"
            color: Theme.textPrimary
            font.pixelSize: Theme.fontTitle
            font.bold: true
        }

        Spacer { gap: 5 }

        Text {
            text: "验证邮箱，为账号设置新密码"
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSmall
        }

        Spacer { gap: 14 }

        Text {
            text: "用户名"
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSmall
        }

        Spacer { gap: 6 }

        AppInput {
            id: userInput
            width: parent.width
            placeholder: "注册时使用的用户名"
            onTextEdited: bridge.user = text
            onAccepted: emailInput.forceInputFocus()
        }

        Spacer { gap: 12 }

        Text {
            text: "邮箱"
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSmall
        }

        Spacer { gap: 6 }

        AppInput {
            id: emailInput
            width: parent.width
            placeholder: "与账号关联的邮箱"
            onTextEdited: bridge.email = text
            onAccepted: codeInput.forceInputFocus()
        }

        Spacer { gap: 12 }

        Text {
            text: "邮箱验证码"
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSmall
        }

        Spacer { gap: 6 }

        RowLayout {
            width: parent.width
            spacing: 10

            AppInput {
                id: codeInput
                Layout.fillWidth: true
                placeholder: "填写验证码"
                onTextEdited: bridge.code = text
                onAccepted: passwordInput.forceInputFocus()
            }

            CodeButton {
                Layout.preferredWidth: 112
                text: bridge.countdown > 0 ? bridge.countdown + " 秒后重发"
                                           : (bridge.codeBusy ? "发送中…" : "获取验证码")
                usable: bridge.countdown === 0 && !bridge.codeBusy && !bridge.busy && !bridge.done
                onClicked: bridge.sendCode()
            }
        }

        Spacer { gap: 12 }

        Text {
            text: "新密码"
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSmall
        }

        Spacer { gap: 6 }

        AppInput {
            id: passwordInput
            width: parent.width
            placeholder: "6–15 位密码"
            password: true
            onRevealToggled: reveal = !reveal
            onTextEdited: bridge.password = text
            onAccepted: bridge.submit()
        }

        Spacer { gap: 8 }

        Text {
            width: parent.width
            height: Math.max(20, implicitHeight)
            text: bridge.tipText
            color: bridge.tipOk ? Theme.success : Theme.danger
            font.pixelSize: Theme.fontSmall
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            opacity: text.length > 0 ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 120 } }
        }

        Spacer { gap: 12 }

        AppButton {
            width: parent.width
            text: bridge.done ? "密码已更新" : (bridge.busy ? "正在更新…" : "更新密码")
            busy: bridge.busy || bridge.done
            onClicked: bridge.submit()
        }

        Spacer { gap: 14 }

        AppLink {
            width: parent.width
            text: "返回登录"
            horizontalAlignment: Text.AlignHCenter
            onClicked: bridge.go("login")
        }
    }

    Component.onCompleted: userInput.forceInputFocus()
}
