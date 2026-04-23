import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: overlay
    Layout.preferredWidth: visible ? 220 : 0
    Layout.fillHeight: true
    visible: exporting || notificationLabel.opacity > 0

    property int current: 0
    property int total: 0
    property bool exporting: false

    function startExport(wordCount) {
        current = 0;
        total = wordCount;
        exporting = true;
        notificationLabel.opacity = 0;
        notificationLabel.text = "";
    }

    function updateProgress(cur, tot) {
        current = cur;
        total = tot;
    }

    function showResult(success) {
        exporting = false;
        if (success) {
            notificationLabel.text = "PDF exported successfully";
            notificationLabel.color = "#27ae60";
        } else {
            notificationLabel.text = "Export failed";
            notificationLabel.color = "#e74c3c";
        }
        fadeInNotification.start();
    }

    RowLayout {
        anchors.fill: parent
        spacing: 6
        visible: overlay.exporting

        Label {
            text: overlay.current + "/" + overlay.total
            font.pixelSize: 11
        }

        ProgressBar {
            Layout.fillWidth: true
            Layout.preferredHeight: 14
            from: 0
            to: overlay.total
            value: overlay.current
        }
    }

    Label {
        id: notificationLabel
        anchors.centerIn: parent
        font.pixelSize: 12
        font.bold: true
        opacity: 0
        visible: opacity > 0

        Behavior on opacity {
            enabled: false
        }
    }

    NumberAnimation {
        id: fadeInNotification
        target: notificationLabel
        property: "opacity"
        from: 0
        to: 1
        duration: 300
        onFinished: holdTimer.start()
    }

    Timer {
        id: holdTimer
        interval: 4000
        onTriggered: fadeOutNotification.start()
    }

    NumberAnimation {
        id: fadeOutNotification
        target: notificationLabel
        property: "opacity"
        from: 1
        to: 0
        duration: 300
    }
}
