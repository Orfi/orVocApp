// tests/test_jsonexporter.cpp
#include <catch2/catch_all.hpp>

#include "baseexporter.h"
#include "jsonexporter.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

static int argc3 = 1;
static char appName3[] = "test";
static char *argv3[] = {appName3, nullptr};

TEST_CASE("JsonExporter renderToFile writes words and definitions", "[jsonexporter]") {
    QCoreApplication app(argc3, argv3);

    QTemporaryDir tmpDir;
    REQUIRE(tmpDir.isValid());
    QString jsonPath = tmpDir.path() + "/test-export.json";

    QVector<WordEntry> entries;
    WordEntry withDef;
    withDef.word = "hello";
    withDef.definitionHtml = "<p><b>noun</b></p><p>An utterance of hello.</p>";
    withDef.valid = true;
    entries.append(withDef);

    // Simulates a word whose fetch ultimately failed: continueOnWordFailure()
    // still includes the bare word, just with no definition.
    WordEntry withoutDef;
    withoutDef.word = "unfetchable";
    withoutDef.valid = true;
    entries.append(withoutDef);

    JsonExporter exporter({"hello", "unfetchable"});
    bool result = exporter.renderToFile(entries, jsonPath);

    REQUIRE(result == true);
    REQUIRE(QFile::exists(jsonPath));

    QFile file(jsonPath);
    REQUIRE(file.open(QIODevice::ReadOnly));
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    QJsonObject root = doc.object();
    QJsonArray words = root.value("words").toArray();
    REQUIRE(words.size() == 2);
    REQUIRE(words.contains(QJsonValue("hello")));
    REQUIRE(words.contains(QJsonValue("unfetchable")));

    QJsonObject definitions = root.value("definitions").toObject();
    REQUIRE(definitions.contains("hello"));
    REQUIRE(definitions.value("hello").toObject().value("html").toString() == withDef.definitionHtml);
    REQUIRE_FALSE(definitions.contains("unfetchable"));
}
