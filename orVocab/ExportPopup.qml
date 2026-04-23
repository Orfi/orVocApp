import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: exportPopup
    modal: true
    dim: true
    padding: 16
    width: 240

    property bool exporting: false

    signal exportRequested(string format, int pageSize)

    onExportingChanged: {
        if (exporting && formatGroup.checkedButton === pdfRadio) {
            jsonRadio.checked = true;
        }
    }

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
            text: "Export Format"
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

        RadioButton {
            id: pdfRadio
            text: "PDF"
            enabled: !exportPopup.exporting
            opacity: enabled ? 1.0 : 0.4
            ButtonGroup.group: formatGroup
        }

        ColumnLayout {
            visible: pdfRadio.checked
            Layout.leftMargin: 28
            spacing: 4

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: pageSizeColumn.implicitHeight + 16
                color: "#101e36"
                radius: 4
                border.width: 0

                Rectangle {
                    width: 2
                    height: parent.height
                    color: "#2d5a9e"
                    anchors.left: parent.left
                }

                ColumnLayout {
                    id: pageSizeColumn
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.topMargin: 8
                    anchors.bottomMargin: 8
                    spacing: 4

                    Label {
                        text: "PAGE SIZE"
                        font.pixelSize: 11
                        color: "#7a8ba0"
                    }

                    ButtonGroup {
                        id: pageSizeGroup
                    }

                    RadioButton {
                        id: a4Radio
                        text: "A4"
                        checked: true
                        ButtonGroup.group: pageSizeGroup
                    }

                    RadioButton {
                        id: letterRadio
                        text: "Letter"
                        ButtonGroup.group: pageSizeGroup
                    }
                }
            }
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
                onClicked: exportPopup.close()
            }

            Button {
                text: "Export"
                onClicked: {
                    var format = "json";
                    if (textRadio.checked) format = "text";
                    else if (pdfRadio.checked) format = "pdf";

                    var pageSize = a4Radio.checked ? 0 : 1;
                    exportPopup.close();
                    exportPopup.exportRequested(format, pageSize);
                }
            }
        }
    }
}
