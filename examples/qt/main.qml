import QtQuick 2.0
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.1

Item {
    width: 800
    height: 600

    focus: true
    Keys.onEscapePressed: Qt.quit()

    Rectangle {
        id: background
        color: "lightblue"
        anchors.fill: parent
    }
    Item {
        id: content
        anchors.fill: parent

        Repeater {
            model: items
            Particle {
                circle: true
                color: model.color
                y: position.y
                x: position.x
                radius: model.radius
                rotation: model.rotation
            }
        }
    }
    ColumnLayout {
        z: 1
        anchors.top: parent.top
        width: parent.width - 10
        RowLayout {
            Layout.margins: 5
            Layout.fillWidth: true
            Label {
                text: "Running:"
            }
            Switch {
                checked: engine.running
                onCheckedChanged: engine.running = checked
            }
            Label {
                text: "Stepsize:"
            }
            TextField {
                id: stepSizeField
                text: "0.02"
                Layout.preferredWidth: 80
                onAccepted: engine.step(stepSizeField.text)
            }
            Button {
                text: "Step"
                onClicked: engine.step(stepSizeField.text)
            }

            Item {
                Layout.fillWidth: true
            }
            Label {
                text: "Target FPS:"
            }
            TextField {
                Layout.preferredWidth: 60
                text: engine.fps
                onAccepted: engine.fps = text
            }
            Label {
                Layout.preferredWidth: 80
                text: "Real: " + engine.realFps.toFixed(1)
            }
        }
        Label {
            Layout.minimumWidth: 100
            text: qsTr("Frames: %1").arg(engine.frames)
        }
        Label {
            text: qsTr("Count: %1 entities").arg(engine.entityCount)
        }
    }
}
