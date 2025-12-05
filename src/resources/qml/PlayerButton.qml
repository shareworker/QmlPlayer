import QtQuick 2.15

Item {
    id: root
    width: 44
    height: 44

    property color iconColor: "white"
    
    default property alias content: contentItem.data
    signal clicked()

    // Background for touch feedback
    Rectangle {
        anchors.centerIn: parent
        width: Math.min(parent.width, parent.height)
        height: width
        radius: width / 2
        color: "white"
        opacity: mouse.pressed ? 0.2 : 0
        visible: mouse.pressed
        
        Behavior on opacity { NumberAnimation { duration: 100 } }
    }
    
    // Scale animation on press
    scale: mouse.pressed ? 0.9 : 1.0
    Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutQuad } }

    Item {
        id: contentItem
        anchors.centerIn: parent
        width: 24
        height: 24
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
