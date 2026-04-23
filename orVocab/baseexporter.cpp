#include "baseexporter.h"

#include "networkclient.h"

#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QThread>
#include <QUrlQuery>

BaseExporter::BaseExporter(const QStringList &words, QObject *parent)
    : QObject(parent)
    , m_words(words)
{
}

BaseExporter::~BaseExporter()
{
    if (m_thread) {
        m_thread->quit();
        m_thread->wait();
    }
}

void BaseExporter::exportToFile(const QString &outputPath)
{
    m_outputPath = outputPath;
    m_thread = QThread::create([this]() { doWork(); });
    connect(m_thread, &QThread::finished, m_thread, &QThread::deleteLater);
    m_thread->start();
}

void BaseExporter::doWork()
{
    m_nam = new QNetworkAccessManager;

    QVector<WordEntry> entries;
    entries.reserve(m_words.size());

    for (int i = 0; i < m_words.size(); ++i) {
        WordEntry entry;
        entry.word = m_words[i];
        fetchWord(entry.word, entry);
        entries.append(entry);
        emit progress(i + 1, m_words.size());
    }

    QVector<WordEntry> validEntries;
    for (const auto &e : entries) {
        if (e.valid)
            validEntries.append(e);
    }

    bool success = false;
    if (!validEntries.isEmpty())
        success = renderToFile(validEntries, m_outputPath);

    delete m_nam;
    m_nam = nullptr;

    emit finished(success, m_outputPath);
    m_thread->quit();
}

void BaseExporter::fetchWord(const QString &word, WordEntry &entry)
{
    QEventLoop loop;

    // Fetch definition
    QUrl defUrl("https://api.dictionaryapi.dev/api/v2/entries/en/" + word);
    QNetworkReply *defReply = m_nam->get(QNetworkRequest(defUrl));
    connect(defReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    bool defOk = false;
    if (defReply->error() == QNetworkReply::NoError) {
        auto result = NetworkClient::parseDictionaryResponse(defReply->readAll());
        if (!result.error) {
            entry.definitionHtml = result.html;
            entry.phonetic = result.phonetic;
            defOk = true;
        }
    }
    defReply->deleteLater();

    if (!defOk) {
        entry.valid = false;
        return;
    }

    // Fetch translation
    QUrl transUrl("https://translate.googleapis.com/translate_a/single");
    QUrlQuery query;
    query.addQueryItem("client", "gtx");
    query.addQueryItem("sl", "en");
    query.addQueryItem("tl", "ar");
    query.addQueryItem("dt", "t");
    query.addQueryItem("q", word);
    transUrl.setQuery(query);

    QNetworkReply *transReply = m_nam->get(QNetworkRequest(transUrl));
    connect(transReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (transReply->error() == QNetworkReply::NoError) {
        auto result = NetworkClient::parseTranslationResponse(transReply->readAll());
        if (!result.error) {
            entry.translationHtml = result.html;
            entry.valid = true;
        }
    }
    transReply->deleteLater();
}
