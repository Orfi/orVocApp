#include <catch2/catch_all.hpp>

#include "baseexporter.h"
#include "networkclient.h"
#include "pdfexporter.h"

#include <QCoreApplication>
#include <QFile>
#include <QPageSize>
#include <QTemporaryDir>

static int argc = 1;
static char appName[] = "test";
static char *argv[] = {appName, nullptr};

TEST_CASE("WordEntry default state", "[baseexporter]") {
    WordEntry entry;
    REQUIRE(entry.word.isEmpty());
    REQUIRE(entry.valid == false);
}

TEST_CASE("PdfExporter letterColor produces unique colors", "[pdfexporter]") {
    QCoreApplication app(argc, argv);

    QSet<QRgb> colors;
    for (int i = 0; i < 26; ++i) {
        QColor c = PdfExporter::letterColor(i);
        REQUIRE(c.isValid());
        colors.insert(c.rgb());
    }
    REQUIRE(colors.size() == 26);
}

TEST_CASE("PdfExporter renderToFile produces a PDF file", "[pdfexporter]") {
    QCoreApplication app(argc, argv);

    QTemporaryDir tmpDir;
    REQUIRE(tmpDir.isValid());
    QString pdfPath = tmpDir.path() + "/test-output.pdf";

    QVector<WordEntry> entries;
    WordEntry e;
    e.word = "hello";
    e.phonetic = "/həˈloʊ/";
    e.definitionHtml = "<p><b>noun</b></p><p>An utterance of hello.</p>";
    e.translationHtml = "<p dir=\"rtl\" style=\"font-size: 20px;\">مرحبا</p>";
    e.valid = true;
    entries.append(e);

    PdfExporter exporter({"hello"}, QPageSize::A4);
    bool result = exporter.renderToFile(entries, pdfPath);

    REQUIRE(result == true);
    REQUIRE(QFile::exists(pdfPath));
    REQUIRE(QFile(pdfPath).size() > 0);
}

TEST_CASE("PdfExporter renderToFile works with Letter page size", "[pdfexporter]") {
    QCoreApplication app(argc, argv);

    QTemporaryDir tmpDir;
    REQUIRE(tmpDir.isValid());
    QString pdfPath = tmpDir.path() + "/test-letter.pdf";

    QVector<WordEntry> entries;
    WordEntry e;
    e.word = "test";
    e.phonetic = "/tɛst/";
    e.definitionHtml = "<p><b>noun</b></p><p>A trial.</p>";
    e.translationHtml = "<p dir=\"rtl\" style=\"font-size: 20px;\">اختبار</p>";
    e.valid = true;
    entries.append(e);

    PdfExporter exporter({"test"}, QPageSize::Letter);
    bool result = exporter.renderToFile(entries, pdfPath);

    REQUIRE(result == true);
    REQUIRE(QFile::exists(pdfPath));
}

TEST_CASE("PdfExporter renderToFile handles multiple letter groups", "[pdfexporter]") {
    QCoreApplication app(argc, argv);

    QTemporaryDir tmpDir;
    REQUIRE(tmpDir.isValid());
    QString pdfPath = tmpDir.path() + "/test-multi.pdf";

    QVector<WordEntry> entries;

    WordEntry e1;
    e1.word = "apple";
    e1.phonetic = "/ˈæpəl/";
    e1.definitionHtml = "<p><b>noun</b></p><p>A fruit.</p>";
    e1.translationHtml = "<p dir=\"rtl\">تفاحة</p>";
    e1.valid = true;
    entries.append(e1);

    WordEntry e2;
    e2.word = "book";
    e2.phonetic = "/bʊk/";
    e2.definitionHtml = "<p><b>noun</b></p><p>A written work.</p>";
    e2.translationHtml = "<p dir=\"rtl\">كتاب</p>";
    e2.valid = true;
    entries.append(e2);

    WordEntry e3;
    e3.word = "cat";
    e3.phonetic = "/kæt/";
    e3.definitionHtml = "<p><b>noun</b></p><p>A small domesticated carnivore.</p>";
    e3.translationHtml = "<p dir=\"rtl\">قطة</p>";
    e3.valid = true;
    entries.append(e3);

    PdfExporter exporter({"apple", "book", "cat"}, QPageSize::A4);
    bool result = exporter.renderToFile(entries, pdfPath);

    REQUIRE(result == true);
    REQUIRE(QFile::exists(pdfPath));
    REQUIRE(QFile(pdfPath).size() > 0);
}
