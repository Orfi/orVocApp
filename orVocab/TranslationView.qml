// orVocab/TranslationView.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

Rectangle {
    id: translationView
    color: "transparent"

    property string currentWord: ""
    property string phonetic: ""
    property string definitionHtml: ""
    property string translationHtml: ""
    property url audioSource: ""
    property string definitionError: ""
    property string translationError: ""
    property bool wantsToPlay: false

    function lookupWord(word) {
        currentWord = word;
        phonetic = "";
        definitionHtml = "";
        translationHtml = "";
        audioSource = "";
        definitionError = "";
        translationError = "";
        wantsToPlay = false;
        mediaPlayer.stop();
        mediaPlayer.source = "";
        audioErrorLabel.visible = false;
        NetworkClient.fetchDefinition(word);
        NetworkClient.fetchTranslation(word);
    }

    function clearView() {
        currentWord = "";
        phonetic = "";
        definitionHtml = "";
        translationHtml = "";
        audioSource = "";
        definitionError = "";
        translationError = "";
        wantsToPlay = false;
        mediaPlayer.stop();
        mediaPlayer.source = "";
        audioErrorLabel.visible = false;
    }

    Connections {
        target: NetworkClient

        function onDefinitionReady(html) {
            translationView.definitionHtml = html;
            translationView.definitionError = "";
        }

        function onPhoneticReady(phon) {
            translationView.phonetic = phon;
        }

        function onAudioUrlReady(url) {
            translationView.audioSource = url;
        }

        function onTranslationReady(html) {
            translationView.translationHtml = html;
            translationView.translationError = "";
        }

        function onRequestFailed(area, errorString) {
            if (area === "definition") {
                translationView.definitionError = errorString;
                translationView.definitionHtml = "";
            } else if (area === "translation") {
                translationView.translationError = errorString;
                translationView.translationHtml = "";
            }
        }
    }

    MediaPlayer {
        id: mediaPlayer
        audioOutput: AudioOutput {}
        onMediaStatusChanged: {
            if (mediaStatus === MediaPlayer.LoadedMedia && translationView.wantsToPlay) {
                translationView.wantsToPlay = false;
                mediaPlayer.play();
            }
            if (mediaStatus === MediaPlayer.InvalidMedia) {
                translationView.wantsToPlay = false;
                audioErrorLabel.text = "Audio not available for this word.";
                audioErrorLabel.visible = true;
            }
        }
        onErrorOccurred: function(error, errorString) {
            translationView.wantsToPlay = false;
            audioErrorLabel.text = "Audio error: " + errorString;
            audioErrorLabel.visible = true;
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        // Word header
        RowLayout {
            Layout.fillWidth: true

            ColumnLayout {
                spacing: 2
                Label {
                    text: currentWord
                    font.pixelSize: 28
                    font.bold: true
                    visible: currentWord !== ""
                }
                Label {
                    text: phonetic
                    font.pixelSize: 14
                    opacity: 0.6
                    visible: phonetic !== ""
                }
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "\u25B6  Play Pronunciation"
                enabled: audioSource.toString() !== ""
                onClicked: {
                    audioErrorLabel.visible = false;
                    translationView.wantsToPlay = true;
                    if (mediaPlayer.source == audioSource) {
                        mediaPlayer.stop();
                        mediaPlayer.play();
                        translationView.wantsToPlay = false;
                    } else {
                        mediaPlayer.source = audioSource;
                    }
                }
            }

            Label {
                id: audioErrorLabel
                visible: false
                color: "#e74c3c"
                font.pixelSize: 12
            }
        }

        // English Definition
        GroupBox {
            Layout.fillWidth: true
            Layout.fillHeight: true
            title: "English Definition"

            ScrollView {
                anchors.fill: parent
                clip: true

                Text {
                    width: parent.width
                    textFormat: Text.RichText
                    wrapMode: Text.Wrap
                    text: definitionError !== "" ? "<p style='color: #e74c3c;'>" + definitionError + "</p>" : definitionHtml
                    color: palette.text
                    visible: definitionHtml !== "" || definitionError !== ""
                }

                Label {
                    anchors.centerIn: parent
                    text: "Select a word to see its definition"
                    opacity: 0.5
                    visible: definitionHtml === "" && definitionError === "" && currentWord === ""
                }

                BusyIndicator {
                    anchors.centerIn: parent
                    running: currentWord !== "" && definitionHtml === "" && definitionError === ""
                }
            }
        }

        // Arabic Translation
        GroupBox {
            Layout.fillWidth: true
            Layout.preferredHeight: 120
            title: "Arabic Translation"

            ScrollView {
                anchors.fill: parent
                clip: true

                Text {
                    width: parent.width
                    textFormat: Text.RichText
                    wrapMode: Text.Wrap
                    horizontalAlignment: Text.AlignRight
                    text: translationError !== "" ? "<p style='color: #e74c3c;'>" + translationError + "</p>" : translationHtml
                    color: palette.text
                    visible: translationHtml !== "" || translationError !== ""
                }

                Label {
                    anchors.centerIn: parent
                    text: "Translation will appear here"
                    opacity: 0.5
                    visible: translationHtml === "" && translationError === "" && currentWord === ""
                }

                BusyIndicator {
                    anchors.centerIn: parent
                    running: currentWord !== "" && translationHtml === "" && translationError === ""
                }
            }
        }
    }
}
