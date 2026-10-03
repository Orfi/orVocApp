// orVocApp/dictionarycache.cpp
#include "dictionarycache.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

DictionaryCache &DictionaryCache::instance()
{
    static DictionaryCache cache;
    return cache;
}

DictionaryCache::DictionaryCache()
{
    load();
}

bool DictionaryCache::contains(const QString &word) const
{
    return m_entries.contains(word.trimmed().toLower());
}

CachedDefinition DictionaryCache::get(const QString &word) const
{
    return m_entries.value(word.trimmed().toLower());
}

void DictionaryCache::insert(const QString &word, const CachedDefinition &definition)
{
    QString normalized = word.trimmed().toLower();
    if (normalized.isEmpty())
        return;

    m_entries.insert(normalized, definition);
    save();
}

void DictionaryCache::remove(const QString &word)
{
    QString normalized = word.trimmed().toLower();
    if (m_entries.remove(normalized) > 0)
        save();
}

QString DictionaryCache::filePath()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return dir + "/dictionary_cache.json";
}

void DictionaryCache::load()
{
    QFile file(filePath());
    if (!file.exists() || !file.open(QIODevice::ReadOnly))
        return;

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError || !doc.isObject())
        return;

    QJsonObject entries = doc.object().value("entries").toObject();
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        QJsonObject obj = it.value().toObject();
        CachedDefinition def;
        def.html = obj.value("html").toString();
        def.phonetic = obj.value("phonetic").toString();
        def.audioUrl = QUrl(obj.value("audioUrl").toString());
        m_entries.insert(it.key(), def);
    }
}

void DictionaryCache::save() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);

    QFile file(filePath());
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning("DictionaryCache: cannot open %s for writing", qPrintable(filePath()));
        return;
    }

    QJsonObject entries;
    for (auto it = m_entries.constBegin(); it != m_entries.constEnd(); ++it) {
        QJsonObject obj;
        obj["html"] = it.value().html;
        obj["phonetic"] = it.value().phonetic;
        obj["audioUrl"] = it.value().audioUrl.toString();
        entries[it.key()] = obj;
    }

    QJsonObject root;
    root["entries"] = entries;

    file.write(QJsonDocument(root).toJson());
    file.close();
}
