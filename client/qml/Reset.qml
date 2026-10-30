import QtQuick
import QtQuick.Layouts
import XiguChat

/**
 * 重置密码页：用户名 / 邮箱 / 验证码 / 新密码。
 *
 * 流程：先确认用户名与邮箱匹配，再获取验证码，最后提交新密码。
 * 校验与倒计时在 ResetBridge 里，页面只做展示与转发。
 */
Item {
    id: page

    ResetBridge {
        id: bridge
        objectName: "pageBridge"
    }

    AuthPage {
        anchors.fill: parent
        enabled: !(bridge.busy || bridge.codeBusy || bridge.done)

        Text {
            width: parent.width
            text: "重置密码"
            color: Theme.textPrimary
            font.pixelSize: Theme.fontTitle
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
        }

        Spacer { gap: 8 }

        Text {
            width: parent.width
            text: "验证邮箱后设置新密码"
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSmall
            horizontalAlignment: Text.AlignHCenter
        }

        Spacer { gap: 28 }

        AppInput {
            id: userInput
            width: parent.width
            placeholder: "用户名"
            onTextEdited: bridge.user = text
            onAccepted: emailInput.forceInputFocus()
        }

        Spacer { gap: 10 }

        AppInput {
            id: emailInput
            width: parent.width
            placeholder: "邮箱地址"
            onTextEdited: bridge.email = text
            onAccepted: codeInput.forceInputFocus()
        }

        Spacer { gap: 10 }

        RowLayout {
            width: parent.width
            spacing: 10

            AppInput {
                id: codeInput
                Layout.fillWidth: true
                placeholder: "验证码"
                onTextEdited: bridge.code = text
                onAccepted: passwordInput.forceInputFocus()
            }

            CodeButton {
                Layout.preferredWidth: 100
                text: bridge.countdown > 0 ? bridge.countdown + "s"
                                           : (bridge.codeBusy ? "发送中…" : "获取验证码")
                usable: bridge.countdown === 0 && !bridge.codeBusy && !bridge.busy && !bridge.done
                onClicked: bridge.sendCode()
            }
        }

        Spacer { gap: 10 }

        AppInput {
            id: passwordInput
            width: parent.width
            placeholder: "新密码（6~15 位）"
            password: true
            onRevealToggled: reveal = !reveal
            onTextEdited: bridge.password = text
            onAccepted: bridge.submit()
        }

        Spacer { gap: 8 }

        Text {
            width: parent.width
            height: 18
            text: bridge.tipText
            color: bridge.tipOk ? Theme.success : Theme.danger
            font.pixelSize: Theme.fontSmall
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            opacity: text.length > 0 ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 120 } }
        }

        Spacer { gap: 12 }

        AppButton {
            width: parent.width
            text: bridge.done ? "已重置" : (bridge.busy ? "提交中…" : "重置密码")
            busy: bridge.busy || bridge.done
            onClicked: bridge.submit()
        }

        Spacer { gap: 18 }

        AppLink {
            text: "返回登录"
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            onClicked: bridge.go("login")
        }
    }

    Component.onCompleted: userInput.forceInputFocus()
}
