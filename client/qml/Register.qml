import QtQuick
import QtQuick.Layouts
import XiguChat

// 注册信息和请求状态归 bridge，表单负责焦点顺序与密码显示。
Item {
    id: page

    RegisterBridge {
        id: bridge
        objectName: "pageBridge"
    }

    AuthPage {
        anchors.fill: parent
        enabled: !(bridge.busy || bridge.codeBusy || bridge.done)

        Item {
            width: parent.width
            height: 91

            PaperCompanion {
                anchors.horizontalCenter: parent.horizontalCenter
                width: 130
                height: 91
                compact: true
                attentive: userInput.inputFocused || emailInput.inputFocused || codeInput.inputFocused
                hiding: (passwordInput.inputFocused && !passwordInput.reveal)
                         || (confirmInput.inputFocused && !confirmInput.reveal)
                revealing: (passwordInput.inputFocused && passwordInput.reveal)
                            || (confirmInput.inputFocused && confirmInput.reveal)
                busy: bridge.busy || bridge.codeBusy
            }
        }

        Spacer { gap: 10 }

        Text {
            text: "认识一下。"
            color: Theme.textPrimary
            font.pixelSize: Theme.fontTitle
            font.bold: true
        }

        Spacer { gap: 5 }

        Text {
            text: "创建一个新账号"
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSmall
        }

        Spacer { gap: 20 }

        Text {
            text: "用户名"
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSmall
        }

        Spacer { gap: 6 }

        AppInput {
            id: userInput
            width: parent.width
            placeholder: "怎么称呼你"
            onTextEdited: bridge.user = text
            onAccepted: emailInput.forceInputFocus()
        }

        Spacer { gap: 13 }

        Text {
            text: "邮箱"
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSmall
        }

        Spacer { gap: 6 }

        AppInput {
            id: emailInput
            width: parent.width
            placeholder: "用来登录和找回密码"
            onTextEdited: bridge.email = text
            onAccepted: codeInput.forceInputFocus()
        }

        Spacer { gap: 13 }

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

        Spacer { gap: 13 }

        Row {
            width: parent.width
            spacing: 12

            Column {
                width: (parent.width - parent.spacing) / 2
                spacing: 6

                Text {
                    text: "密码"
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSmall
                }

                AppInput {
                    id: passwordInput
                    width: parent.width
                    placeholder: "6–15 位"
                    password: true
                    onRevealToggled: reveal = !reveal
                    onTextEdited: bridge.password = text
                    onAccepted: confirmInput.forceInputFocus()
                }
            }

            Column {
                width: (parent.width - parent.spacing) / 2
                spacing: 6

                Text {
                    text: "确认密码"
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSmall
                }

                AppInput {
                    id: confirmInput
                    width: parent.width
                    placeholder: "再输一次"
                    password: true
                    onRevealToggled: reveal = !reveal
                    onTextEdited: bridge.confirm = text
                    onAccepted: bridge.submit()
                }
            }
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
            text: bridge.done ? "账号已创建" : (bridge.busy ? "正在创建…" : "创建账号")
            busy: bridge.busy || bridge.done
            onClicked: bridge.submit()
        }

        Spacer { gap: 20 }

        AppLink {
            width: parent.width
            text: "已有账号，返回登录"
            horizontalAlignment: Text.AlignHCenter
            onClicked: bridge.go("login")
        }
    }

    Component.onCompleted: userInput.forceInputFocus()
}
