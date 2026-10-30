import QtQuick

/**
 * 好友资料页：头像 + 名字（带性别）+ 昵称 / 备注 / 标签，下面是三个动作。
 *
 * 数据由外部通过 profile 传入（ChatBridge::profile 的 QVariantMap），
 * 页面只负责展示，点「发消息」用信号抛回外层。
 *
 * 昵称、备注和性别来自服务端；标签只用于本地界面：
 * 没有值时显示 —，性别为 0 时不画图标。语音、视频暂时没有实现，
 * 图标显示为不可用状态。
 */
Item {
    id: root

    // uid / name / nick / remark / desc / sex / initial
    property var profile: ({})

    // 点「发消息」时抛出，由外层切到会话页
    signal chatRequested()

    // 性别：1 男 / 2 女，其它值（含未设置 0）不显示图标
    readonly property int sex: root.profile.sex === undefined ? 0 : root.profile.sex

    Rectangle {
        anchors.fill: parent
        color: Theme.chatBg
    }

    // 内容整体居中，宽度设上限，窗口拉宽时不会把信息扯散
    Column {
        width: Math.min(parent.width - 96, 480)
        anchors.top: parent.top
        anchors.topMargin: Math.max(48, parent.height * 0.12)
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 0

        Row {
            spacing: 18

            Avatar {
                width: 76
                height: 76
                source: root.profile.icon || ""
                name: root.profile.name || ""
                textSize: 26
            }

            // 右侧信息列：名字 + 昵称 / 备注 / 标签
            Column {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.parent.width - 76 - 18
                spacing: 7

                Row {
                    spacing: 7

                    // 名字过长时省略，别把性别图标挤出可视区
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.profile.name || ""
                        color: Theme.textPrimary
                        font.pixelSize: 20
                        font.bold: true
                        elide: Text.ElideRight
                        width: Math.min(implicitWidth, parent.parent.width - 26)
                    }

                    // 性别图标：Canvas 画的 ♂ / ♀，和动作图标同一套线条
                    Canvas {
                        anchors.verticalCenter: parent.verticalCenter
                        width: 16
                        height: 18
                        visible: root.sex === 1 || root.sex === 2

                        property int sex: root.sex
                        onSexChanged: requestPaint()

                        onPaint: {
                            const ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)
                            ctx.lineWidth = 1.5
                            ctx.lineCap = "round"

                            if (sex === 1) {
                                // ♂：圆 + 右上箭头
                                ctx.strokeStyle = "#5B7DA8"
                                ctx.beginPath()
                                ctx.arc(7, 11.5, 5, 0, Math.PI * 2)
                                ctx.stroke()

                                ctx.beginPath()
                                ctx.moveTo(10.8, 7.8)
                                ctx.lineTo(14.5, 4)
                                ctx.moveTo(10.4, 4)
                                ctx.lineTo(14.5, 4)
                                ctx.lineTo(14.5, 8)
                                ctx.stroke()
                            } else {
                                // ♀：圆 + 下方十字
                                ctx.strokeStyle = "#B0808F"
                                ctx.beginPath()
                                ctx.arc(8, 7.5, 5, 0, Math.PI * 2)
                                ctx.stroke()

                                ctx.beginPath()
                                ctx.moveTo(8, 12.5)
                                ctx.lineTo(8, 17)
                                ctx.moveTo(5.4, 14.6)
                                ctx.lineTo(10.6, 14.6)
                                ctx.stroke()
                            }
                        }
                    }
                }

                Text {
                    width: parent.width
                    text: "昵称：" + (root.profile.nick || "—")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSmall
                    elide: Text.ElideRight
                }

                Text {
                    width: parent.width
                    text: "备注：" + (root.profile.remark || "—")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSmall
                    elide: Text.ElideRight
                }

                Text {
                    width: parent.width
                    text: "标签：" + (root.profile.tags || "—")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSmall
                    elide: Text.ElideRight
                }
            }
        }

        Spacer { gap: 32 }

        Rectangle {
            width: parent.width
            height: 1
            color: Theme.fieldBorder
        }

        Spacer { gap: 26 }

        // 三个动作：只有「发消息」可用，语音 / 视频等后端具备能力后再打开
        Row {
            spacing: 18

            IconButton {
                kind: "chat"
                label: "发消息"
                onClicked: root.chatRequested()
            }

            IconButton {
                kind: "voice"
                label: "语音通话"
                available: false
            }

            IconButton {
                kind: "video"
                label: "视频通话"
                available: false
            }
        }
    }
}
