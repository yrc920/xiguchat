import QtQuick
import QtQuick.Layouts
import XiguChat

/**
 * 注册页：用户名 / 邮箱 / 验证码 / 密码 / 确认密码，纵向单列。
 *
 * 逐项校验与倒计时都在 RegisterBridge 里；页面只负责把输入写进属性、
 * 把 tipText 与 busy 展示出来。回车按顺序跳到下一个输入框，
 * 最后一个（确认密码）回车直接提交。
 */
Item {
    id: page

    RegisterBridge {
        id: bridge
        objectName: "pageBridge"
    }

    AuthPage {
        anchors.fill: parent
        enabled: !(bridge.busy || bridge.codeBusy || bridge.done)

        // 标题
        Text {
            width: parent.width
            text: "创建账号"
            color: Theme.textPrimary
            font.pixelSize: Theme.fontTitle
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
        }

        Spacer { gap: 8 }

        Text {
            width: parent.width
            text: "注册后就能加好友、发消息"
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSmall
            horizontalAlignment: Text.AlignHCenter
        }

        Spacer { gap: 28 }

        // 用户名：回车跳到邮箱
        AppInput {
            id: userInput
            width: parent.width
            placeholder: "用户名"
            onTextEdited: bridge.user = text
            onAccepted: emailInput.forceInputFocus()
        }

        Spacer { gap: 10 }

        // 邮箱：回车跳到验证码
        AppInput {
            id: emailInput
            width: parent.width
            placeholder: "邮箱地址"
            onTextEdited: bridge.email = text
            onAccepted: codeInput.forceInputFocus()
        }

        Spacer { gap: 10 }

        // 验证码 + 获取按钮：倒计时期间按钮显示秒数并禁用
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

        // 密码：6~15 位，字符集由 bridge 校验
        AppInput {
            id: passwordInput
            width: parent.width
            placeholder: "密码（6~15 位）"
            password: true
            onRevealToggled: reveal = !reveal
            onTextEdited: bridge.password = text
            onAccepted: confirmInput.forceInputFocus()
        }

        Spacer { gap: 10 }

        // 确认密码：回车即提交
        AppInput {
            id: confirmInput
            width: parent.width
            placeholder: "确认密码"
            password: true
            onRevealToggled: reveal = !reveal
            onTextEdited: bridge.confirm = text
            onAccepted: bridge.submit()
        }

        Spacer { gap: 8 }

        // 提示行：成功绿、失败红，固定高度占位避免跳动
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

        // 提交按钮：成功态也保持禁用，防止重复注册
        AppButton {
            width: parent.width
            text: bridge.done ? "注册成功" : (bridge.busy ? "提交中…" : "注 册")
            busy: bridge.busy || bridge.done
            onClicked: bridge.submit()
        }

        Spacer { gap: 18 }

        // 返回登录
        AppLink {
            text: "已有账号？返回登录"
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            onClicked: bridge.go("login")
        }
    }

    // 进页面聚焦用户名
    Component.onCompleted: userInput.forceInputFocus()
}
