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

    Component.onCompleted: {
        // Load last opened file from config
        var lastFile = Config.getValue("lastOpenedFile", "")
        if (lastFile !== "") {
            renderer.source = lastFile
        }
    }

    VideoRenderer {
        id: renderer
        anchors.fill: parent
    }

    // Mouse inactivity detector
    Timer {
        id: hideTimer
        interval: 3000
        onTriggered: controlPanel.opacity = 0.0
    }

    // Overlay to detect mouse movement over video
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        propagateComposedEvents: true // Let events propagate (though VideoRenderer might not need them)
        
        onPositionChanged: {
            controlPanel.opacity = 1.0
            hideTimer.restart()
        }
        
        onClicked: {
            // Toggle play/pause on click
            if (renderer.state === 1) renderer.pause()
            else renderer.play()
            
            controlPanel.opacity = 1.0
            hideTimer.restart()
        }
    }

    // Drag-and-drop support for video files
    DropArea {
        id: dropArea
        anchors.fill: parent

        onDropped: function(drop) {
            if (drop.hasUrls && drop.urls.length > 0) {
                var url = drop.urls[0].toString()
                // Check if it's a video file by extension
                var ext = url.substring(url.lastIndexOf('.')).toLowerCase()
                var videoExtensions = [".mp4", ".mkv", ".avi", ".mov", ".wmv", ".flv", ".webm", ".m4v", ".mpg", ".mpeg", ".3gp"]
                if (videoExtensions.indexOf(ext) !== -1) {
                    renderer.source = url
                    Config.setValue("lastOpenedFile", url)
                    drop.accepted = true
                }
            }
        }

        Rectangle {
            id: dropOverlay
            anchors.fill: parent
            color: "#80000000"
            visible: dropArea.containsDrag
            z: 100

            Text {
                anchors.centerIn: parent
                text: "Drop video file here"
                color: "white"
                font.pixelSize: 24
                font.bold: true
            }
        }
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
            var fileUrl = ""
            if (typeof selectedFile !== 'undefined' && selectedFile) {
                fileUrl = selectedFile.toString()
            } else if (typeof selectedFiles !== 'undefined' && selectedFiles.length > 0) {
                fileUrl = selectedFiles[0].toString()
            }
            if (fileUrl !== "") {
                renderer.source = fileUrl
                Config.setValue("lastOpenedFile", fileUrl)
            }
        }
    }

    UrlInputDialog {
        id: urlDialog
        // Dialog handles its own centering or positioning
        onUrlAccepted: function(url) {
            renderer.source = url
            Config.setValue("lastOpenedFile", url)
        }
    }

    // --- Keyboard Shortcuts (Root Context) ---
    // Using Shortcut items instead of focusing an Item allows standard event propagation
    // and automatic enabling/disabling based on context (like dialogs).

    Shortcut {
        sequence: "Space"
        enabled: !urlDialog.visible
        onActivated: {
            if (renderer.state === 1) renderer.pause()
            else renderer.play()
        }
    }

    Shortcut {
        sequence: "Left"
        enabled: !urlDialog.visible
        onActivated: {
            if (renderer.duration > 0) {
                var newPos = Math.max(0, renderer.position - 5000)
                renderer.seek(newPos)
            }
        }
    }

    Shortcut {
        sequence: "Right"
        enabled: !urlDialog.visible
        onActivated: {
            if (renderer.duration > 0) {
                var newPos = Math.min(renderer.duration, renderer.position + 5000)
                renderer.seek(newPos)
            }
        }
    }

    Shortcut {
        sequence: "Up"
        enabled: !urlDialog.visible
        onActivated: renderer.volume = Math.min(1.0, renderer.volume + 0.05)
    }

    Shortcut {
        sequence: "Down"
        enabled: !urlDialog.visible
        onActivated: renderer.volume = Math.max(0.0, renderer.volume - 0.05)
    }

    Shortcut {
        sequence: "F"
        enabled: !urlDialog.visible
        onActivated: root.visibility = root.visibility === Window.FullScreen ? Window.Windowed : Window.FullScreen
    }

    Shortcut {
        sequence: "O"
        enabled: !urlDialog.visible
        onActivated: fileDialog.open()
    }

    Shortcut {
        sequence: "Q"
        context: Qt.ApplicationShortcut
        onActivated: Qt.quit()
    }

    Shortcut {
        sequence: "M"
        enabled: !urlDialog.visible
        onActivated: renderer.muted = !renderer.muted
    }

    Shortcut {
        sequence: "U"
        enabled: !urlDialog.visible
        onActivated: urlDialog.open()
    }

    // Modern control panel with time display and smooth seeking
    ControlPanel {
        id: controlPanel
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        renderer: renderer
        formatTimeFunc: formatTime
        
        // Auto-hide logic
        opacity: 1.0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: 300 } }

        // Keep visible when hovering control panel
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            propagateComposedEvents: true
            onEntered: {
                controlPanel.opacity = 1.0
                hideTimer.stop()
            }
            onExited: hideTimer.restart()
            onPressed: (mouse) => { mouse.accepted = false }
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
