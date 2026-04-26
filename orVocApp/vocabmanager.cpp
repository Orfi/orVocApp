// orVocApp/vocabmanager.cpp
#include "vocabmanager.h"
#include "pdfexporter.h"
#include <algorithm>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPageSize>
#include <QStandardPaths>

VocabManager::VocabManager(QObject *parent)
    : QObject(parent)
    , m_sourceModel(new QStringListModel(this))
    , m_proxyModel(new QSortFilterProxyModel(this))
{
    m_proxyModel->setSourceModel(m_sourceModel);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
}

QStringList VocabManager::words() const
{
    return m_sourceModel->stringList();
}

QSortFilterProxyModel* VocabManager::wordModel() const
{
    return m_proxyModel;
}

QStringList VocabManager::filteredWords() const
{
    QStringList result;
    for (int i = 0; i < m_proxyModel->rowCount(); ++i) {
        QModelIndex idx = m_proxyModel->index(i, 0);
        result.append(idx.data(Qt::DisplayRole).toString());
    }
    return result;
}

int VocabManager::addWord(const QString &word)
{
    QString normalized = word.trimmed().toLower();
    if (normalized.isEmpty())
        return -1;

    QStringList current = m_sourceModel->stringList();
    auto it = std::lower_bound(current.begin(), current.end(), normalized);
    int idx = static_cast<int>(it - current.begin());

    if (it != current.end() && *it == normalized)
        return idx;

    m_sourceModel->insertRow(idx);
    m_sourceModel->setData(m_sourceModel->index(idx, 0), normalized);
    saveToJson();
    emit wordsChanged();
    return idx;
}

void VocabManager::removeWord(const QString &word)
{
    QString normalized = word.trimmed().toLower();
    QStringList current = m_sourceModel->stringList();
    int idx = current.indexOf(normalized);
    if (idx >= 0) {
        m_sourceModel->removeRow(idx);
        saveToJson();
        emit wordsChanged();
    }
}

void VocabManager::filterWords(const QString &query)
{
    m_proxyModel->setFilterFixedString(query);
}

int VocabManager::indexOfWord(const QString &word) const
{
    QString normalized = word.trimmed().toLower();
    for (int i = 0; i < m_proxyModel->rowCount(); ++i) {
        QModelIndex idx = m_proxyModel->index(i, 0);
        if (idx.data(Qt::DisplayRole).toString() == normalized)
            return i;
    }
    return -1;
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
    QStringList words;
    for (const QJsonValue &v : arr)
        words.append(v.toString());

    m_sourceModel->setStringList(words);
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
    for (const QString &w : m_sourceModel->stringList())
        arr.append(w);

    QJsonObject obj;
    obj["words"] = arr;

    file.write(QJsonDocument(obj).toJson());
    file.close();
}

void VocabManager::exportJson(const QUrl &path)
{
    QFile file(path.toLocalFile());
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning("VocabManager: cannot write to %s", qPrintable(path.toLocalFile()));
        return;
    }

    QJsonArray arr;
    for (const QString &w : m_sourceModel->stringList())
        arr.append(w);

    QJsonObject obj;
    obj["words"] = arr;
    file.write(QJsonDocument(obj).toJson());
    file.close();
}

void VocabManager::exportText(const QUrl &path)
{
    QFile file(path.toLocalFile());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning("VocabManager: cannot write to %s", qPrintable(path.toLocalFile()));
        return;
    }

    for (const QString &w : m_sourceModel->stringList()) {
        file.write(w.toUtf8());
        file.write("\n");
    }
    file.close();
}

void VocabManager::importJson(const QUrl &path)
{
    QFile file(path.toLocalFile());
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("VocabManager: cannot read %s", qPrintable(path.toLocalFile()));
        return;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError) {
        qWarning("VocabManager: corrupt JSON in import file: %s",
                 qPrintable(parseError.errorString()));
        return;
    }

    QJsonArray arr = doc.object().value("words").toArray();
    QStringList newWords;
    for (const QJsonValue &v : arr)
        newWords.append(v.toString().trimmed().toLower());

    newWords.sort();
    m_sourceModel->setStringList(newWords);
    saveToJson();
    emit wordsChanged();
}

void VocabManager::importText(const QUrl &path)
{
    QFile file(path.toLocalFile());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("VocabManager: cannot read %s", qPrintable(path.toLocalFile()));
        return;
    }

    QStringList current = m_sourceModel->stringList();
    while (!file.atEnd()) {
        QString line = QString::fromUtf8(file.readLine()).trimmed().toLower();
        if (line.isEmpty())
            continue;

        auto it = std::lower_bound(current.begin(), current.end(), line);
        if (it == current.end() || *it != line)
            current.insert(it, line);
    }
    file.close();

    m_sourceModel->setStringList(current);
    saveToJson();
    emit wordsChanged();
}

QObject* VocabManager::createPdfExporter(int pageSize)
{
    auto sizeId = (pageSize == 0) ? QPageSize::A4 : QPageSize::Letter;
    auto *exporter = new PdfExporter(words(), sizeId);
    return exporter;
}

QString VocabManager::dataFilePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return dir + "/vocab.json";
}
