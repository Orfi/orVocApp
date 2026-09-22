// orVocApp/networkclient.cpp
#include "networkclient.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>

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

    // Phonetic — from hwi.prs[].ipa (Merriam-Webster)
    QJsonObject hwi = entry.value("hwi").toObject();
    QJsonArray prs = hwi.value("prs").toArray();
    for (const QJsonValue &p : prs) {
        QString ipa = p.toObject().value("ipa").toString();
        if (!ipa.isEmpty()) {
            result.phonetic = ipa;
            break;
        }
    }

    // Audio URL — MW current scheme: media.merriam-webster.com/audio/prs/en/us/mp3/{subdir}/{audio}.mp3
    // field source: MW nests audio under prs[].sound.audio (hwi.sound is empty)
    QString audio;
    for (const QJsonValue &pp : hwi.value("prs").toArray()) {
        audio = pp.toObject().value("sound").toObject().value("audio").toString();
        if (!audio.isEmpty()) break;
    }
    if (!audio.isEmpty()) {
        QString subdir;
        if (audio.startsWith("bix")) subdir = "bix";
        else if (audio.startsWith("gg")) subdir = "gg";
        else if (audio.left(1) >= "0" && audio.left(1) <= "9") subdir = "number";
        else subdir = audio.left(1);
        result.audioUrl = QUrl("https://media.merriam-webster.com/soundc11/" + subdir + "/" + audio + ".wav");
    }

    // Build HTML from MW shortdef array + part of speech
    QString html;
    QString fl = entry.value("fl").toString();
    if (!fl.isEmpty())
        html += "<p><b>" + fl + "</b></p>";

    QJsonArray shortdef = entry.value("shortdef").toArray();
    for (const QJsonValue &d : shortdef) {
        QString def = d.toString();
        if (!def.isEmpty())
            html += "<p>" + def + "</p>";
    }

    if (html.isEmpty())
        result.error = true;

    result.html = html;
    return result;
}

void NetworkClient::fetchDefinition(const QString &word)
{
    QUrl url("https://www.dictionaryapi.com/api/v3/references/collegiate/json/" + word + "?key=6db9cb08-34fb-4a9e-abd8-a08586b2113a");
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

void NetworkClient::fetchTranslation(const QString &word)
{
    QUrl url("https://translate.googleapis.com/translate_a/single");
    QUrlQuery query;
    query.addQueryItem("client", "gtx");
    query.addQueryItem("sl", "en");
    query.addQueryItem("tl", "ar");
    query.addQueryItem("dt", "t");
    query.addQueryItem("q", word);
    url.setQuery(query);

    QNetworkReply *reply = m_nam->get(QNetworkRequest(url));

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit requestFailed("translation", "Could not fetch translation.");
            return;
        }

        auto result = parseTranslationResponse(reply->readAll());
        if (result.error) {
            emit requestFailed("translation", "Could not fetch translation.");
            return;
        }

        emit translationReady(result.html);
    });
}

TranslationResult NetworkClient::parseTranslationResponse(const QByteArray &data)
{
    TranslationResult result;

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        result.error = true;
        return result;
    }

    // GTX response structure: [[["translated","original",...],...],...,"source_lang"]
    QJsonArray root = doc.array();
    if (root.isEmpty() || !root[0].isArray()) {
        result.error = true;
        return result;
    }

    QJsonArray translations = root[0].toArray();
    if (translations.isEmpty() || !translations[0].isArray()) {
        result.error = true;
        return result;
    }

    QString translated = translations[0].toArray()[0].toString();
    if (translated.isEmpty()) {
        result.error = true;
        return result;
    }

    result.html = "<p dir=\"rtl\" style=\"font-size: 20px;\">" + translated + "</p>";
    return result;
}
