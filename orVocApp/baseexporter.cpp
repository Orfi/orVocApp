#include "baseexporter.h"

#include "networkclient.h"

#include <QDebug>
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
    issueCurrentRequest();
}

void BaseExporter::issueCurrentRequest()
{
    if (m_cancelled.load(std::memory_order_acquire)) {
        emit cancelled();
        return;
    }

    qDebug().noquote() << "[RVC-45] issueCurrentRequest idx=" << m_currentIndex
                       << "word=" << m_words[m_currentIndex]
                       << "phase=" << (m_currentPhase == FetchPhase::Definition ? "Definition" : "Translation")
                       << "retryAttempt=" << m_retryAttempt;

    const QString &word = m_words[m_currentIndex];

    QNetworkRequest request;
    if (m_currentPhase == FetchPhase::Definition) {
        QUrl url("https://api.dictionaryapi.dev/api/v2/entries/en/" + word);
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
    qDebug().noquote() << "[RVC-45]   issued url=" << request.url().toString();
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

    const QVariant httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    const QByteArray peek = reply->peek(200);
    qDebug().noquote() << "[RVC-45] onReplyFinished idx=" << m_currentIndex
                       << "word=" << m_words[m_currentIndex]
                       << "phase=" << (m_currentPhase == FetchPhase::Definition ? "Definition" : "Translation")
                       << "retryAttempt=" << m_retryAttempt
                       << "qnrError=" << reply->error()
                       << "errString=" << reply->errorString()
                       << "httpStatus=" << (httpStatus.isValid() ? httpStatus.toString() : "(none)")
                       << "bodyPeek(200)=" << QString::fromUtf8(peek);

    WordEntry &entry = m_entries[m_currentIndex];

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray body = reply->readAll();

        if (m_currentPhase == FetchPhase::Definition) {
            auto result = NetworkClient::parseDictionaryResponse(body);
            qDebug().noquote() << "[RVC-45]   parseDictionary result.error=" << result.error
                               << "htmlLen=" << result.html.size()
                               << "phonetic=" << result.phonetic
                               << "audioUrl=" << result.audioUrl.toString();
            if (!result.error) {
                entry.definitionHtml = result.html;
                entry.phonetic = result.phonetic;
                m_currentPhase = FetchPhase::Translation;
                m_retryAttempt = 0;
                issueCurrentRequest();
                return;
            }
        } else {
            auto result = NetworkClient::parseTranslationResponse(body);
            qDebug().noquote() << "[RVC-45]   parseTranslation result.error=" << result.error
                               << "htmlLen=" << result.html.size();
            if (!result.error) {
                entry.translationHtml = result.html;
                entry.valid = true;
                advanceWord();
                return;
            }
        }
    }

    scheduleRetryOrFail();
}

void BaseExporter::scheduleRetryOrFail()
{
    if (m_cancelled.load(std::memory_order_acquire)) {
        emit cancelled();
        return;
    }

    qDebug().noquote() << "[RVC-45] scheduleRetryOrFail idx=" << m_currentIndex
                       << "word=" << m_words[m_currentIndex]
                       << "phase=" << (m_currentPhase == FetchPhase::Definition ? "Definition" : "Translation")
                       << "retryAttempt=" << m_retryAttempt
                       << "kMaxRetries=" << kMaxRetries;

    if (m_retryAttempt >= kMaxRetries) {
        // Exhausted retries for this word — hard-fail the whole export.
        // No partial PDF exists yet (render phase hasn't started).
        qDebug().noquote() << "[RVC-45]   HARD-FAIL: retries exhausted for idx="
                           << m_currentIndex << "word=" << m_words[m_currentIndex];
        emit finished(false, m_outputPath);
        return;
    }

    const int delay = retryBackoffMs(m_retryAttempt);
    ++m_retryAttempt;
    qDebug().noquote() << "[RVC-45]   scheduling retry in" << delay << "ms (attempt becomes"
                       << m_retryAttempt << "of" << kMaxRetries << ")";
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
