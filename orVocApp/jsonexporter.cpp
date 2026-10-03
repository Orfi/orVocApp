// orVocApp/jsonexporter.cpp
#include "jsonexporter.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

JsonExporter::JsonExporter(const QStringList &words, QObject *parent)
    : BaseExporter(words, parent)
{
}

bool JsonExporter::renderToFile(const QVector<WordEntry> &entries, const QString &outputPath)
{
    QJsonArray wordsArr;
    QJsonObject definitions;
    for (const WordEntry &entry : entries) {
        wordsArr.append(entry.word);
        if (!entry.definitionHtml.isEmpty()) {
            QJsonObject def;
            def["html"] = entry.definitionHtml;
            definitions[entry.word] = def;
        }
    }

    QJsonObject root;
    root["words"] = wordsArr;
    root["definitions"] = definitions;

    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    file.write(QJsonDocument(root).toJson());
    file.close();
    return true;
}
