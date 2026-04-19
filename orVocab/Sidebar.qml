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
    signal wordDoubleClicked(string word)
    signal wordDeleted(string word)

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
                onTextChanged: VocabManager.filterWords(text)
            }

            Button {
                text: "Add"
                onClicked: {
                    if (searchField.text.trim() === "")
                        return;
                    var word = searchField.text.trim().toLowerCase();
                    VocabManager.addWord(searchField.text);
                    searchField.text = "";
                    var idx = VocabManager.filteredWords.indexOf(word);
                    if (idx >= 0) {
                        wordList.currentIndex = idx;
                        sidebar.wordDoubleClicked(word);
                    }
                }
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
            model: VocabManager.filteredWords
            clip: true
            currentIndex: -1

            delegate: ItemDelegate {
                width: wordList.width
                text: modelData
                highlighted: wordList.currentIndex === index

                onDoubleClicked: {
                    wordList.currentIndex = index;
                    sidebar.wordDoubleClicked(modelData);
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.RightButton
                    onClicked: function(mouse) {
                        wordList.currentIndex = index;
                        contextMenu.selectedWord = modelData;
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
