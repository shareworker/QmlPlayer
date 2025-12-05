import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: controlPanel
    height: 90
    color: "transparent"
    
    // Gradient background
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "transparent" }
            GradientStop { position: 1.0; color: "#cc000000" }
        }
    }
    
    property var renderer
    property var formatTimeFunc

    MouseArea { anchors.fill: parent }

    // Progress Bar at top
    PlaybackProgressBar {
        id: progressBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: 20
        anchors.rightMargin: 20
        anchors.topMargin: 10
        height: 20
        duration: renderer ? renderer.duration : 0
        position: renderer ? renderer.position : 0
        formatTimeFunc: controlPanel.formatTimeFunc
        onSeekRequested: function(positionMs) {
            if (renderer) renderer.seek(positionMs)
        }
    }

    // Controls centered between progress bar and bottom
    Item {
        id: controlsArea
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: progressBar.bottom
        anchors.bottom: parent.bottom
        anchors.leftMargin: 20
        anchors.rightMargin: 20

        RowLayout {
            anchors.centerIn: parent
            spacing: 20

            // Left spacer
            Item { Layout.fillWidth: true }

            // Playback Controls
            Row {
                spacing: 30
                Layout.alignment: Qt.AlignVCenter
                
                // Stop Button
                PlayerButton {
                    width: 36
                    height: 36
                    anchors.verticalCenter: parent.verticalCenter
                    onClicked: if (renderer) renderer.stop()
                    Rectangle {
                        anchors.centerIn: parent
                        width: 14
                        height: 14
                        radius: 2
                        color: "white"
                    }
                }

                // Play/Pause Button
                PlayerButton {
                    width: 50
                    height: 50
                    anchors.verticalCenter: parent.verticalCenter
                    onClicked: {
                        if (!renderer) return
                        if (renderer.state === 1) renderer.pause()
                        else renderer.play()
                    }

                    Item {
                        anchors.centerIn: parent
                        width: 20
                        height: 20

                        Canvas {
                            anchors.fill: parent
                            visible: renderer && renderer.state !== 1
                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.reset()
                                ctx.fillStyle = "white"
                                ctx.beginPath()
                                ctx.moveTo(2, 0)
                                ctx.lineTo(20, 10)
                                ctx.lineTo(2, 20)
                                ctx.closePath()
                                ctx.fill()
                            }
                        }

                        Row {
                            anchors.centerIn: parent
                            spacing: 5
                            visible: renderer && renderer.state === 1
                            Rectangle { width: 5; height: 20; radius: 2; color: "white" }
                            Rectangle { width: 5; height: 20; radius: 2; color: "white" }
                        }
                    }
                }
                
                // Placeholder for symmetry
                Item { width: 36; height: 36; anchors.verticalCenter: parent.verticalCenter }
            }

            // Right: Volume Control
            Row {
                spacing: 8
                Layout.alignment: Qt.AlignVCenter
                
                Item {
                    width: 20
                    height: 20
                    MouseArea {
                        anchors.fill: parent
                        onClicked: if (renderer) renderer.muted = !renderer.muted
                    }
                    
                    Canvas {
                        id: speakerCanvas
                        anchors.fill: parent
                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.reset()
                            ctx.strokeStyle = "white"
                            ctx.fillStyle = "white"
                            ctx.lineWidth = 1.5
                            
                            ctx.beginPath()
                            ctx.moveTo(6, 6)
                            ctx.lineTo(0, 6)
                            ctx.lineTo(0, 14)
                            ctx.lineTo(6, 14)
                            ctx.lineTo(11, 19)
                            ctx.lineTo(11, 1)
                            ctx.closePath()
                            ctx.fill()

                            if (renderer && !renderer.muted) {
                                ctx.beginPath()
                                ctx.arc(11, 10, 4, -Math.PI/3, Math.PI/3)
                                ctx.stroke()
                                if (renderer.volume > 0.5) {
                                    ctx.beginPath()
                                    ctx.arc(11, 10, 7, -Math.PI/3, Math.PI/3)
                                    ctx.stroke()
                                }
                            } else {
                                ctx.beginPath()
                                ctx.moveTo(13, 13)
                                ctx.lineTo(19, 7)
                                ctx.moveTo(13, 7)
                                ctx.lineTo(19, 13)
                                ctx.stroke()
                            }
                        }
                    }
                    Connections {
                        target: renderer
                        function onMutedChanged() { speakerCanvas.requestPaint() }
                        function onVolumeChanged() { speakerCanvas.requestPaint() }
                    }
                }

                Slider {
                    id: volumeSlider
                    width: 100
                    anchors.verticalCenter: parent.verticalCenter
                    from: 0
                    to: 1.0
                    value: renderer ? renderer.volume : 0
                    onMoved: if (renderer) renderer.volume = value

                    background: Rectangle {
                        x: volumeSlider.leftPadding
                        y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - height / 2
                        width: volumeSlider.availableWidth
                        height: 4
                        radius: 2
                        color: "#3a3a3c"
                        Rectangle {
                            width: volumeSlider.visualPosition * parent.width
                            height: parent.height
                            color: "#ffffff"
                            radius: 2
                        }
                    }
                    
                    handle: Rectangle {
                        x: volumeSlider.leftPadding + volumeSlider.visualPosition * (volumeSlider.availableWidth - width)
                        y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - height / 2
                        width: 12
                        height: 12
                        radius: 6
                        color: "white"
                    }
                }
            }

            // Right spacer
            Item { Layout.fillWidth: true }
        }
    }
}
