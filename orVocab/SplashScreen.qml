import QtQuick
import QtQuick.Controls

Window {
    id: splash
    width: 600
    height: 400
    visible: true
    flags: Qt.SplashScreen | Qt.WindowStaysOnTopHint
    color: "#2196F3"
    opacity: 0.7

    Image {
        anchors.fill: parent
        source: "splash.png"
        fillMode: Image.PreserveAspectFit
    }
}
