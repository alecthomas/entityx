import QtQuick 2.0

Item {
    property color color
    property int radius
    property real rotation
    property bool circle: false
    Rectangle {
        color: parent.color
        rotation: parent.rotation
        radius: parent.circle ? width : 0
        x: -parent.radius
        y: -parent.radius
        width: 2*parent.radius
        height: 2*parent.radius
    }
}
