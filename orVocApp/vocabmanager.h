// orVocApp/vocabmanager.h
#ifndef VOCABMANAGER_H
#define VOCABMANAGER_H

#include <QObject>
#include <QSortFilterProxyModel>
#include <QStringList>
#include <QStringListModel>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

/**
 * @brief Owns the user's vocabulary word bank and exposes it to QML.
 *
 * Wraps a QStringListModel (source) behind a QSortFilterProxyModel (filtered view)
 * to support case-insensitive live filtering from the QML sidebar. All mutations
 * auto-persist to vocab.json under QStandardPaths::AppDataLocation.
 *
 * Registered with QML as a singleton under URI "orvocapp".
 */
class VocabManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QStringList words READ words NOTIFY wordsChanged)
    Q_PROPERTY(QSortFilterProxyModel* wordModel READ wordModel CONSTANT)

public:
    /**
     * @brief Constructs an empty VocabManager. Call @ref loadFromJson to populate.
     * @param parent Optional QObject parent for lifetime management.
     */
    explicit VocabManager(QObject *parent = nullptr);

    /**
     * @brief Returns the full, unfiltered, sorted list of words.
     * @return A QStringList of all stored words, lowercase and alphabetically sorted.
     */
    QStringList words() const;

    /**
     * @brief Returns the filter proxy model used by the sidebar ListView.
     * @return A QSortFilterProxyModel that applies the current filter string.
     */
    QSortFilterProxyModel* wordModel() const;

    /**
     * @brief Returns the current filtered view as a plain list (test helper).
     * @return The subset of words that currently pass the filter, in proxy order.
     */
    QStringList filteredWords() const;

    /**
     * @brief Adds a word to the bank, preserving alphabetical order.
     * @param word The word to add; trimmed and lowercased before insertion.
     * @return The index of the word after insertion (0-based in the source model),
     *         or -1 if @p word was empty or whitespace-only. If the word already
     *         exists, returns its existing index without modifying the bank.
     *
     * Auto-saves to JSON on successful insertion and emits @ref wordsChanged.
     */
    Q_INVOKABLE int addWord(const QString &word);

    /**
     * @brief Removes a word from the bank if it exists.
     * @param word The word to remove; trimmed and lowercased before lookup.
     *
     * Auto-saves to JSON and emits @ref wordsChanged when the word is found and
     * removed; a no-op if the word is not present.
     */
    Q_INVOKABLE void removeWord(const QString &word);

    /**
     * @brief Sets the case-insensitive "contains" filter on the proxy model.
     * @param query The substring to filter by; empty string shows all words.
     */
    Q_INVOKABLE void filterWords(const QString &query);

    /**
     * @brief Finds the row index of @p word in the current proxy (filtered) view.
     * @param word The word to locate; trimmed and lowercased before comparison.
     * @return The 0-based proxy row index, or -1 if not present in the current view.
     */
    Q_INVOKABLE int indexOfWord(const QString &word) const;

    /// @brief Loads the word bank from vocab.json; silently starts empty if missing or corrupt.
    Q_INVOKABLE void loadFromJson();

    /// @brief Persists the current word bank to vocab.json under AppDataLocation.
    Q_INVOKABLE void saveToJson();

    /**
     * @brief Exports the word bank as a JSON file at a user-chosen path.
     * @param path File URL chosen by the user via FileDialog.
     */
    Q_INVOKABLE void exportJson(const QUrl &path);

    /**
     * @brief Exports the word bank as a newline-separated plain-text file.
     * @param path File URL chosen by the user via FileDialog.
     */
    Q_INVOKABLE void exportText(const QUrl &path);

    /**
     * @brief Replaces the entire word bank from a JSON file.
     * @param path File URL chosen by the user via FileDialog.
     *
     * On corrupt JSON or read failure the existing bank is left untouched.
     * On success, auto-saves and emits @ref wordsChanged.
     */
    Q_INVOKABLE void importJson(const QUrl &path);

    /**
     * @brief Merges words from a plain-text file into the current bank.
     * @param path File URL chosen by the user via FileDialog.
     *
     * Each non-empty line becomes a word (trimmed, lowercased). Duplicates
     * are skipped. Auto-saves and emits @ref wordsChanged.
     */
    Q_INVOKABLE void importText(const QUrl &path);

    /**
     * @brief Factory for a PdfExporter pre-populated with the current word list.
     * @param pageSize 0 for A4, 1 for Letter.
     * @return A newly-allocated PdfExporter QObject; ownership is transferred to QML.
     */
    Q_INVOKABLE QObject* createPdfExporter(int pageSize);

signals:
    /// @brief Emitted whenever the word list mutates (add, remove, import, load).
    void wordsChanged();

private:
    QStringListModel *m_sourceModel;
    QSortFilterProxyModel *m_proxyModel;

    QString dataFilePath() const;
};

#endif // VOCABMANAGER_H
