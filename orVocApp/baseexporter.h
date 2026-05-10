#ifndef BASEEXPORTER_H
#define BASEEXPORTER_H

#include <QObject>
#include <QPointer>
#include <QString>
#include <QUrl>
#include <QVector>
#include <atomic>

class QNetworkAccessManager;
class QNetworkReply;
class QThread;

/**
 * @brief One word's fetched dictionary and translation data, ready for rendering.
 *
 * Populated incrementally by BaseExporter during the fetch phase. An entry is
 * considered renderable only when @ref valid is true, which requires both a
 * successful dictionary response and a successful translation response.
 */
struct WordEntry {
    QString word;              ///< The source word being exported.
    QString phonetic;          ///< Phonetic transcription (e.g. "/həˈloʊ/"), empty if none.
    QString definitionHtml;    ///< HTML-formatted English definitions from the dictionary API.
    QString translationHtml;   ///< HTML-formatted RTL-wrapped Arabic translation.
    bool valid = false;        ///< True iff both dictionary and translation fetches succeeded.
};

/**
 * @brief Abstract base class for exporters that need dictionary + translation data per word.
 *
 * Runs a two-phase pipeline:
 *  1. Fetch phase — for each input word, sequentially fetches the dictionary entry
 *     and then the Arabic translation via QNetworkAccessManager. Each individual
 *     request carries a @ref kTransferTimeoutMs transfer timeout and is retried up
 *     to @ref kMaxRetries times with exponential backoff (see @ref retryBackoffMs)
 *     on any network/parse failure. If retries are exhausted for a single word, the
 *     whole export hard-fails via @ref finished with @c success=false — the pipeline
 *     never silently drops words from the output. A @ref kInterWordDelayMs throttle
 *     is applied between consecutive words to stay under the rate-limit of the free
 *     public APIs (dictionaryapi.dev is Cloudflare-fronted and trips a 1015 ban on
 *     bursts).
 *  2. Render phase — spawns a QThread that calls the subclass-provided
 *     @ref renderToFile implementation with the successfully-fetched entries.
 *
 * Subclasses implement @ref renderToFile to produce a format-specific output file
 * (e.g. PDF). Progress is reported via @ref progress; completion via @ref finished;
 * user-initiated cancellation via @ref cancelled.
 */
class BaseExporter : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a BaseExporter for the given list of source words.
     * @param words The source words to fetch and export.
     * @param parent Optional QObject parent for lifetime management.
     */
    explicit BaseExporter(const QStringList &words, QObject *parent = nullptr);

    /// @brief Destroys the exporter and releases the internal QNetworkAccessManager.
    ~BaseExporter() override;

    /**
     * @brief Kicks off the fetch + render pipeline, writing the result to @p outputPath.
     * @param outputPath Absolute path to the target output file.
     *
     * Emits @ref progress repeatedly during the fetch phase and @ref finished once
     * the render phase completes (successfully or otherwise).
     */
    Q_INVOKABLE void exportToFile(const QString &outputPath);

    /**
     * @brief Requests cancellation of the in-progress export.
     *
     * Sets the atomic cancel flag and aborts the currently outstanding
     * QNetworkReply (if any). Fetch-phase callbacks and the render loop
     * observe the flag at their next checkpoint and emit @ref cancelled
     * instead of @ref finished. Safe to call after the pipeline has
     * already completed (no-op in that case). Thread-safe.
     */
    Q_INVOKABLE void requestCancel();

    /**
     * @brief Returns whether cancellation has been requested.
     * @return True once @ref requestCancel has been called on this exporter.
     *
     * Safe to call from the render worker thread — reads the atomic flag
     * with acquire ordering.
     */
    bool isCancelled() const { return m_cancelled.load(std::memory_order_acquire); }

    /// @brief Delay between finishing one word's fetch pair and starting the next,
    /// in milliseconds. Paces requests to dictionaryapi.dev (Cloudflare-fronted,
    /// trips rate-limit 1015 at low values); keep ≥ ~1000 to avoid 429 bursts.
    static constexpr int kInterWordDelayMs = 1000;

    /// @brief Maximum retry attempts per network request before failing the whole export.
    static constexpr int kMaxRetries = 3;

    /// @brief Per-request transfer timeout in milliseconds (applied to both dict and translation).
    static constexpr int kTransferTimeoutMs = 10000;

    /**
     * @brief Returns the delay to wait *before* attempting retry number @p attempt.
     * @param attempt Zero-based attempt index. Must satisfy
     *                @c 0 <= attempt < kMaxRetries (i.e. 0, 1, or 2 with the
     *                current @ref kMaxRetries of 3). 0 == delay before the
     *                first retry, 1 == delay before the second, etc.
     * @return 500 ms for attempt 0, 1500 ms for attempt 1, 4500 ms for attempt 2, and
     *         more generally @c 500 * 3^attempt. Defined as a pure function so it can
     *         be unit-tested without touching the network.
     *
     * @note Behaviour is undefined for @p attempt outside the documented
     *       range; a Q_ASSERT enforces the contract in debug builds. The
     *       @c int return type and base-3 growth mean callers must not
     *       pass large values — @c 500 * 3^attempt overflows 32-bit signed
     *       around @c attempt == 19.
     */
    static int retryBackoffMs(int attempt);

signals:
    /**
     * @brief Emitted after each word's fetch phase finishes (regardless of success).
     * @param current Number of words processed so far (1-based).
     * @param total Total number of words in the input list.
     */
    void progress(int current, int total);

    /**
     * @brief Emitted once the render phase has completed.
     * @param success True if the subclass's renderToFile returned true and a valid
     *                output was produced; false on render error or if no fetched
     *                entries were valid.
     * @param filePath The absolute path that was written to (same as the argument
     *                 passed to @ref exportToFile).
     */
    void finished(bool success, const QString &filePath);

    /**
     * @brief Emitted when a user-requested cancellation has taken effect.
     *
     * Fires instead of @ref finished when @ref requestCancel is observed
     * by the fetch loop or the render worker. Any partial output file is
     * removed before this signal is emitted.
     */
    void cancelled();

protected:
    /**
     * @brief Subclass hook — render the successfully-fetched entries to @p outputPath.
     * @param entries The subset of WordEntry objects whose fetches succeeded.
     * @param outputPath Absolute path to the target output file.
     * @return True on successful render, false on any failure.
     *
     * Called on a worker QThread; implementations must not touch UI or objects
     * owned by the main thread.
     */
    virtual bool renderToFile(const QVector<WordEntry> &entries, const QString &outputPath) = 0;

private:
    /// @brief Which leg of the per-word fetch pair is currently in-flight.
    enum class FetchPhase { Definition, Translation };

    void fetchNextWord();         ///< Resets retry state and starts the definition leg for m_currentIndex.
    void issueCurrentRequest();   ///< (Re-)issues the HTTP GET for the current phase+word.
    void onReplyFinished(QNetworkReply *reply); ///< Handles dict or translation reply based on m_currentPhase.
    void scheduleRetryOrFail();   ///< Schedules a backoff retry of issueCurrentRequest, or hard-fails the export.
    void advanceWord();           ///< Emits progress, clears retry state, schedules next word via the 150ms throttle.
    void startRender();

    QStringList m_words;
    QString m_outputPath;
    QNetworkAccessManager *m_nam = nullptr;
    QVector<WordEntry> m_entries;
    int m_currentIndex = 0;
    std::atomic<bool> m_cancelled{false};
    QPointer<QNetworkReply> m_currentReply;

    FetchPhase m_currentPhase = FetchPhase::Definition;
    int m_retryAttempt = 0;       ///< Retries already attempted for the current leg (0..kMaxRetries).
};

#endif // BASEEXPORTER_H
