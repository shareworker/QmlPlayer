import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Dialogs
import QtQuick.Controls
import VideoPlayer 1.0

Window {
    id: root
    visible: true
    width: 800
    height: 600
    title: "QmlPlayer"

    VideoRenderer {
        id: renderer
        anchors.fill: parent
        anchors.bottomMargin: 80
        // Set a test file path to verify rendering, or leave empty to start without playback
        // Example (uncomment and adjust path):
        // source: "D:/videos/test.mp4"
    }

    Rectangle {
        id: openBtn
        width: 80
        height: 32
        radius: 6
        color: mouseArea.pressed ? "#2d6cdf" : "#3d7cff"
        border.color: "#1f5bd6"
        anchors.margins: 12
        anchors.left: parent.left
        anchors.top: parent.top
        Text {
            anchors.centerIn: parent
            text: "Open"
            color: "white"
        }
        MouseArea {
            id: mouseArea
            anchors.fill: parent
            onClicked: fileDialog.open()
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
        }
    }

    FileDialog {
        id: fileDialog
        title: "Choose a media file"
        fileMode: FileDialog.OpenFile
        onAccepted: {
            if (typeof selectedFile !== 'undefined' && selectedFile) {
                renderer.source = selectedFile.toString();
            } else if (typeof selectedFiles !== 'undefined' && selectedFiles.length > 0) {
                renderer.source = selectedFiles[0].toString();
            }
        }
    }

    // Modern control panel with time display and smooth seeking
    Rectangle {
        id: controlPanel
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 80
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
            Row {
                width: parent.width
                spacing: 8

                Text {
                    id: currentTime
                    width: 50
                    color: "#ffffff"
                    font.pixelSize: 11
                    horizontalAlignment: Text.AlignRight
                    text: formatTime(progress.dragging ? progress.dragValue : renderer.position)
                }

                Slider {
                    id: progress
                    width: parent.width - 120
                    from: 0
                    to: renderer.duration > 0 ? renderer.duration : 1
                    enabled: renderer.duration > 0

                    // Local drag state so we don't fight with decoder updates
                    property bool dragging: false
                    property real dragValue: 0

                    value: dragging ? dragValue : renderer.position

                    onPressedChanged: {
                        dragging = pressed
                        if (pressed) {
                            dragValue = value
                        } else if (renderer.duration > 0) {
                            renderer.seek(dragValue)
                        }
                    }

                    onValueChanged: {
                        if (dragging) {
                            dragValue = value
                        }
                    }

                    background: Rectangle {
                        x: progress.leftPadding
                        y: progress.topPadding + progress.availableHeight / 2 - height / 2
                        implicitWidth: 200
                        implicitHeight: 4
                        width: progress.availableWidth
                        height: implicitHeight
                        radius: 2
                        color: "#404040"

                        Rectangle {
                            width: progress.visualPosition * parent.width
                            height: parent.height
                            color: "#3d7cff"
                            radius: 2
                        }
                    }

                    handle: Rectangle {
                        x: progress.leftPadding + progress.visualPosition * (progress.availableWidth - width)
                        y: progress.topPadding + progress.availableHeight / 2 - height / 2
                        implicitWidth: 14
                        implicitHeight: 14
                        radius: 7
                        color: progress.pressed ? "#5a8fff" : "#3d7cff"
                        border.color: "#ffffff"
                        border.width: 2
                    }
                }

                Text {
                    id: totalTime
                    width: 50
                    color: "#ffffff"
                    font.pixelSize: 11
                    text: formatTime(renderer.duration)
                }
            }

            // Control buttons
            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 12

                // Stop button
                Rectangle {
                    width: 40
                    height: 40
                    radius: 20
                    color: stopBtn.pressed ? "#2d6cdf" : (stopBtn.containsMouse ? "#4a8fff" : "#3d7cff")

                    Rectangle {
                        anchors.centerIn: parent
                        width: 14
                        height: 14
                        radius: 3
                        color: "white"
                    }

                    MouseArea {
                        id: stopBtn
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: renderer.stop()
                    }
                }

                // Play/Pause button
                Rectangle {
                    width: 40
                    height: 40
                    radius: 20
                    color: playPauseBtn.pressed ? "#2d6cdf" : (playPauseBtn.containsMouse ? "#4a8fff" : "#3d7cff")

                    // Play icon (triangle)
                    Canvas {
                        id: playIcon
                        anchors.fill: parent
                        visible: renderer.state !== 1
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
                        visible: renderer.state === 1

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

                    MouseArea {
                        id: playPauseBtn
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (renderer.state === 1) {
                                renderer.pause()
                            } else {
                                renderer.play()
                            }
                        }
                    }
                }
            }
        }
    }

    function formatTime(ms) {
        if (ms <= 0) return "00:00"
        var totalSeconds = Math.floor(ms / 1000)
        var minutes = Math.floor(totalSeconds / 60)
        var seconds = totalSeconds % 60
        return (minutes < 10 ? "0" : "") + minutes + ":" + (seconds < 10 ? "0" : "") + seconds
    }
}
