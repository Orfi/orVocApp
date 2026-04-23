// orVocab/Main.qml
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

            Label {
                text: "orVocApp"
                font.bold: true
                Layout.fillWidth: true
            }

            Button {
                text: "Import JSON"
                onClicked: importJsonDialog.open()
            }
            Button {
                text: "Import Text"
                onClicked: importTextDialog.open()
            }
            Button {
                text: "Export JSON"
                onClicked: exportJsonDialog.open()
            }
            Button {
                text: "Export Text"
                onClicked: exportTextDialog.open()
            }
            ExportOverlay {
                id: exportOverlay
            }

            Button {
                text: "Export PDF"
                enabled: !exportOverlay.exporting
                opacity: enabled ? 1.0 : 0.4
                onClicked: pageSizeDialog.open()
            }
        }
    }

    SplitView {
        anchors.fill: parent

        Sidebar {
            SplitView.preferredWidth: 220
            SplitView.minimumWidth: 150

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

    Dialog {
        id: pageSizeDialog
        title: "Select Page Size"
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Cancel

        RowLayout {
            spacing: 12

            Button {
                text: "A4"
                onClicked: {
                    pageSizeDialog.close();
                    root.selectedPageSize = 0;
                    exportPdfDialog.open();
                }
            }

            Button {
                text: "Letter"
                onClicked: {
                    pageSizeDialog.close();
                    root.selectedPageSize = 1;
                    exportPdfDialog.open();
                }
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
            exportOverlay.startExport(VocabManager.words.length);
            exporter.progress.connect(exportOverlay.updateProgress);
            exporter.finished.connect(function(success, path) {
                exportOverlay.showResult(success);
            });
            exporter.exportToFile(exportPdfDialog.file.toString().replace("file://", ""));
        }
    }
}
