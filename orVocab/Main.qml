// orVocab/Main.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    id: root
    width: 900
    height: 600
    visible: true
    title: "orVocab"

    property string currentWord: ""

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8

            Label {
                text: "orVocab"
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
        }
    }

    SplitView {
        anchors.fill: parent

        Sidebar {
            SplitView.preferredWidth: 220
            SplitView.minimumWidth: 150

            onWordDoubleClicked: function(word) {
                root.currentWord = word;
                translationView.lookupWord(word);
            }
        }

        TranslationView {
            id: translationView
            SplitView.fillWidth: true
            currentWord: root.currentWord
        }
    }

    // File dialogs
    FileDialog {
        id: importJsonDialog
        title: "Import JSON Word List"
        nameFilters: ["JSON files (*.json)"]
        onAccepted: confirmImportDialog.open()
    }

    FileDialog {
        id: importTextDialog
        title: "Import Text Word List"
        nameFilters: ["Text files (*.txt)"]
        onAccepted: VocabManager.importText(importTextDialog.selectedFile)
    }

    FileDialog {
        id: exportJsonDialog
        title: "Export JSON Word List"
        fileMode: FileDialog.SaveFile
        nameFilters: ["JSON files (*.json)"]
        onAccepted: VocabManager.exportJson(exportJsonDialog.selectedFile)
    }

    FileDialog {
        id: exportTextDialog
        title: "Export Text Word List"
        fileMode: FileDialog.SaveFile
        nameFilters: ["Text files (*.txt)"]
        onAccepted: VocabManager.exportText(exportTextDialog.selectedFile)
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

        onAccepted: VocabManager.importJson(importJsonDialog.selectedFile)
    }
}
