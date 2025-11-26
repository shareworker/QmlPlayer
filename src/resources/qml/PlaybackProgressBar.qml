import QtQuick 2.15
import QtQuick.Controls 2.15

Row {
    id: root
    spacing: 8

    // External API
    property real duration: 0        // total duration in ms
    property real position: 0        // current position in ms
    property var formatTimeFunc: null

    signal seekRequested(real positionMs)

    function formattedTime(ms) {
        if (formatTimeFunc) {
            return formatTimeFunc(ms)
        }
        if (ms <= 0) return "00:00"
        var totalSeconds = Math.floor(ms / 1000)
        var minutes = Math.floor(totalSeconds / 60)
        var seconds = totalSeconds % 60
        return (minutes < 10 ? "0" : "") + minutes + ":" + (seconds < 10 ? "0" : "") + seconds
    }

    // Current time
    Text {
        id: currentTime
        width: 50
        color: "#ffffff"
        font.pixelSize: 11
        horizontalAlignment: Text.AlignRight
        text: root.formattedTime(progress.dragging ? progress.dragValue : root.position)
    }

    // Progress slider with drag-to-seek behavior
    Slider {
        id: progress
        width: parent.width - 120
        from: 0
        to: root.duration > 0 ? root.duration : 1
        enabled: root.duration > 0

        property bool dragging: false
        property real dragValue: 0

        value: dragging ? dragValue : root.position

        onPressedChanged: {
            dragging = pressed
            if (pressed) {
                dragValue = value
            } else if (root.duration > 0) {
                root.seekRequested(dragValue)
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

    // Total time
    Text {
        id: totalTime
        width: 50
        color: "#ffffff"
        font.pixelSize: 11
        text: root.formattedTime(root.duration)
    }
}
