// orVocab/main.cpp
#include <QApplication>
#include <QElapsedTimer>
#include <QIcon>
#include <QPixmap>
#include <QQmlApplicationEngine>
#include <QSplashScreen>
#include "vocabmanager.h"
#include "networkclient.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("orVocab");
    app.setApplicationName("orVocApp");
    QIcon appIcon;
    for (const auto &res : {":/qt/qml/orVocab/icon_16.png",
                            ":/qt/qml/orVocab/icon_32.png",
                            ":/qt/qml/orVocab/icon_48.png",
                            ":/qt/qml/orVocab/icon_64.png",
                            ":/qt/qml/orVocab/icon_128.png",
                            ":/qt/qml/orVocab/icon_256.png"})
        appIcon.addFile(res);
    app.setWindowIcon(appIcon);
    app.setDesktopFileName("orVocab");

    VocabManager vocabManager;
    vocabManager.loadFromJson();

    NetworkClient networkClient;

    QPixmap splashPix(":/qt/qml/orVocab/splash.png");
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

    qmlRegisterSingletonInstance("orVocab", 1, 0, "VocabManager", &vocabManager);
    qmlRegisterSingletonInstance("orVocab", 1, 0, "NetworkClient", &networkClient);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("orVocab", "Main");
    splash.close();

    return app.exec();
}
