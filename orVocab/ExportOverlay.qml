import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: overlay
    anchors.right: parent.right
    anchors.bottom: parent.bottom
    anchors.margins: 16
    width: 280
    height: progressColumn.implicitHeight + 16

    property int current: 0
    property int total: 0
    property bool exporting: false

    signal exportComplete(bool success)

    function startExport(wordCount) {
        current = 0;
        total = wordCount;
        exporting = true;
        progressColumn.opacity = 1;
        notificationLabel.opacity = 0;
        notificationLabel.text = "";
    }

    function updateProgress(cur, tot) {
        current = cur;
        total = tot;
    }

    function showResult(success) {
        exporting = false;
        fadeOutProgress.start();
        if (success) {
            notificationLabel.text = "PDF exported successfully";
            notificationLabel.color = "#27ae60";
        } else {
            notificationLabel.text = "Export failed";
            notificationLabel.color = "#e74c3c";
        }
    }

    Column {
        id: progressColumn
        anchors.fill: parent
        anchors.margins: 8
        spacing: 4
        visible: opacity > 0

        Behavior on opacity {
            enabled: false
        }

        Label {
            text: "Exporting... " + overlay.current + "/" + overlay.total + " words"
            font.pixelSize: 12
            visible: overlay.exporting
        }

        ProgressBar {
            width: parent.width
            from: 0
            to: overlay.total
            value: overlay.current
            visible: overlay.exporting
        }
    }

    NumberAnimation {
        id: fadeOutProgress
        target: progressColumn
        property: "opacity"
        from: 1
        to: 0
        duration: 300
        onFinished: fadeInNotification.start()
    }

    Label {
        id: notificationLabel
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 8
        font.pixelSize: 13
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
