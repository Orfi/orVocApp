import QtQuick
import QtQuick.Controls

Window {
    id: splash
    width: 300
    height: 200
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
