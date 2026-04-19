// orVocab/vocabmanager.h
#ifndef VOCABMANAGER_H
#define VOCABMANAGER_H

#include <QObject>
#include <QStringList>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

/**
 * @brief Manages the user's vocabulary word bank.
 *
 * Provides CRUD operations on an alphabetically sorted word list,
 * real-time filtering, JSON persistence via QStandardPaths, and
 * import/export in JSON and plain-text formats. Exposed to QML as a singleton.
 */
class VocabManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QStringList words READ words NOTIFY wordsChanged)
    Q_PROPERTY(QStringList filteredWords READ filteredWords NOTIFY filteredWordsChanged)

public:
    /// @brief Constructs the VocabManager.
    explicit VocabManager(QObject *parent = nullptr);

    /// @brief Returns the full sorted word list.
    QStringList words() const;

    /// @brief Returns the currently filtered subset of words.
    QStringList filteredWords() const;

    /**
     * @brief Adds a word to the bank in alphabetical order.
     * @param word The word to add (trimmed and lowercased internally).
     * @return Index of the word in the list, or -1 if the input is empty.
     *         If the word already exists, returns its existing index without duplicating.
     */
    Q_INVOKABLE int addWord(const QString &word);

    /**
     * @brief Removes a word from the bank.
     * @param word The word to remove. No-op if the word does not exist.
     */
    Q_INVOKABLE void removeWord(const QString &word);

    /**
     * @brief Filters the word list using case-insensitive substring matching.
     * @param query The search string. An empty query shows all words.
     */
    Q_INVOKABLE void filterWords(const QString &query);

    /// @brief Loads the word list from vocab.json in the app data directory.
    Q_INVOKABLE void loadFromJson();

    /// @brief Saves the current word list to vocab.json in the app data directory.
    Q_INVOKABLE void saveToJson();

    /**
     * @brief Exports the word list as a JSON file.
     * @param path Destination file URL chosen by the user.
     */
    Q_INVOKABLE void exportJson(const QUrl &path);

    /**
     * @brief Exports the word list as a plain-text file (one word per line).
     * @param path Destination file URL chosen by the user.
     */
    Q_INVOKABLE void exportText(const QUrl &path);

    /**
     * @brief Replaces the entire word bank from a JSON file.
     * @param path Source file URL. Corrupt files are rejected (existing list preserved).
     */
    Q_INVOKABLE void importJson(const QUrl &path);

    /**
     * @brief Merges words from a text file into the existing bank.
     * @param path Source file URL (one word per line). Duplicates are skipped.
     */
    Q_INVOKABLE void importText(const QUrl &path);

signals:
    /// @brief Emitted when the word list changes (add, remove, import, load).
    void wordsChanged();

    /// @brief Emitted when the filtered word list changes.
    void filteredWordsChanged();

private:
    QStringList m_words;
    QStringList m_filteredWords;
    QString m_currentFilter;

    QString dataFilePath() const;
    void updateFilteredWords();
};

#endif // VOCABMANAGER_H
