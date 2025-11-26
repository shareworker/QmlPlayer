import QtQuick 2.15

Rectangle {
    id: root
    width: 40
    height: 40
    radius: width / 2

    property color baseColor: "#3d7cff"
    property color hoverColor: "#4a8fff"
    property color pressedColor: "#2d6cdf"

    color: mouse.pressed ? pressedColor : (mouse.containsMouse ? hoverColor : baseColor)

    default property alias content: contentItem.data

    signal clicked()

    Item {
        id: contentItem
        anchors.fill: parent
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
