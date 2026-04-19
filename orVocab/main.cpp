// orVocab/main.cpp
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "vocabmanager.h"
#include "networkclient.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName("orVocab");
    app.setApplicationName("orVocab");

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
    engine.loadFromModule("orVocab", "Main");

    return app.exec();
}
