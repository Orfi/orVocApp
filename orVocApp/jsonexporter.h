// orVocApp/jsonexporter.h
#ifndef JSONEXPORTER_H
#define JSONEXPORTER_H

#include "baseexporter.h"

/**
 * @brief Exports the word bank as a JSON file enriched with cached English definitions.
 *
 * Reuses BaseExporter's cache-first fetch pipeline, but:
 *  - Skips the Arabic translation leg entirely (@ref needsTranslation = false) —
 *    this format never includes translation or pronunciation data.
 *  - Tolerates per-word definition failures (@ref continueOnWordFailure = true) —
 *    a word whose MW fetch ultimately fails is still written to the "words"
 *    array (bare, as plain-word exports always were), just without an entry
 *    in the "definitions" map, so one bad word never fails the whole export.
 *
 * Output schema: @code
 * { "words": ["word1", "word2", ...],
 *   "definitions": { "word1": { "html": "..." }, ... } }
 * @endcode
 * matching the schema written by VocabManager::exportJson and read back by
 * VocabManager::importJson.
 */
class JsonExporter : public BaseExporter
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a JsonExporter for the given words.
     * @param words The source words to fetch and export.
     * @param parent Optional QObject parent for lifetime management.
     */
    explicit JsonExporter(const QStringList &words, QObject *parent = nullptr);

    /**
     * @brief Writes the given entries to a JSON file at @p outputPath.
     * @param entries Fetched entries — every input word, since failed definitions
     *                are tolerated rather than excluded (see @ref continueOnWordFailure).
     * @param outputPath Absolute path to the JSON file to write.
     * @return True on successful write, false on file I/O failure.
     *
     * Called on a worker thread.
     */
    bool renderToFile(const QVector<WordEntry> &entries, const QString &outputPath) override;

protected:
    bool needsTranslation() const override { return false; }
    bool continueOnWordFailure() const override { return true; }
};

#endif // JSONEXPORTER_H
