// orVocab/main.cpp
#include <QApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQuickWindow>
#include <QTimer>
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

    QQmlApplicationEngine engine;

    qmlRegisterSingletonInstance("orVocab", 1, 0, "VocabManager", &vocabManager);
    qmlRegisterSingletonInstance("orVocab", 1, 0, "NetworkClient", &networkClient);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    QQmlComponent splashComponent(&engine, QUrl("qrc:/qt/qml/orVocab/SplashScreen.qml"));
    QObject *splashObj = splashComponent.create();

    QTimer::singleShot(3000, &app, [&engine, splashObj]() {
        if (splashObj) {
            auto *splashWindow = qobject_cast<QQuickWindow *>(splashObj);
            if (splashWindow)
                splashWindow->close();
            splashObj->deleteLater();
        }
        engine.loadFromModule("orVocab", "Main");
    });

    return app.exec();
}
