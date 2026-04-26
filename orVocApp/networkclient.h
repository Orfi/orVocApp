// orVocApp/networkclient.h
#ifndef NETWORKCLIENT_H
#define NETWORKCLIENT_H

#include <QObject>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

class QNetworkAccessManager;

/// @brief Parsed result from the dictionary API response.
struct DictionaryResult {
    QString html;      ///< HTML-formatted definitions with part-of-speech and examples.
    QString phonetic;  ///< Phonetic transcription (e.g. "/həˈloʊ/").
    QUrl audioUrl;     ///< URL to the pronunciation audio file, empty if unavailable.
    bool error = false;///< True if the response could not be parsed.
};

/// @brief Parsed result from the Google Translate GTX API response.
struct TranslationResult {
    QString html;      ///< HTML-formatted Arabic translation with RTL direction.
    bool error = false;///< True if the response could not be parsed.
};

/**
 * @brief Handles REST API calls for dictionary lookups and Arabic translations.
 *
 * Uses QNetworkAccessManager for async HTTP requests. Emits signals with
 * parsed results for definition HTML, phonetic text, audio URL, and
 * Arabic translation. Exposed to QML as a singleton.
 */
class NetworkClient : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    /// @brief Constructs the NetworkClient with an internal QNetworkAccessManager.
    explicit NetworkClient(QObject *parent = nullptr);

    /**
     * @brief Fetches the English definition from dictionaryapi.dev.
     * @param word The word to look up.
     *
     * On success emits definitionReady(), phoneticReady(), and audioUrlReady().
     * On failure emits requestFailed() with area "definition".
     */
    Q_INVOKABLE void fetchDefinition(const QString &word);

    /**
     * @brief Fetches the Arabic translation from Google Translate GTX API.
     * @param word The English word to translate.
     *
     * On success emits translationReady().
     * On failure emits requestFailed() with area "translation".
     */
    Q_INVOKABLE void fetchTranslation(const QString &word);

    /**
     * @brief Parses a raw dictionary API JSON response.
     * @param data The raw JSON bytes from dictionaryapi.dev.
     * @return DictionaryResult with HTML, phonetic, and audio URL.
     */
    static DictionaryResult parseDictionaryResponse(const QByteArray &data);

    /**
     * @brief Parses a raw Google Translate GTX JSON response.
     * @param data The raw JSON bytes from the GTX API.
     * @return TranslationResult with RTL-wrapped Arabic HTML.
     */
    static TranslationResult parseTranslationResponse(const QByteArray &data);

signals:
    /// @brief Emitted with formatted HTML when a definition is successfully fetched.
    void definitionReady(const QString &html);

    /// @brief Emitted with the phonetic transcription string.
    void phoneticReady(const QString &phonetic);

    /// @brief Emitted with the pronunciation audio URL.
    void audioUrlReady(const QUrl &url);

    /// @brief Emitted with RTL-formatted HTML when a translation is successfully fetched.
    void translationReady(const QString &html);

    /**
     * @brief Emitted when an API request fails.
     * @param area Either "definition" or "translation".
     * @param errorString Human-readable error description.
     */
    void requestFailed(const QString &area, const QString &errorString);

private:
    QNetworkAccessManager *m_nam;
};

#endif // NETWORKCLIENT_H
