import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: importPopup
    modal: true
    dim: true
    padding: 16
    width: 240

    signal importRequested(string format)

    background: Rectangle {
        color: "#162744"
        border.color: "#2d5a9e"
        border.width: 1
        radius: 6
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        Label {
            text: "Import Format"
            font.bold: true
        }

        ButtonGroup {
            id: formatGroup
        }

        RadioButton {
            id: jsonRadio
            text: "JSON"
            checked: true
            ButtonGroup.group: formatGroup
        }

        RadioButton {
            id: textRadio
            text: "Text"
            ButtonGroup.group: formatGroup
        }

        RowLayout {
            Layout.topMargin: 8
            Layout.fillWidth: true

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#1e3a5f"
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Item { Layout.fillWidth: true }

            Button {
                text: "Cancel"
                flat: true
                onClicked: importPopup.close()
            }

            Button {
                text: "Import"
                onClicked: {
                    var format = jsonRadio.checked ? "json" : "text";
                    importPopup.close();
                    importPopup.importRequested(format);
                }
            }
        }
    }
}
