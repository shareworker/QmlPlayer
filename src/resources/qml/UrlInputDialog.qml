import QtQuick 2.15
import QtQuick.Controls 2.15

Dialog {
    id: root
    title: "Open Network Stream"
    modal: true
    standardButtons: Dialog.Ok | Dialog.Cancel
    
    // Center the dialog in the parent window
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    property alias url: urlField.text
    signal urlAccepted(string url)

    width: 400
    
    contentItem: Column {
        spacing: 15
        padding: 5

        Text {
            text: "Enter stream URL:"
            color: "#333"
            font.bold: true
        }

        TextField {
            id: urlField
            width: parent.width
            placeholderText: "rtsp://example.com/stream"
            selectByMouse: true
            focus: true
            
            // Handle Enter key to accept
            Keys.onReturnPressed: root.accept()
            Keys.onEnterPressed: root.accept()
        }
    }

    onAccepted: {
        if (urlField.text.trim() !== "") {
            urlAccepted(urlField.text.trim())
        }
    }

    onOpened: {
        urlField.text = ""
        // Force focus to TextField when dialog opens
        urlField.forceActiveFocus()
    }
}
