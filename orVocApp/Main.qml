// orVocApp/Main.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform as Platform

ApplicationWindow {
    id: root
    width: 900
    height: 600
    visible: true
    title: "orVocApp"

    property string currentWord: ""
    property int selectedPageSize: 0

    palette.window: "#0a1628"
    palette.base: "#101e36"
    palette.alternateBase: "#162744"
    palette.text: "#e8e4dd"
    palette.windowText: "#e8e4dd"
    palette.button: "#1a3055"
    palette.buttonText: "#e8e4dd"
    palette.highlight: "#2d5a9e"
    palette.highlightedText: "#ffffff"
    palette.mid: "#1e3a5f"
    palette.placeholderText: "#7a8ba0"

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            anchors.topMargin: 4
            anchors.bottomMargin: 4

            Label {
                text: "orVocApp"
                font.bold: true
            }

            Label {
                text: VocabManager.words.length + " words"
                color: "#27ae60"
                font.pixelSize: 12
                Layout.fillWidth: true
            }

            NotificationOverlay {
                id: notificationOverlay
                buttonSize: pronunciationButton.implicitHeight
                indicatorSize: pronunciationButton.indicatorSize
            }

            Button {
                id: pronunciationButton
                readonly property int indicatorSize: glyph.font.pixelSize
                implicitWidth: exportButton.implicitHeight
                implicitHeight: exportButton.implicitHeight
                padding: 0
                enabled: translationView.hasAudio
                opacity: enabled ? 1.0 : 0.4
                hoverEnabled: true
                ToolTip.visible: hovered
                ToolTip.delay: 500
                ToolTip.text: "Pronunciation"

                background: Rectangle {
                    radius: width / 2
                    color: pronunciationButton.down
                           ? Qt.darker(palette.button, 1.2)
                           : (pronunciationButton.hovered
                              ? Qt.lighter(palette.button, 1.15)
                              : palette.button)
                    border.color: palette.mid
                    border.width: 1
                }

                contentItem: Label {
                    id: glyph
                    text: "▶"
                    color: palette.buttonText
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 14
                    leftPadding: 2
                }

                onClicked: translationView.playPronunciation()
            }

            Button {
                id: exportButton
                text: "Export"
                onClicked: exportPopup.open()
            }

            Button {
                id: importButton
                text: "Import"
                enabled: !notificationOverlay.exporting
                opacity: enabled ? 1.0 : 0.4
                onClicked: importPopup.open()
            }
        }
    }

    SplitView {
        anchors.fill: parent

        Sidebar {
            id: sidebar
            SplitView.preferredWidth: 220
            SplitView.minimumWidth: 150
            enabled: !notificationOverlay.exporting
            opacity: notificationOverlay.exporting ? 0.4 : 1.0

            onValidationFailed: notificationOverlay.showNotification(
                "Could not validate input",
                true
            )

            onWordClicked: function(word) {
                root.currentWord = word;
                translationView.lookupWord(word);
            }

            onWordDeleted: function(word) {
                if (root.currentWord === word) {
                    root.currentWord = "";
                    translationView.clearView();
                }
            }
        }

        TranslationView {
            id: translationView
            SplitView.fillWidth: true
            currentWord: root.currentWord
            suppressUpdates: sidebar.validating

            onAudioError: function(message) {
                notificationOverlay.showNotification(message, true);
            }
        }
    }

    // File dialogs (Qt.labs.platform avoids portal freeze on Linux)
    Platform.FileDialog {
        id: importJsonDialog
        title: "Import JSON Word List"
        fileMode: Platform.FileDialog.OpenFile
        nameFilters: ["JSON files (*.json)"]
        onAccepted: confirmImportDialog.open()
    }

    Platform.FileDialog {
        id: importTextDialog
        title: "Import Text Word List"
        fileMode: Platform.FileDialog.OpenFile
        nameFilters: ["Text files (*.txt)"]
        onAccepted: VocabManager.importText(importTextDialog.file)
    }

    Platform.FileDialog {
        id: exportJsonDialog
        title: "Export JSON Word List"
        fileMode: Platform.FileDialog.SaveFile
        defaultSuffix: "json"
        nameFilters: ["JSON files (*.json)"]
        onAccepted: VocabManager.exportJson(exportJsonDialog.file)
    }

    Platform.FileDialog {
        id: exportTextDialog
        title: "Export Text Word List"
        fileMode: Platform.FileDialog.SaveFile
        defaultSuffix: "txt"
        nameFilters: ["Text files (*.txt)"]
        onAccepted: VocabManager.exportText(exportTextDialog.file)
    }

    Dialog {
        id: confirmImportDialog
        title: "Confirm Import"
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Ok | Dialog.Cancel

        Label {
            text: "This will replace your current word list of "
                  + VocabManager.words.length + " words. Continue?"
        }

        onAccepted: VocabManager.importJson(importJsonDialog.file)
    }

    ExportPopup {
        id: exportPopup
        parent: exportButton
        x: exportButton.width - width
        y: exportButton.height
        exporting: notificationOverlay.exporting

        onExportRequested: function(format, pageSize) {
            if (format === "json") {
                exportJsonDialog.open();
            } else if (format === "text") {
                exportTextDialog.open();
            } else if (format === "pdf") {
                root.selectedPageSize = pageSize;
                exportPdfDialog.open();
            }
        }
    }

    ImportPopup {
        id: importPopup
        parent: importButton
        x: importButton.width - width
        y: importButton.height

        onImportRequested: function(format) {
            if (format === "json") {
                importJsonDialog.open();
            } else if (format === "text") {
                importTextDialog.open();
            }
        }
    }

    Platform.FileDialog {
        id: exportPdfDialog
        title: "Export PDF Dictionary"
        fileMode: Platform.FileDialog.SaveFile
        defaultSuffix: "pdf"
        nameFilters: ["PDF files (*.pdf)"]
        onAccepted: {
            var exporter = VocabManager.createPdfExporter(root.selectedPageSize);
            notificationOverlay.startExport(VocabManager.words.length);
            notificationOverlay.setActiveExporter(exporter);
            exporter.progress.connect(notificationOverlay.updateProgress);
            exporter.finished.connect(function(success, path) {
                notificationOverlay.showResult(success);
            });
            exporter.cancelled.connect(function() {
                notificationOverlay.showCancelled();
            });
            exporter.exportToFile(exportPdfDialog.file.toString().replace("file://", ""));
        }
    }
}
