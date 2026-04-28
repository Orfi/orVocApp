// orVocApp/main.cpp
#include <QApplication>
#include <QElapsedTimer>
#include <QIcon>
#include <QPixmap>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QSplashScreen>
#include "vocabmanager.h"
#include "networkclient.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QQuickStyle::setStyle("Fusion");
    app.setOrganizationName("orVocApp");
    app.setApplicationName("orVocApp");
    QIcon appIcon;
    for (const auto &res : {":/qt/qml/orvocapp/icon_16.png",
                            ":/qt/qml/orvocapp/icon_32.png",
                            ":/qt/qml/orvocapp/icon_48.png",
                            ":/qt/qml/orvocapp/icon_64.png",
                            ":/qt/qml/orvocapp/icon_128.png",
                            ":/qt/qml/orvocapp/icon_256.png"})
        appIcon.addFile(res);
    app.setWindowIcon(appIcon);
    app.setDesktopFileName("orVocApp");

    VocabManager vocabManager;
    vocabManager.loadFromJson();

    NetworkClient networkClient;

    QPixmap splashPix(":/qt/qml/orvocapp/splash.png");
    splashPix = splashPix.scaled(splashPix.size() * 0.2, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QSplashScreen splash(splashPix);
    splash.setWindowOpacity(0.6);
    splash.show();

    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 3000) {
        app.processEvents();
    }

    QQmlApplicationEngine engine;

    qmlRegisterSingletonInstance("orvocapp", 1, 0, "VocabManager", &vocabManager);
    qmlRegisterSingletonInstance("orvocapp", 1, 0, "NetworkClient", &networkClient);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("orvocapp", "Main");
    splash.close();

    return app.exec();
}
