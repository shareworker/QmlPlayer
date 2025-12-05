import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root
    height: 30

    // External API
    property real duration: 0        // total duration in ms
    property real position: 0        // current position in ms
    property var formatTimeFunc: null

    signal seekRequested(real positionMs)

    function formattedTime(ms) {
        if (formatTimeFunc) {
            return formatTimeFunc(ms)
        }
        if (ms <= 0) return "0:00"
        var totalSeconds = Math.floor(ms / 1000)
        var minutes = Math.floor(totalSeconds / 60)
        var seconds = totalSeconds % 60
        return minutes + ":" + (seconds < 10 ? "0" : "") + seconds
    }

    // Layout
    Row {
        anchors.fill: parent
        spacing: 10
        
        // Current time
        Text {
            width: 40
            anchors.verticalCenter: parent.verticalCenter
            color: "#8e8e93" // iOS gray
            font.pixelSize: 12
            font.family: "SF Pro Text" // Fallback to system
            horizontalAlignment: Text.AlignRight
            text: root.formattedTime(progress.pressed ? progress.value : root.position)
        }

        // Slider
        Slider {
            id: progress
            width: parent.width - 100
            anchors.verticalCenter: parent.verticalCenter
            from: 0
            to: root.duration > 0 ? root.duration : 1
            enabled: root.duration > 0
            
            value: pressed ? value : root.position

            onPressedChanged: {
                if (!pressed && root.duration > 0) {
                    root.seekRequested(value)
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
                color: "#3a3a3c" // Darker track

                Rectangle {
                    width: progress.visualPosition * parent.width
                    height: parent.height
                    color: "#ffffff" // White progress
                    radius: 2
                }
            }

            handle: Rectangle {
                x: progress.leftPadding + progress.visualPosition * (progress.availableWidth - width)
                y: progress.topPadding + progress.availableHeight / 2 - height / 2
                implicitWidth: progress.pressed ? 16 : 8
                implicitHeight: progress.pressed ? 16 : 8
                radius: width / 2
                color: "#ffffff"
                
                Behavior on implicitWidth { NumberAnimation { duration: 150 } }
                Behavior on implicitHeight { NumberAnimation { duration: 150 } }
            }
        }

        // Total time
        Text {
            width: 40
            anchors.verticalCenter: parent.verticalCenter
            color: "#8e8e93"
            font.pixelSize: 12
            horizontalAlignment: Text.AlignLeft
            text: root.formattedTime(root.duration)
        }
    }
}
