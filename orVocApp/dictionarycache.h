// orVocApp/dictionarycache.h
#ifndef DICTIONARYCACHE_H
#define DICTIONARYCACHE_H

#include <QHash>
#include <QString>
#include <QUrl>

/**
 * @brief A cached dictionary lookup result (subset of @ref DictionaryResult, success only).
 */
struct CachedDefinition {
    QString html;      ///< HTML-formatted definition.
    QString phonetic;  ///< Phonetic transcription, may be empty.
    QUrl audioUrl;     ///< Pronunciation audio URL, may be empty.
};

/**
 * @brief Process-wide, disk-persisted cache of Merriam-Webster definition lookups.
 *
 * Merriam-Webster's Collegiate API enforces a strict 1000 requests/day quota per
 * key. Without caching, every PDF export re-fetches every word from scratch, and
 * every failed word retries up to 3x — both of which burn through the daily quota
 * very quickly on word banks of a few hundred+ entries.
 *
 * This cache stores successfully-parsed definitions keyed by lowercased word in
 * @c dictionary_cache.json under @c QStandardPaths::AppDataLocation, shared by
 * both the interactive @ref NetworkClient lookups and the @ref BaseExporter
 * fetch pipeline. Only successful lookups are cached — failures are always
 * retried so a transient error or a not-yet-published word isn't stuck.
 *
 * Not exposed to QML; used internally from C++ only. Not thread-safe — callers
 * must only use it from the GUI thread (both NetworkClient and BaseExporter's
 * fetch phase run there).
 */
class DictionaryCache
{
public:
    /// @brief Returns the process-wide singleton instance, loading from disk on first use.
    static DictionaryCache &instance();

    /**
     * @brief Checks whether a definition is cached for @p word.
     * @param word The word to look up; matched case-insensitively.
     * @return True if a cached definition exists.
     */
    bool contains(const QString &word) const;

    /**
     * @brief Retrieves a cached definition. Only valid if @ref contains returns true.
     * @param word The word to look up; matched case-insensitively.
     */
    CachedDefinition get(const QString &word) const;

    /**
     * @brief Stores a successfully-fetched definition and persists the cache to disk.
     * @param word The word being cached; stored case-insensitively.
     * @param definition The definition data to cache.
     */
    void insert(const QString &word, const CachedDefinition &definition);

    /**
     * @brief Evicts a word's cached definition, if any, and persists the change.
     * @param word The word to evict; matched case-insensitively.
     *
     * No-op if the word isn't cached. Used so that deleting a word from the
     * vocab bank and re-adding it later forces a fresh MW lookup instead of
     * silently returning the old cached result.
     */
    void remove(const QString &word);

private:
    DictionaryCache();

    void load();
    void save() const;
    static QString filePath();

    QHash<QString, CachedDefinition> m_entries;
};

#endif // DICTIONARYCACHE_H
