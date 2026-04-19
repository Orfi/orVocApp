// orVocab/networkclient.h
#ifndef NETWORKCLIENT_H
#define NETWORKCLIENT_H

#include <QObject>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

class QNetworkAccessManager;

struct DictionaryResult {
    QString html;
    QString phonetic;
    QUrl audioUrl;
    bool error = false;
};

struct TranslationResult {
    QString html;
    bool error = false;
};

class NetworkClient : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit NetworkClient(QObject *parent = nullptr);

    Q_INVOKABLE void fetchDefinition(const QString &word);
    Q_INVOKABLE void fetchTranslation(const QString &word);

    static DictionaryResult parseDictionaryResponse(const QByteArray &data);
    static TranslationResult parseTranslationResponse(const QByteArray &data);

signals:
    void definitionReady(const QString &html);
    void phoneticReady(const QString &phonetic);
    void audioUrlReady(const QUrl &url);
    void translationReady(const QString &html);
    void requestFailed(const QString &area, const QString &errorString);

private:
    QNetworkAccessManager *m_nam;
};

#endif // NETWORKCLIENT_H
