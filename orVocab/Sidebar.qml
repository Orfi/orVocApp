// orVocab/Sidebar.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: sidebar
    color: palette.base
    border.color: palette.mid
    border.width: 0

    signal wordSelected(string word)
    signal wordClicked(string word)
    signal wordDeleted(string word)

    signal validationFailed()

    property bool validating: false
    property string pendingWord: ""

    function selectAndScrollTo(word) {
        var idx = VocabManager.indexOfWord(word);
        if (idx >= 0) {
            wordList.currentIndex = idx;
            wordList.positionViewAtIndex(idx, ListView.Contain);
            sidebar.wordClicked(word);
        }
    }

    function attemptAdd() {
        var word = searchField.text.trim().toLowerCase();
        if (word === "")
            return;

        if (VocabManager.indexOfWord(word) >= 0) {
            sidebar.selectAndScrollTo(word);
            searchField.text = "";
            return;
        }

        sidebar.pendingWord = word;
        sidebar.validating = true;
        validationTimeout.restart();
        NetworkClient.fetchDefinition(word);
    }

    Connections {
        target: NetworkClient
        enabled: sidebar.validating

        function onDefinitionReady(html) {
            validationTimeout.stop();
            VocabManager.addWord(sidebar.pendingWord);
            searchField.text = "";
            sidebar.selectAndScrollTo(sidebar.pendingWord);
            sidebar.validating = false;
            sidebar.pendingWord = "";
        }

        function onRequestFailed(area, errorString) {
            if (area !== "definition")
                return;
            validationTimeout.stop();
            sidebar.validationFailed();
            sidebar.validating = false;
            sidebar.pendingWord = "";
        }
    }

    Timer {
        id: validationTimeout
        interval: 5000
        repeat: false
        onTriggered: {
            if (!sidebar.validating)
                return;
            sidebar.validationFailed();
            sidebar.validating = false;
            sidebar.pendingWord = "";
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 8
            spacing: 4

            TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: "Search or add word..."
                enabled: !sidebar.validating
                onTextChanged: VocabManager.filterWords(text)
                onAccepted: sidebar.attemptAdd()
            }

            Button {
                id: addButton
                text: "Add"
                enabled: !sidebar.validating && searchField.text.trim() !== ""
                onClicked: sidebar.attemptAdd()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: palette.mid
        }

        ListView {
            id: wordList
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: VocabManager.wordModel
            clip: true
            currentIndex: -1
            enabled: !sidebar.validating
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            delegate: ItemDelegate {
                width: wordList.width
                text: model.display
                highlighted: wordList.currentIndex === index

                onClicked: {
                    wordList.currentIndex = index;
                    sidebar.wordClicked(model.display);
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.RightButton
                    onClicked: function(mouse) {
                        wordList.currentIndex = index;
                        contextMenu.selectedWord = model.display;
                        contextMenu.popup();
                    }
                }
            }

            Menu {
                id: contextMenu
                property string selectedWord: ""

                MenuItem {
                    text: "Delete"
                    onTriggered: {
                        var word = contextMenu.selectedWord;
                        VocabManager.removeWord(word);
                        sidebar.wordDeleted(word);
                    }
                }
            }
        }
    }
}
