// orVocab/vocabmanager.cpp
#include "vocabmanager.h"
#include <algorithm>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

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
    saveToJson();
    updateFilteredWords();
    emit wordsChanged();
    return idx;
}

void VocabManager::removeWord(const QString &word)
{
    QString normalized = word.trimmed().toLower();
    if (m_words.removeOne(normalized)) {
        saveToJson();
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

void VocabManager::loadFromJson()
{
    QFile file(dataFilePath());
    if (!file.exists())
        return;

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("VocabManager: cannot open %s for reading", qPrintable(dataFilePath()));
        return;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError) {
        qWarning("VocabManager: corrupt JSON in %s: %s",
                 qPrintable(dataFilePath()),
                 qPrintable(parseError.errorString()));
        return;
    }

    QJsonArray arr = doc.object().value("words").toArray();
    m_words.clear();
    for (const QJsonValue &v : arr)
        m_words.append(v.toString());

    updateFilteredWords();
    emit wordsChanged();
}

void VocabManager::saveToJson()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);

    QFile file(dataFilePath());
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning("VocabManager: cannot open %s for writing", qPrintable(dataFilePath()));
        return;
    }

    QJsonArray arr;
    for (const QString &w : m_words)
        arr.append(w);

    QJsonObject obj;
    obj["words"] = arr;

    file.write(QJsonDocument(obj).toJson());
    file.close();
}

void VocabManager::exportJson(const QUrl &) {}
void VocabManager::exportText(const QUrl &) {}
void VocabManager::importJson(const QUrl &) {}
void VocabManager::importText(const QUrl &) {}

QString VocabManager::dataFilePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return dir + "/vocab.json";
}
