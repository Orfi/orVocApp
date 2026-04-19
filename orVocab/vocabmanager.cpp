// orVocab/vocabmanager.cpp
#include "vocabmanager.h"
#include <algorithm>

VocabManager::VocabManager(QObject *parent)
    : QObject(parent)
{
}

QStringList VocabManager::words() const
{
    return m_words;
}

QStringList VocabManager::filteredWords() const
{
    return m_filteredWords;
}

int VocabManager::addWord(const QString &word)
{
    QString normalized = word.trimmed().toLower();
    if (normalized.isEmpty())
        return -1;

    auto it = std::lower_bound(m_words.begin(), m_words.end(), normalized);
    int idx = static_cast<int>(it - m_words.begin());

    if (it != m_words.end() && *it == normalized)
        return idx;

    m_words.insert(idx, normalized);
    updateFilteredWords();
    emit wordsChanged();
    return idx;
}

void VocabManager::removeWord(const QString &word)
{
    QString normalized = word.trimmed().toLower();
    if (m_words.removeOne(normalized)) {
        updateFilteredWords();
        emit wordsChanged();
    }
}

void VocabManager::filterWords(const QString &query)
{
    m_currentFilter = query;
    updateFilteredWords();
}

void VocabManager::updateFilteredWords()
{
    QStringList filtered;
    if (m_currentFilter.isEmpty()) {
        filtered = m_words;
    } else {
        for (const QString &w : m_words) {
            if (w.contains(m_currentFilter, Qt::CaseInsensitive))
                filtered.append(w);
        }
    }
    if (filtered != m_filteredWords) {
        m_filteredWords = filtered;
        emit filteredWordsChanged();
    }
}

void VocabManager::loadFromJson() {}
void VocabManager::saveToJson() {}
void VocabManager::exportJson(const QUrl &) {}
void VocabManager::exportText(const QUrl &) {}
void VocabManager::importJson(const QUrl &) {}
void VocabManager::importText(const QUrl &) {}
QString VocabManager::dataFilePath() const { return QString(); }
