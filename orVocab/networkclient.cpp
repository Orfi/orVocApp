// orVocab/networkclient.cpp
#include "networkclient.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

NetworkClient::NetworkClient(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

DictionaryResult NetworkClient::parseDictionaryResponse(const QByteArray &data)
{
    DictionaryResult result;

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray() || doc.array().isEmpty()) {
        result.error = true;
        return result;
    }

    QJsonObject entry = doc.array().first().toObject();

    // Extract phonetic
    result.phonetic = entry.value("phonetic").toString();

    // Extract audio URL — find first phonetics entry with a non-empty .mp3 audio
    QJsonArray phonetics = entry.value("phonetics").toArray();
    for (const QJsonValue &p : phonetics) {
        QString audio = p.toObject().value("audio").toString();
        if (!audio.isEmpty() && audio.endsWith(".mp3")) {
            result.audioUrl = QUrl(audio);
            break;
        }
    }

    // If phonetic was empty, try from phonetics array
    if (result.phonetic.isEmpty()) {
        for (const QJsonValue &p : phonetics) {
            QString text = p.toObject().value("text").toString();
            if (!text.isEmpty()) {
                result.phonetic = text;
                break;
            }
        }
    }

    // Build HTML from meanings
    QString html;
    QJsonArray meanings = entry.value("meanings").toArray();
    for (const QJsonValue &m : meanings) {
        QJsonObject meaning = m.toObject();
        QString pos = meaning.value("partOfSpeech").toString();
        html += "<p><b>" + pos + "</b></p>";

        QJsonArray definitions = meaning.value("definitions").toArray();
        for (const QJsonValue &d : definitions) {
            QJsonObject def = d.toObject();
            html += "<p>" + def.value("definition").toString() + "</p>";
            QString example = def.value("example").toString();
            if (!example.isEmpty())
                html += "<p><i>\"" + example + "\"</i></p>";
        }
    }

    if (html.isEmpty())
        result.error = true;

    result.html = html;
    return result;
}

void NetworkClient::fetchDefinition(const QString &word)
{
    QUrl url("https://api.dictionaryapi.dev/api/v2/entries/en/" + word);
    QNetworkReply *reply = m_nam->get(QNetworkRequest(url));

    connect(reply, &QNetworkReply::finished, this, [this, reply, word]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::ContentNotFoundError)
                emit requestFailed("definition", QString("No definition found for '%1'.").arg(word));
            else
                emit requestFailed("definition", "Could not fetch definition. Check your connection.");
            return;
        }

        auto result = parseDictionaryResponse(reply->readAll());
        if (result.error) {
            emit requestFailed("definition", QString("No definition found for '%1'.").arg(word));
            return;
        }

        emit definitionReady(result.html);
        if (!result.phonetic.isEmpty())
            emit phoneticReady(result.phonetic);
        if (!result.audioUrl.isEmpty())
            emit audioUrlReady(result.audioUrl);
    });
}

void NetworkClient::fetchTranslation(const QString &) {}

TranslationResult NetworkClient::parseTranslationResponse(const QByteArray &)
{
    return TranslationResult{};
}
