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
    property var activeExporter: null

    function startExport(wordCount) {
        current = 0;
        total = wordCount;
        exporting = true;
        notificationLabel.opacity = 0;
        notificationLabel.text = "";
        cancelButton.enabled = true;
    }

    function setActiveExporter(e) {
        activeExporter = e;
    }

    function updateProgress(cur, tot) {
        current = cur;
        total = tot;
    }

    function showNotification(text, isError) {
        fadeOutNotification.stop();
        holdTimer.stop();
        notificationLabel.text = text;
        notificationLabel.color = isError ? "#e74c3c" : "#27ae60";
        fadeInNotification.start();
    }

    function showResult(success) {
        exporting = false;
        activeExporter = null;
        showNotification(
            success ? "PDF exported successfully" : "Export failed",
            !success
        );
    }

    function showCancelled() {
        exporting = false;
        activeExporter = null;
        showNotification("Export cancelled", false);
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

        Button {
            id: cancelButton
            implicitWidth: height
            implicitHeight: 24
            padding: 0
            hoverEnabled: true
            ToolTip.visible: hovered
            ToolTip.delay: 500
            ToolTip.text: "Cancel export"

            background: Rectangle {
                radius: width / 2
                color: cancelButton.enabled
                       ? (cancelButton.down
                          ? Qt.darker("#e74c3c", 1.2)
                          : (cancelButton.hovered
                             ? Qt.lighter("#e74c3c", 1.15)
                             : "#e74c3c"))
                       : Qt.darker("#e74c3c", 1.6)
                border.color: Qt.darker("#e74c3c", 1.4)
                border.width: 1
            }

            contentItem: Item {}

            onClicked: {
                cancelButton.enabled = false;
                if (overlay.activeExporter)
                    overlay.activeExporter.requestCancel();
            }
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
        interval: 3000
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
