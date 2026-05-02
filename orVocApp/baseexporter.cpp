#include "baseexporter.h"

#include "networkclient.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QThread>
#include <QTimer>
#include <QUrlQuery>

BaseExporter::BaseExporter(const QStringList &words, QObject *parent)
    : QObject(parent)
    , m_words(words)
{
}

BaseExporter::~BaseExporter()
{
    delete m_nam;
}

void BaseExporter::exportToFile(const QString &outputPath)
{
    m_outputPath = outputPath;
    m_nam = new QNetworkAccessManager(this);
    m_entries.resize(m_words.size());
    for (int i = 0; i < m_words.size(); ++i)
        m_entries[i].word = m_words[i];
    m_currentIndex = 0;
    fetchNextWord();
}

void BaseExporter::requestCancel()
{
    m_cancelled.store(true, std::memory_order_release);
    if (m_currentReply)
        m_currentReply->abort();
}

void BaseExporter::fetchNextWord()
{
    if (m_cancelled.load(std::memory_order_acquire)) {
        emit cancelled();
        return;
    }

    if (m_currentIndex >= m_words.size()) {
        startRender();
        return;
    }

    const QString &word = m_words[m_currentIndex];

    QUrl defUrl("https://api.dictionaryapi.dev/api/v2/entries/en/" + word);
    QNetworkReply *reply = m_nam->get(QNetworkRequest(defUrl));
    m_currentReply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        onDefinitionReply(m_currentIndex, reply);
    });
}

void BaseExporter::onDefinitionReply(int index, QNetworkReply *defReply)
{
    if (m_cancelled.load(std::memory_order_acquire)) {
        emit cancelled();
        return;
    }

    WordEntry &entry = m_entries[index];

    if (defReply->error() == QNetworkReply::NoError) {
        auto result = NetworkClient::parseDictionaryResponse(defReply->readAll());
        if (!result.error) {
            entry.definitionHtml = result.html;
            entry.phonetic = result.phonetic;

            QUrl transUrl("https://translate.googleapis.com/translate_a/single");
            QUrlQuery query;
            query.addQueryItem("client", "gtx");
            query.addQueryItem("sl", "en");
            query.addQueryItem("tl", "ar");
            query.addQueryItem("dt", "t");
            query.addQueryItem("q", entry.word);
            transUrl.setQuery(query);

            QNetworkReply *transReply = m_nam->get(QNetworkRequest(transUrl));
            m_currentReply = transReply;
            connect(transReply, &QNetworkReply::finished, this, [this, transReply]() {
                transReply->deleteLater();
                onTranslationReply(m_currentIndex, transReply);
            });
            return;
        }
    }

    emit progress(m_currentIndex + 1, m_words.size());
    m_currentIndex++;
    QTimer::singleShot(150, this, &BaseExporter::fetchNextWord);
}

void BaseExporter::onTranslationReply(int index, QNetworkReply *transReply)
{
    if (m_cancelled.load(std::memory_order_acquire)) {
        emit cancelled();
        return;
    }

    WordEntry &entry = m_entries[index];

    if (transReply->error() == QNetworkReply::NoError) {
        auto result = NetworkClient::parseTranslationResponse(transReply->readAll());
        if (!result.error) {
            entry.translationHtml = result.html;
            entry.valid = true;
        }
    }

    emit progress(m_currentIndex + 1, m_words.size());
    m_currentIndex++;
    QTimer::singleShot(150, this, &BaseExporter::fetchNextWord);
}

void BaseExporter::startRender()
{
    if (m_cancelled.load(std::memory_order_acquire)) {
        emit cancelled();
        return;
    }

    QVector<WordEntry> validEntries;
    for (const auto &e : m_entries) {
        if (e.valid)
            validEntries.append(e);
    }

    if (validEntries.isEmpty()) {
        emit finished(false, m_outputPath);
        return;
    }

    QThread *thread = QThread::create([this, validEntries]() {
        bool success = renderToFile(validEntries, m_outputPath);
        if (m_cancelled.load(std::memory_order_acquire)) {
            emit cancelled();
        } else {
            emit finished(success, m_outputPath);
        }
    });
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}
