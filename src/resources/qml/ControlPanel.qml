import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    id: controlPanel
    height: 80

    // External API
    property var renderer
    property var formatTimeFunc

    gradient: Gradient {
        GradientStop { position: 0.0; color: "#00000000" }
        GradientStop { position: 0.3; color: "#aa000000" }
        GradientStop { position: 1.0; color: "#dd000000" }
    }

    Column {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        anchors.topMargin: 8
        anchors.bottomMargin: 10
        spacing: 6

        // Progress bar with time labels
        PlaybackProgressBar {
            id: progressBar
            width: parent.width
            duration: renderer ? renderer.duration : 0
            position: renderer ? renderer.position : 0
            formatTimeFunc: controlPanel.formatTimeFunc
            onSeekRequested: function(positionMs) {
                if (renderer)
                    renderer.seek(positionMs)
            }
        }

        // Control buttons
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 16

            // Stop button
            PlayerButton {
                id: stopButton
                baseColor: "#3d7cff"
                hoverColor: "#4a8fff"
                pressedColor: "#2d6cdf"

                onClicked: if (renderer) renderer.stop()

                Rectangle {
                    anchors.centerIn: parent
                    width: 14
                    height: 14
                    radius: 3
                    color: "white"
                }
            }

            // Play/Pause button
            PlayerButton {
                id: playPauseButton
                baseColor: "#3d7cff"
                hoverColor: "#4a8fff"
                pressedColor: "#2d6cdf"

                onClicked: {
                    if (!renderer)
                        return
                    if (renderer.state === 1) {
                        renderer.pause()
                    } else {
                        renderer.play()
                    }
                }

                // Play icon (triangle)
                Canvas {
                    id: playIcon
                    anchors.fill: parent
                    visible: renderer && renderer.state !== 1
                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.reset()
                        ctx.fillStyle = "white"
                        var w = width
                        var h = height
                        ctx.beginPath()
                        ctx.moveTo(w * 0.38, h * 0.28)
                        ctx.lineTo(w * 0.38, h * 0.72)
                        ctx.lineTo(w * 0.70, h * 0.50)
                        ctx.closePath()
                        ctx.fill()
                    }
                }

                // Pause icon (double bars)
                Row {
                    anchors.centerIn: parent
                    spacing: 4
                    visible: renderer && renderer.state === 1

                    Rectangle {
                        width: 4
                        height: 16
                        radius: 2
                        color: "white"
                    }
                    Rectangle {
                        width: 4
                        height: 16
                        radius: 2
                        color: "white"
                    }
                }
            }

            // Volume controls container
            Row {
                spacing: 8
                anchors.verticalCenter: parent.verticalCenter

                // Mute/Unmute button
                Rectangle {
                    width: 32
                    height: 32
                    radius: 16
                    color: muteBtn.pressed ? "#2d6cdf" : (muteBtn.containsMouse ? "#4a8fff" : "#3d7cff")

                    // Speaker icon
                    Canvas {
                        anchors.fill: parent
                        onPaint: {
                            if (!renderer)
                                return
                            var ctx = getContext("2d")
                            ctx.reset()
                            ctx.fillStyle = "white"
                            ctx.strokeStyle = "white"
                            ctx.lineWidth = 2
                            var w = width
                            var h = height

                            if (!renderer.muted) {
                                // Speaker cone
                                ctx.beginPath()
                                ctx.moveTo(w * 0.30, h * 0.40)
                                ctx.lineTo(w * 0.40, h * 0.40)
                                ctx.lineTo(w * 0.50, h * 0.30)
                                ctx.lineTo(w * 0.50, h * 0.70)
                                ctx.lineTo(w * 0.40, h * 0.60)
                                ctx.lineTo(w * 0.30, h * 0.60)
                                ctx.closePath()
                                ctx.fill()

                                // Sound waves
                                if (renderer.volume > 0.3) {
                                    ctx.beginPath()
                                    ctx.arc(w * 0.50, h * 0.50, w * 0.15, -Math.PI/4, Math.PI/4, false)
                                    ctx.stroke()
                                }
                                if (renderer.volume > 0.6) {
                                    ctx.beginPath()
                                    ctx.arc(w * 0.50, h * 0.50, w * 0.22, -Math.PI/4, Math.PI/4, false)
                                    ctx.stroke()
                                }
                            } else {
                                // Speaker cone (muted)
                                ctx.beginPath()
                                ctx.moveTo(w * 0.30, h * 0.40)
                                ctx.lineTo(w * 0.40, h * 0.40)
                                ctx.lineTo(w * 0.50, h * 0.30)
                                ctx.lineTo(w * 0.50, h * 0.70)
                                ctx.lineTo(w * 0.40, h * 0.60)
                                ctx.lineTo(w * 0.30, h * 0.60)
                                ctx.closePath()
                                ctx.fill()

                                // X mark
                                ctx.beginPath()
                                ctx.moveTo(w * 0.58, h * 0.42)
                                ctx.lineTo(w * 0.68, h * 0.58)
                                ctx.moveTo(w * 0.68, h * 0.42)
                                ctx.lineTo(w * 0.58, h * 0.58)
                                ctx.stroke()
                            }
                        }
                    }

                    MouseArea {
                        id: muteBtn
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: if (renderer) renderer.muted = !renderer.muted
                    }

                    Connections {
                        target: renderer
                        function onMutedChanged() {
                            parent.requestPaint()
                        }
                        function onVolumeChanged() {
                            parent.requestPaint()
                        }
                    }
                }

                // Volume slider
                Slider {
                    id: volumeSlider
                    width: 80
                    from: 0
                    to: 1.0
                    anchors.verticalCenter: parent.verticalCenter

                    value: renderer ? renderer.volume : 0

                    onValueChanged: {
                        if (renderer && !pressed) {
                            renderer.volume = value
                        }
                    }

                    onPressedChanged: {
                        if (!pressed && renderer) {
                            renderer.volume = value
                        }
                    }

                    background: Rectangle {
                        x: volumeSlider.leftPadding
                        y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - height / 2
                        implicitWidth: 80
                        implicitHeight: 4
                        width: volumeSlider.availableWidth
                        height: implicitHeight
                        radius: 2
                        color: "#404040"

                        Rectangle {
                            width: volumeSlider.visualPosition * parent.width
                            height: parent.height
                            color: renderer && renderer.muted ? "#666666" : "#3d7cff"
                            radius: 2
                        }
                    }

                    handle: Rectangle {
                        x: volumeSlider.leftPadding + volumeSlider.visualPosition * (volumeSlider.availableWidth - width)
                        y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - height / 2
                        implicitWidth: 12
                        implicitHeight: 12
                        radius: 6
                        color: volumeSlider.pressed ? "#5a8fff" : "#3d7cff"
                        border.color: "#ffffff"
                        border.width: 2
                    }
                }
            }
        }
    }
}
