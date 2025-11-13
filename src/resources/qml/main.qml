import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Dialogs
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
}
