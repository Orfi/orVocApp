#include "baseexporter.h"

#include "applogger.h"
#include "dictionarycache.h"
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

int BaseExporter::retryBackoffMs(int attempt)
{
    Q_ASSERT(attempt >= 0 && attempt < kMaxRetries);
    // 500 * 3^attempt — 500, 1500, 4500 for attempts 0..2.
    int delay = 500;
    for (int i = 0; i < attempt; ++i)
        delay *= 3;
    return delay;
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

    m_currentPhase = FetchPhase::Definition;
    m_retryAttempt = 0;

    // Serve the definition from cache when available — avoids re-spending MW's
    // 1000 req/day quota on words already fetched in this or a prior export.
    const QString &word = m_words[m_currentIndex];
    if (DictionaryCache::instance().contains(word)) {
        CachedDefinition cached = DictionaryCache::instance().get(word);
        WordEntry &entry = m_entries[m_currentIndex];
        entry.definitionHtml = cached.html;
        entry.phonetic = cached.phonetic;
        onDefinitionReady();
        return;
    }

    issueCurrentRequest();
}

void BaseExporter::issueCurrentRequest()
{
    if (m_cancelled.load(std::memory_order_acquire)) {
        emit cancelled();
        return;
    }

    const QString &word = m_words[m_currentIndex];

    QNetworkRequest request;
    if (m_currentPhase == FetchPhase::Definition) {
        QUrl url("https://www.dictionaryapi.com/api/v3/references/collegiate/json/" + word + "?key=6db9cb08-34fb-4a9e-abd8-a08586b2113a");
        request.setUrl(url);
    } else {
        QUrl url("https://translate.googleapis.com/translate_a/single");
        QUrlQuery query;
        query.addQueryItem("client", "gtx");
        query.addQueryItem("sl", "en");
        query.addQueryItem("tl", "ar");
        query.addQueryItem("dt", "t");
        query.addQueryItem("q", word);
        url.setQuery(query);
        request.setUrl(url);
    }
    request.setTransferTimeout(kTransferTimeoutMs);

    QNetworkReply *reply = m_nam->get(request);
    m_currentReply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        onReplyFinished(reply);
    });
}

void BaseExporter::onReplyFinished(QNetworkReply *reply)
{
    if (m_cancelled.load(std::memory_order_acquire)) {
        emit cancelled();
        return;
    }

    WordEntry &entry = m_entries[m_currentIndex];
    const QString phaseName = (m_currentPhase == FetchPhase::Definition) ? "definition" : "translation";

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray body = reply->readAll();

        if (m_currentPhase == FetchPhase::Definition) {
            auto result = NetworkClient::parseDictionaryResponse(body);
            if (!result.error) {
                entry.definitionHtml = result.html;
                entry.phonetic = result.phonetic;
                DictionaryCache::instance().insert(
                    entry.word, {result.html, result.phonetic, result.audioUrl});
                onDefinitionReady();
                return;
            }
            AppLogger::log(QString("export: %1 fetch for '%2' returned HTTP 200 but failed to parse "
                                    "(attempt %3/%4)")
                               .arg(phaseName, entry.word)
                               .arg(m_retryAttempt + 1)
                               .arg(kMaxRetries));
        } else {
            auto result = NetworkClient::parseTranslationResponse(body);
            if (!result.error) {
                entry.translationHtml = result.html;
                entry.valid = true;
                advanceWord();
                return;
            }
            AppLogger::log(QString("export: %1 fetch for '%2' returned HTTP 200 but failed to parse "
                                    "(attempt %3/%4)")
                               .arg(phaseName, entry.word)
                               .arg(m_retryAttempt + 1)
                               .arg(kMaxRetries));
        }
    } else {
        AppLogger::log(QString("export: %1 fetch for '%2' failed: %3 (attempt %4/%5)")
                           .arg(phaseName, entry.word, reply->errorString())
                           .arg(m_retryAttempt + 1)
                           .arg(kMaxRetries));
    }

    scheduleRetryOrFail();
}

void BaseExporter::onDefinitionReady()
{
    if (needsTranslation()) {
        m_currentPhase = FetchPhase::Translation;
        m_retryAttempt = 0;
        issueCurrentRequest();
    } else {
        m_entries[m_currentIndex].valid = true;
        advanceWord();
    }
}

void BaseExporter::scheduleRetryOrFail()
{
    if (m_cancelled.load(std::memory_order_acquire)) {
        emit cancelled();
        return;
    }

    if (m_retryAttempt >= kMaxRetries) {
        // Exhausted retries for this word's definition. Exporters that tolerate
        // a missing definition (e.g. JsonExporter) still include the bare word
        // and move on; others hard-fail the whole export — no partial output
        // exists yet (render phase hasn't started).
        const QString phaseName = (m_currentPhase == FetchPhase::Definition) ? "definition" : "translation";
        const QString &word = m_words[m_currentIndex];

        if (m_currentPhase == FetchPhase::Definition && continueOnWordFailure()) {
            AppLogger::log(QString("export: giving up on %1 for '%2' after %3 attempts — "
                                    "skipping definition, word still included")
                               .arg(phaseName, word)
                               .arg(kMaxRetries));
            WordEntry &entry = m_entries[m_currentIndex];
            entry.definitionHtml.clear();
            entry.phonetic.clear();
            entry.valid = true;
            advanceWord();
            return;
        }

        AppLogger::log(QString("export: giving up on %1 for '%2' after %3 attempts — "
                                "hard-failing the whole export")
                           .arg(phaseName, word)
                           .arg(kMaxRetries));
        emit finished(false, m_outputPath);
        return;
    }

    const int delay = retryBackoffMs(m_retryAttempt);
    ++m_retryAttempt;
    QTimer::singleShot(delay, this, &BaseExporter::issueCurrentRequest);
}

void BaseExporter::advanceWord()
{
    emit progress(m_currentIndex + 1, m_words.size());
    m_currentIndex++;
    QTimer::singleShot(kInterWordDelayMs, this, &BaseExporter::fetchNextWord);
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
