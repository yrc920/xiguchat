import QtQuick
import QtQuick.Layouts
import XiguChat

/**
 * 登录页：居中单列，浅暖底，除主按钮外不出现任何实色块。
 *
 * 交互全在 LoginBridge 里：输入写入属性、错误由属性回读、按钮调 login()。
 * 回车在邮箱框跳到密码框，在密码框直接提交。
 */
Item {
    id: page

    LoginBridge {
        id: bridge
        objectName: "pageBridge"
    }

    AuthPage {
        anchors.fill: parent
        enabled: !(bridge.busy)

        // 品牌标识（固定 40×40，不随后面内容伸缩）
        Rectangle {
            width: 40
            height: 40
            radius: 13
            color: Theme.primary
            x: (parent.width - width) / 2

            Text {
                anchors.centerIn: parent
                text: "X"
                color: "#FFFFFF"
                font.pixelSize: 17
                font.bold: true
            }
        }

        Spacer { gap: 20 }

        // 标题与副标题：只做视觉层次，不参与状态
        Text {
            width: parent.width
            text: "欢迎回来"
            color: Theme.textPrimary
            font.pixelSize: Theme.fontTitle
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
        }

        Spacer { gap: 10 }

        Text {
            width: parent.width
            text: "登录你的账号，继续和朋友聊天"
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSmall
            horizontalAlignment: Text.AlignHCenter
        }

        Spacer { gap: 32 }

        // 邮箱框：回车把焦点交给密码框
        AppInput {
            id: emailInput
            width: parent.width
            placeholder: "邮箱地址"
            onTextEdited: bridge.email = text
            onAccepted: passwordInput.forceInputFocus()
        }

        Spacer { gap: 10 }

        // 密码框：右侧有可见性开关，回车直接提交
        AppInput {
            id: passwordInput
            width: parent.width
            placeholder: "密码"
            password: true
            reveal: bridge.passwordVisible
            onTextEdited: bridge.password = text
            onAccepted: bridge.login()
            onRevealToggled: bridge.togglePasswordVisible()
        }

        Spacer { gap: 6 }

        Text {
            width: parent.width
            height: 18
            text: bridge.errorText
            color: Theme.danger
            font.pixelSize: Theme.fontSmall
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            opacity: text.length > 0 ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 120 } }
        }

        Spacer { gap: 12 }

        // 主按钮：忙碌时显示过程文案并禁用
        AppButton {
            width: parent.width
            text: bridge.busy ? bridge.busyText : "登 录"
            busy: bridge.busy
            onClicked: bridge.login()
        }

        Spacer { gap: 18 }

        // 底部两个入口：忘记密码 / 注册新账号
        RowLayout {
            width: parent.width
            spacing: 0

            AppLink {
                text: "忘记密码？"
                onClicked: bridge.go("reset")
            }

            Item { Layout.fillWidth: true }

            AppLink {
                text: "注册新账号"
                emphasize: true
                onClicked: bridge.go("register")
            }
        }
    }

    // 进页面就聚焦邮箱框，直接可以输入
    Component.onCompleted: emailInput.forceInputFocus()
}
