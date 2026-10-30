import QtQuick
import XiguChat

/**
 * 应用根窗口。
 *
 * 只做三件事，不承载任何业务：
 *   1. 按 ShellBridge 的 page 决定显示哪个页面（Loader + setSource）；
 *   2. 页面切换时同步调整窗口尺寸（认证页固定小窗，聊天页可拉伸）；
 *   3. 掉线时盖一层提示，确认后回到登录页。
 *
 * 页面本身互不认识：需要跳转时调 bridge 的 go("xxx")，由 ShellBridge 改 page。
 */
Window {
    id: window

    visible: true
    title: "xiguchat"
    color: Theme.bgTop

    // 全局唯一的导航状态持有者；窗口里只允许有一个实例
    ShellBridge {
        id: shell
    }

    // 认证类页面统一宽度（登录 470 / 注册 560 / 重置 500 高）
    readonly property int authWidth: 380

    /** 页面标识 → QML 文件名。默认回落到登录页，避免未知值把窗口留空。 */
    function pageFile(name) {
        if (name === "chat")
            return "ChatWindow.qml"
        if (name === "register")
            return "Register.qml"
        if (name === "reset")
            return "Reset.qml"
        return "Login.qml"
    }

    /**
     * 按页面类型设置窗口尺寸。
     *
     * 顺序有讲究：先放开上限再收紧下限。Qt 的 Window 不允许 max < min，
     * 反过来写会被夹住，窗口停在上一个页面的大小上。
     */
    function applySize(name) {
        maximumWidth = 16777215
        maximumHeight = 16777215

        // 聊天页可拉伸，只给下限
        if (name === "chat") {
            minimumWidth = 900
            minimumHeight = 650
            if (width < 900)
                width = 900
            if (height < 650)
                height = 650
            return
        }

        // 认证页是定尺寸小窗：宽高上下限设成同一个值
        const h = name === "register" ? 560 : (name === "reset" ? 500 : 470)
        minimumWidth = authWidth
        minimumHeight = h
        maximumWidth = authWidth
        maximumHeight = h
        width = authWidth
        height = h
    }

    /** 换页：重建页面 + 调整窗口尺寸。 */
    function showPage(name) {
        // 用 setSource 而不是绑定 source：同一页面重新进入（比如掉线回登录）
        // 必须重建，否则表单内容与错误提示会留着上一次的状态
        loader.setSource(pageFile(name))
        applySize(name)
    }

    // 首帧前就把页面和尺寸定好，避免窗口先以默认大小闪一下
    Component.onCompleted: showPage(shell.page)

    // ShellBridge 改 page 时跟着换页
    Connections {
        target: shell

        function onPageChanged() { window.showPage(shell.page) }
    }

    // 页面容器：铺满窗口，具体页面由 setSource 决定
    Loader {
        id: loader
        enabled: shell.notice.length === 0
        anchors.fill: parent
    }

    // ---------------- 掉线提示 ----------------
    // 服务端踢人 / 长连接断开时由 ShellBridge 写入 notice，这里只负责呈现
    Item {
        anchors.fill: parent
        visible: shell.notice.length > 0
        z: 100 // 压在页面与页面内弹层之上

        // 半透明遮罩，挡住下层交互
        Rectangle {
            anchors.fill: parent
            color: "#59000000"
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.AllButtons
                onWheel: (wheel) => { wheel.accepted = true }
            }
        }

        Rectangle {
            width: 320
            height: body.height + 52
            anchors.centerIn: parent
            radius: 14
            color: Theme.panelBg

            Column {
                id: body
                x: 24
                y: 26
                width: parent.width - 48
                spacing: 0

                Text {
                    text: "已离线"
                    color: Theme.textPrimary
                    font.pixelSize: 17
                    font.bold: true
                }

                Spacer { gap: 10 }

                // 具体原因：异地登录 / 连接断开，由 ShellBridge 给出
                Text {
                    width: parent.width
                    text: shell.notice
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSmall
                    wrapMode: Text.WordWrap
                }

                Spacer { gap: 20 }

                // 确认后关闭会话并回登录页（由 ShellBridge 处理）
                Rectangle {
                    width: parent.width
                    height: 40
                    radius: 10
                    color: backArea.containsMouse ? Theme.primaryHover : Theme.primary

                    Text {
                        anchors.centerIn: parent
                        text: "重新登录"
                        color: "#FFFFFF"
                        font.pixelSize: Theme.fontBody
                    }

                    MouseArea {
                        id: backArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: shell.dismissNotice()
                    }
                }
            }
        }
    }
}
