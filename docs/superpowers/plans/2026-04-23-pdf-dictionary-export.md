# PDF Dictionary Export Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Export the vocabulary list as a printer-friendly PDF dictionary with English definitions, Arabic translations, color-coded letter tabs, and a cover page.

**Architecture:** `BaseExporter` abstract class handles threading and batch API fetching. `PdfExporter` subclass renders the dictionary PDF using QPrinter + QPainter + QTextDocument. A QML `ExportOverlay` component shows progress and notifications. The export runs on a background QThread so the UI stays responsive.

**Tech Stack:** Qt 6 (PrintSupport, Network, Quick), C++17, QML, Catch2 for tests.

---

## File Structure

| File | Responsibility |
|------|---------------|
| `orVocab/baseexporter.h` | `WordEntry` struct, `BaseExporter` abstract class declaration |
| `orVocab/baseexporter.cpp` | Worker thread setup, QNAM creation, sequential batch fetch, progress emission |
| `orVocab/pdfexporter.h` | `PdfExporter` subclass declaration |
| `orVocab/pdfexporter.cpp` | PDF rendering: cover page, letter sections, color tabs, entry layout, page headers/footers |
| `orVocab/ExportOverlay.qml` | Bottom-right progress bar + sequenced fade notification |
| `orVocab/Main.qml` | "Export PDF" button, page size dialog, file dialog, overlay integration |
| `orVocab/CMakeLists.txt` | New sources, QML file, PrintSupport dependency |
| `tests/CMakeLists.txt` | New test file, PrintSupport link |
| `tests/test_pdfexporter.cpp` | Tests for BaseExporter batch fetch logic and PdfExporter rendering |

---

### Task 1: CMakeLists.txt — Add PrintSupport dependency and new source files

**Files:**
- Modify: `orVocab/CMakeLists.txt:1` (find_package line)
- Modify: `orVocab/CMakeLists.txt:5-11` (qt_add_executable sources)
- Modify: `orVocab/CMakeLists.txt:13-28` (qt_add_qml_module QML_FILES)
- Modify: `orVocab/CMakeLists.txt:37-44` (target_link_libraries)

- [ ] **Step 1: Add PrintSupport to find_package**

Change line 1 of `orVocab/CMakeLists.txt` from:
```cmake
find_package(Qt6 REQUIRED COMPONENTS Quick Network Multimedia QuickDialogs2 Widgets)
```
to:
```cmake
find_package(Qt6 REQUIRED COMPONENTS Quick Network Multimedia QuickDialogs2 Widgets PrintSupport)
```

- [ ] **Step 2: Add new source files to qt_add_executable**

Change the `qt_add_executable` block to:
```cmake
qt_add_executable(orVocApp
    main.cpp
    vocabmanager.h
    vocabmanager.cpp
    networkclient.h
    networkclient.cpp
    baseexporter.h
    baseexporter.cpp
    pdfexporter.h
    pdfexporter.cpp
)
```

- [ ] **Step 3: Add ExportOverlay.qml to QML_FILES**

Add `ExportOverlay.qml` to the `QML_FILES` list in `qt_add_qml_module`:
```cmake
qt_add_qml_module(orVocApp
    URI orVocab
    QML_FILES
        Main.qml
        Sidebar.qml
        TranslationView.qml
        ExportOverlay.qml
    RESOURCES
        splash.png
        icon.png
        icon_256.png
        icon_128.png
        icon_64.png
        icon_48.png
        icon_32.png
        icon_16.png
)
```

- [ ] **Step 4: Add PrintSupport to target_link_libraries**

Change `target_link_libraries` to:
```cmake
target_link_libraries(orVocApp
    PRIVATE
        Qt6::Quick
        Qt6::Network
        Qt6::Multimedia
        Qt6::QuickDialogs2
        Qt6::Widgets
        Qt6::PrintSupport
)
```

- [ ] **Step 5: Commit**

```bash
git add orVocab/CMakeLists.txt
git commit -m "feat(RVC-31): ADDED: PrintSupport dependency and new source file entries to CMakeLists"
```

---

### Task 2: BaseExporter — WordEntry struct and abstract class header

**Files:**
- Create: `orVocab/baseexporter.h`

- [ ] **Step 1: Create baseexporter.h**

```cpp
#ifndef BASEEXPORTER_H
#define BASEEXPORTER_H

#include <QObject>
#include <QString>
#include <QUrl>
#include <QVector>

class QNetworkAccessManager;
class QThread;

struct WordEntry {
    QString word;
    QString phonetic;
    QString definitionHtml;
    QString translationHtml;
    bool valid = false;
};

class BaseExporter : public QObject
{
    Q_OBJECT

public:
    explicit BaseExporter(const QStringList &words, QObject *parent = nullptr);
    ~BaseExporter() override;

    void exportToFile(const QString &outputPath);

signals:
    void progress(int current, int total);
    void finished(bool success, const QString &filePath);

protected:
    virtual bool renderToFile(const QVector<WordEntry> &entries, const QString &outputPath) = 0;

private slots:
    void doWork();

private:
    void fetchWord(const QString &word, WordEntry &entry);

    QStringList m_words;
    QString m_outputPath;
    QThread *m_thread = nullptr;
    QNetworkAccessManager *m_nam = nullptr;
};

#endif // BASEEXPORTER_H
```

- [ ] **Step 2: Commit**

```bash
git add orVocab/baseexporter.h
git commit -m "feat(RVC-31): ADDED: BaseExporter abstract class header with WordEntry struct"
```

---

### Task 3: BaseExporter — Implementation with threaded batch fetch

**Files:**
- Create: `orVocab/baseexporter.cpp`

- [ ] **Step 1: Create baseexporter.cpp**

```cpp
#include "baseexporter.h"

#include "networkclient.h"

#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QThread>
#include <QUrlQuery>

BaseExporter::BaseExporter(const QStringList &words, QObject *parent)
    : QObject(parent)
    , m_words(words)
{
}

BaseExporter::~BaseExporter()
{
    if (m_thread) {
        m_thread->quit();
        m_thread->wait();
    }
}

void BaseExporter::exportToFile(const QString &outputPath)
{
    m_outputPath = outputPath;
    m_thread = new QThread;
    moveToThread(m_thread);
    connect(m_thread, &QThread::started, this, &BaseExporter::doWork);
    connect(m_thread, &QThread::finished, m_thread, &QThread::deleteLater);
    m_thread->start();
}

void BaseExporter::doWork()
{
    m_nam = new QNetworkAccessManager;

    QVector<WordEntry> entries;
    entries.reserve(m_words.size());

    for (int i = 0; i < m_words.size(); ++i) {
        WordEntry entry;
        entry.word = m_words[i];
        fetchWord(entry.word, entry);
        entries.append(entry);
        emit progress(i + 1, m_words.size());
    }

    QVector<WordEntry> validEntries;
    for (const auto &e : entries) {
        if (e.valid)
            validEntries.append(e);
    }

    bool success = false;
    if (!validEntries.isEmpty())
        success = renderToFile(validEntries, m_outputPath);

    delete m_nam;
    m_nam = nullptr;

    emit finished(success, m_outputPath);
    m_thread->quit();
}

void BaseExporter::fetchWord(const QString &word, WordEntry &entry)
{
    QEventLoop loop;

    // Fetch definition
    QUrl defUrl("https://api.dictionaryapi.dev/api/v2/entries/en/" + word);
    QNetworkReply *defReply = m_nam->get(QNetworkRequest(defUrl));
    connect(defReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    bool defOk = false;
    if (defReply->error() == QNetworkReply::NoError) {
        auto result = NetworkClient::parseDictionaryResponse(defReply->readAll());
        if (!result.error) {
            entry.definitionHtml = result.html;
            entry.phonetic = result.phonetic;
            defOk = true;
        }
    }
    defReply->deleteLater();

    if (!defOk) {
        entry.valid = false;
        return;
    }

    // Fetch translation
    QUrl transUrl("https://translate.googleapis.com/translate_a/single");
    QUrlQuery query;
    query.addQueryItem("client", "gtx");
    query.addQueryItem("sl", "en");
    query.addQueryItem("tl", "ar");
    query.addQueryItem("dt", "t");
    query.addQueryItem("q", word);
    transUrl.setQuery(query);

    QNetworkReply *transReply = m_nam->get(QNetworkRequest(transUrl));
    connect(transReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (transReply->error() == QNetworkReply::NoError) {
        auto result = NetworkClient::parseTranslationResponse(transReply->readAll());
        if (!result.error) {
            entry.translationHtml = result.html;
            entry.valid = true;
        }
    }
    transReply->deleteLater();
}
```

- [ ] **Step 2: Commit**

```bash
git add orVocab/baseexporter.cpp
git commit -m "feat(RVC-31): ADDED: BaseExporter implementation with threaded batch fetch"
```

---

### Task 4: PdfExporter — Header

**Files:**
- Create: `orVocab/pdfexporter.h`

- [ ] **Step 1: Create pdfexporter.h**

```cpp
#ifndef PDFEXPORTER_H
#define PDFEXPORTER_H

#include "baseexporter.h"

#include <QPageSize>

class QPainter;

class PdfExporter : public BaseExporter
{
    Q_OBJECT

public:
    explicit PdfExporter(const QStringList &words,
                         QPageSize::PageSizeId pageSize,
                         QObject *parent = nullptr);

    bool renderToFile(const QVector<WordEntry> &entries, const QString &outputPath) override;
    static QColor letterColor(int letterIndex);

private:
    void drawCoverPage(QPainter &painter, int wordCount, const QRectF &pageRect);
    void drawPageHeader(QPainter &painter, QChar letter, int letterIndex, const QRectF &pageRect);
    void drawPageFooter(QPainter &painter, int pageNumber, const QRectF &pageRect);
    qreal drawWordEntry(QPainter &painter, const WordEntry &entry, qreal yPos, const QRectF &pageRect);
    qreal measureWordEntry(const WordEntry &entry, const QRectF &pageRect);

    QPageSize::PageSizeId m_pageSize;
};

#endif // PDFEXPORTER_H
```

- [ ] **Step 2: Commit**

```bash
git add orVocab/pdfexporter.h
git commit -m "feat(RVC-31): ADDED: PdfExporter subclass header"
```

---

### Task 5: PdfExporter — Implementation

**Files:**
- Create: `orVocab/pdfexporter.cpp`

- [ ] **Step 1: Create pdfexporter.cpp**

```cpp
#include "pdfexporter.h"

#include <QDate>
#include <QFont>
#include <QMargins>
#include <QPainter>
#include <QPrinter>
#include <QTextDocument>

static const qreal kMarginMm = 20.0;
static const qreal kHeaderHeight = 40.0;
static const qreal kFooterHeight = 30.0;
static const qreal kEntrySpacing = 16.0;

PdfExporter::PdfExporter(const QStringList &words,
                         QPageSize::PageSizeId pageSize,
                         QObject *parent)
    : BaseExporter(words, parent)
    , m_pageSize(pageSize)
{
}

QColor PdfExporter::letterColor(int letterIndex)
{
    qreal hue = (letterIndex / 26.0) * 360.0;
    return QColor::fromHsv(static_cast<int>(hue), 200, 200);
}

bool PdfExporter::renderToFile(const QVector<WordEntry> &entries, const QString &outputPath)
{
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(outputPath);
    printer.setPageSize(QPageSize(m_pageSize));
    printer.setPageOrientation(QPageLayout::Portrait);
    printer.setPageMargins(QMarginsF(kMarginMm, kMarginMm, kMarginMm, kMarginMm),
                           QPageLayout::Millimeter);

    QPainter painter;
    if (!painter.begin(&printer))
        return false;

    QRectF pageRect = printer.pageRect(QPrinter::DevicePixel);

    // Cover page
    drawCoverPage(painter, entries.size(), pageRect);

    // Group entries by first letter
    QMap<QChar, QVector<const WordEntry *>> letterGroups;
    for (const auto &e : entries) {
        QChar letter = e.word.at(0).toUpper();
        letterGroups[letter].append(&e);
    }

    int pageNumber = 1;

    for (auto it = letterGroups.constBegin(); it != letterGroups.constEnd(); ++it) {
        QChar letter = it.key();
        int letterIndex = letter.unicode() - QChar('A').unicode();
        const auto &words = it.value();

        printer.newPage();
        pageNumber++;
        drawPageHeader(painter, letter, letterIndex, pageRect);
        drawPageFooter(painter, pageNumber, pageRect);

        qreal contentTop = kHeaderHeight + kEntrySpacing;
        qreal contentBottom = pageRect.height() - kFooterHeight;
        qreal yPos = contentTop;

        for (const auto *entry : words) {
            qreal entryHeight = measureWordEntry(*entry, pageRect);

            if (yPos + entryHeight > contentBottom) {
                printer.newPage();
                pageNumber++;
                drawPageHeader(painter, letter, letterIndex, pageRect);
                drawPageFooter(painter, pageNumber, pageRect);
                yPos = contentTop;
            }

            yPos = drawWordEntry(painter, *entry, yPos, pageRect);
            yPos += kEntrySpacing;
        }
    }

    painter.end();
    return true;
}

void PdfExporter::drawCoverPage(QPainter &painter, int wordCount, const QRectF &pageRect)
{
    qreal centerX = pageRect.width() / 2.0;
    qreal centerY = pageRect.height() / 2.0;

    // Rainbow gradient lines (top and bottom)
    qreal lineY_top = pageRect.height() * 0.15;
    qreal lineY_bottom = pageRect.height() * 0.85;
    qreal lineLeft = pageRect.width() * 0.1;
    qreal lineRight = pageRect.width() * 0.9;
    qreal lineWidth = lineRight - lineLeft;

    for (qreal pos : {lineY_top, lineY_bottom}) {
        int segments = 26;
        qreal segWidth = lineWidth / segments;
        for (int i = 0; i < segments; ++i) {
            painter.fillRect(QRectF(lineLeft + i * segWidth, pos, segWidth, 4),
                             letterColor(i));
        }
    }

    // Title
    QFont titleFont("Georgia", 36, QFont::Bold);
    painter.setFont(titleFont);
    painter.setPen(Qt::black);
    QRectF titleRect(0, centerY - 80, pageRect.width(), 50);
    painter.drawText(titleRect, Qt::AlignCenter, "orVocApp");

    // Subtitle
    QFont subtitleFont("Georgia", 20);
    subtitleFont.setItalic(true);
    painter.setFont(subtitleFont);
    painter.setPen(QColor("#444444"));
    QRectF subtitleRect(0, centerY - 25, pageRect.width(), 35);
    painter.drawText(subtitleRect, Qt::AlignCenter, "Personal Dictionary");

    // Divider
    painter.setPen(QPen(QColor("#2d5a9e"), 2));
    painter.drawLine(QPointF(centerX - 40, centerY + 30),
                     QPointF(centerX + 40, centerY + 30));

    // Metadata
    QFont metaFont("Georgia", 12);
    painter.setFont(metaFont);
    painter.setPen(QColor("#666666"));
    QRectF wordsRect(0, centerY + 50, pageRect.width(), 25);
    painter.drawText(wordsRect, Qt::AlignCenter,
                     QString("Words: %1").arg(wordCount));
    QRectF dateRect(0, centerY + 75, pageRect.width(), 25);
    painter.drawText(dateRect, Qt::AlignCenter,
                     QString("Generated: %1").arg(QDate::currentDate().toString("MMMM d, yyyy")));
}

void PdfExporter::drawPageHeader(QPainter &painter, QChar letter, int letterIndex, const QRectF &pageRect)
{
    // "orVocApp Dictionary" left-aligned
    QFont headerFont("Georgia", 9);
    painter.setFont(headerFont);
    painter.setPen(QColor("#555555"));
    painter.drawText(QRectF(0, 0, pageRect.width() / 2, kHeaderHeight),
                     Qt::AlignLeft | Qt::AlignVCenter, "orVocApp Dictionary");

    // Colored letter tab top-right
    QColor tabColor = letterColor(letterIndex);
    qreal tabSize = 32;
    qreal tabX = pageRect.width() - tabSize - 4;
    qreal tabY = 4;
    painter.setBrush(tabColor);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(QRectF(tabX, tabY, tabSize, tabSize), 4, 4);

    QFont letterFont("Georgia", 16, QFont::Bold);
    painter.setFont(letterFont);
    painter.setPen(Qt::white);
    painter.drawText(QRectF(tabX, tabY, tabSize, tabSize),
                     Qt::AlignCenter, letter);

    // Header underline
    painter.setPen(QPen(QColor("#111111"), 2));
    painter.drawLine(QPointF(0, kHeaderHeight - 2),
                     QPointF(pageRect.width(), kHeaderHeight - 2));
}

void PdfExporter::drawPageFooter(QPainter &painter, int pageNumber, const QRectF &pageRect)
{
    qreal footerTop = pageRect.height() - kFooterHeight;

    painter.setPen(QPen(QColor("#cccccc"), 1));
    painter.drawLine(QPointF(0, footerTop + 4),
                     QPointF(pageRect.width(), footerTop + 4));

    QFont footerFont("Georgia", 8);
    painter.setFont(footerFont);
    painter.setPen(QColor("#888888"));
    painter.drawText(QRectF(0, footerTop + 8, pageRect.width() / 2, 20),
                     Qt::AlignLeft | Qt::AlignVCenter, "Generated by orVocApp");
    painter.drawText(QRectF(pageRect.width() / 2, footerTop + 8, pageRect.width() / 2, 20),
                     Qt::AlignRight | Qt::AlignVCenter, QString("Page %1").arg(pageNumber));
}

qreal PdfExporter::measureWordEntry(const WordEntry &entry, const QRectF &pageRect)
{
    qreal contentWidth = pageRect.width();

    QTextDocument doc;
    doc.setTextWidth(contentWidth);

    QString html = QString(
        "<div style='font-family: Georgia, serif;'>"
        "<p style='font-size: 18px; font-weight: bold; margin: 0;'>%1</p>"
        "<p style='font-size: 11px; color: #666; margin: 2px 0 8px 0;'>%2</p>"
        "%3"
        "<div style='margin-top: 8px; padding: 6px 10px; background: #f8f8f8; "
        "border: 1px solid #ddd; border-radius: 4px; text-align: right; "
        "direction: rtl; font-size: 15px;'>%4</div>"
        "</div>")
        .arg(entry.word.toHtmlEscaped(),
             entry.phonetic.toHtmlEscaped(),
             entry.definitionHtml,
             entry.translationHtml);

    doc.setHtml(html);
    return doc.size().height() + kEntrySpacing;
}

qreal PdfExporter::drawWordEntry(QPainter &painter, const WordEntry &entry, qreal yPos, const QRectF &pageRect)
{
    qreal contentWidth = pageRect.width();

    QTextDocument doc;
    doc.setTextWidth(contentWidth);

    QString html = QString(
        "<div style='font-family: Georgia, serif;'>"
        "<p style='font-size: 18px; font-weight: bold; margin: 0;'>%1</p>"
        "<p style='font-size: 11px; color: #666; margin: 2px 0 8px 0;'>%2</p>"
        "%3"
        "<div style='margin-top: 8px; padding: 6px 10px; background: #f8f8f8; "
        "border: 1px solid #ddd; border-radius: 4px; text-align: right; "
        "direction: rtl; font-size: 15px;'>%4</div>"
        "</div>")
        .arg(entry.word.toHtmlEscaped(),
             entry.phonetic.toHtmlEscaped(),
             entry.definitionHtml,
             entry.translationHtml);

    doc.setHtml(html);

    painter.save();
    painter.translate(0, yPos);
    doc.drawContents(&painter);
    painter.restore();

    return yPos + doc.size().height();
}
```

- [ ] **Step 2: Commit**

```bash
git add orVocab/pdfexporter.cpp
git commit -m "feat(RVC-31): ADDED: PdfExporter implementation with cover page, letter tabs, and entry rendering"
```

---

### Task 6: ExportOverlay.qml — Progress bar with sequenced fade animations

**Files:**
- Create: `orVocab/ExportOverlay.qml`

- [ ] **Step 1: Create ExportOverlay.qml**

```qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: overlay
    anchors.right: parent.right
    anchors.bottom: parent.bottom
    anchors.margins: 16
    width: 280
    height: progressColumn.implicitHeight + 16

    property int current: 0
    property int total: 0
    property bool exporting: false

    signal exportComplete(bool success)

    function startExport(wordCount) {
        current = 0;
        total = wordCount;
        exporting = true;
        progressColumn.opacity = 1;
        notificationLabel.opacity = 0;
        notificationLabel.text = "";
    }

    function updateProgress(cur, tot) {
        current = cur;
        total = tot;
    }

    function showResult(success) {
        exporting = false;
        fadeOutProgress.start();
        if (success) {
            notificationLabel.text = "PDF exported successfully";
            notificationLabel.color = "#27ae60";
        } else {
            notificationLabel.text = "Export failed";
            notificationLabel.color = "#e74c3c";
        }
    }

    Column {
        id: progressColumn
        anchors.fill: parent
        anchors.margins: 8
        spacing: 4
        visible: opacity > 0

        Behavior on opacity {
            enabled: false
        }

        Label {
            text: "Exporting... " + overlay.current + "/" + overlay.total + " words"
            font.pixelSize: 12
            visible: overlay.exporting
        }

        ProgressBar {
            width: parent.width
            from: 0
            to: overlay.total
            value: overlay.current
            visible: overlay.exporting
        }
    }

    NumberAnimation {
        id: fadeOutProgress
        target: progressColumn
        property: "opacity"
        from: 1
        to: 0
        duration: 300
        onFinished: fadeInNotification.start()
    }

    Label {
        id: notificationLabel
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 8
        font.pixelSize: 13
        font.bold: true
        opacity: 0
        visible: opacity > 0

        Behavior on opacity {
            enabled: false
        }
    }

    NumberAnimation {
        id: fadeInNotification
        target: notificationLabel
        property: "opacity"
        from: 0
        to: 1
        duration: 300
        onFinished: holdTimer.start()
    }

    Timer {
        id: holdTimer
        interval: 4000
        onTriggered: fadeOutNotification.start()
    }

    NumberAnimation {
        id: fadeOutNotification
        target: notificationLabel
        property: "opacity"
        from: 1
        to: 0
        duration: 300
    }
}
```

- [ ] **Step 2: Commit**

```bash
git add orVocab/ExportOverlay.qml
git commit -m "feat(RVC-31): ADDED: ExportOverlay QML component with sequenced fade animations"
```

---

### Task 7: Main.qml — Export PDF button, page size dialog, file dialog, and overlay integration

**Files:**
- Modify: `orVocab/Main.qml:1-135`

- [ ] **Step 1: Add PdfExporter import and Export PDF button to toolbar**

In `Main.qml`, add a new `Button` inside the `RowLayout` in the `header: ToolBar` block, after the "Export Text" button (line 54):

```qml
Button {
    text: "Export PDF"
    onClicked: pageSizeDialog.open()
}
```

- [ ] **Step 2: Add the page size selection dialog**

Add after the `confirmImportDialog` closing brace (after line 134), before the closing `}` of `ApplicationWindow`:

```qml
Dialog {
    id: pageSizeDialog
    title: "Select Page Size"
    modal: true
    anchors.centerIn: parent
    standardButtons: Dialog.Cancel

    RowLayout {
        spacing: 12

        Button {
            text: "A4"
            onClicked: {
                pageSizeDialog.close();
                root.selectedPageSize = 0;
                exportPdfDialog.open();
            }
        }

        Button {
            text: "Letter"
            onClicked: {
                pageSizeDialog.close();
                root.selectedPageSize = 1;
                exportPdfDialog.open();
            }
        }
    }
}
```

- [ ] **Step 3: Add the selectedPageSize property and PDF file dialog**

Add `property int selectedPageSize: 0` to the `ApplicationWindow` properties (near `property string currentWord: ""` on line 14).

Add the PDF file dialog after `pageSizeDialog`:

```qml
Platform.FileDialog {
    id: exportPdfDialog
    title: "Export PDF Dictionary"
    fileMode: Platform.FileDialog.SaveFile
    defaultSuffix: "pdf"
    nameFilters: ["PDF files (*.pdf)"]
    onAccepted: {
        var exporter = VocabManager.createPdfExporter(root.selectedPageSize);
        exportOverlay.startExport(VocabManager.words.length);
        exporter.progress.connect(exportOverlay.updateProgress);
        exporter.finished.connect(function(success, path) {
            exportOverlay.showResult(success);
        });
        exporter.exportToFile(exportPdfDialog.file.toString().replace("file://", ""));
    }
}
```

- [ ] **Step 4: Add the ExportOverlay instance**

Add inside `ApplicationWindow`, after the `SplitView` closing brace (after line 84):

```qml
ExportOverlay {
    id: exportOverlay
}
```

- [ ] **Step 5: Add factory method to VocabManager**

This step requires a small addition to `VocabManager` to create and own the `PdfExporter` instance from QML. Add to `vocabmanager.h` in the public section:

```cpp
Q_INVOKABLE QObject* createPdfExporter(int pageSize);
```

Add to `vocabmanager.cpp`:

```cpp
#include "pdfexporter.h"

QObject* VocabManager::createPdfExporter(int pageSize)
{
    auto sizeId = (pageSize == 0) ? QPageSize::A4 : QPageSize::Letter;
    auto *exporter = new PdfExporter(words(), sizeId);
    return exporter;
}
```

- [ ] **Step 6: Commit**

```bash
git add orVocab/Main.qml orVocab/vocabmanager.h orVocab/vocabmanager.cpp
git commit -m "feat(RVC-31): ADDED: Export PDF button, page size dialog, file dialog, and overlay integration"
```

---

### Task 8: Tests — Update test CMakeLists and add PdfExporter tests

**Files:**
- Modify: `tests/CMakeLists.txt`
- Create: `tests/test_pdfexporter.cpp`

- [ ] **Step 1: Update tests/CMakeLists.txt**

Replace the full file with:

```cmake
find_package(Qt6 REQUIRED COMPONENTS Core Network PrintSupport)
find_package(Catch2 3 REQUIRED)

add_executable(orVocabTests
    test_vocabmanager.cpp
    test_networkclient.cpp
    test_pdfexporter.cpp
    ${CMAKE_SOURCE_DIR}/orVocab/vocabmanager.cpp
    ${CMAKE_SOURCE_DIR}/orVocab/networkclient.cpp
    ${CMAKE_SOURCE_DIR}/orVocab/baseexporter.cpp
    ${CMAKE_SOURCE_DIR}/orVocab/pdfexporter.cpp
)

set_target_properties(orVocabTests PROPERTIES AUTOMOC ON)

target_include_directories(orVocabTests PRIVATE ${CMAKE_SOURCE_DIR}/orVocab)

target_link_libraries(orVocabTests
    PRIVATE
        Catch2::Catch2WithMain
        Qt6::Core
        Qt6::Network
        Qt6::PrintSupport
)

include(CTest)
include(Catch)
catch_discover_tests(orVocabTests)
```

- [ ] **Step 2: Write test_pdfexporter.cpp**

```cpp
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
```

- [ ] **Step 3: Build and run tests**

Run:
```bash
cd build/Desktop_Qt_6_10_2-Debug && cmake --build . && ctest --output-on-failure
```

Expected: All tests pass, including the new pdfexporter tests.

- [ ] **Step 4: Commit**

```bash
git add tests/CMakeLists.txt tests/test_pdfexporter.cpp
git commit -m "test(RVC-31): ADDED: PdfExporter unit tests for letter colors, single entry, and multi-group rendering"
```

---

### Task 9: Build verification and manual testing

**Files:** None (verification only)

- [ ] **Step 1: Build the full project**

```bash
cd build/Desktop_Qt_6_10_2-Debug && cmake --build .
```

Expected: Clean build with no errors or warnings.

- [ ] **Step 2: Run the application**

```bash
./build/Desktop_Qt_6_10_2-Debug/orVocab/orVocApp
```

- [ ] **Step 3: Manual test — Export PDF flow**

1. Add a few words to the word bank (e.g., "hello", "world", "abandon", "book")
2. Click each word to verify definitions load
3. Click "Export PDF" in the toolbar
4. Select "A4" in the page size dialog
5. Choose a save location and filename
6. Verify the progress bar appears at bottom-right with word count
7. Verify the progress bar fades out and "PDF exported successfully" notification appears
8. Verify the notification fades out after ~4 seconds
9. Open the PDF and verify:
   - Cover page with "orVocApp", "Personal Dictionary", word count, date, rainbow lines
   - Letter sections with colored tabs in top-right corner
   - Each entry has word, phonetic, definitions, Arabic translation
   - Page headers and footers on every page
   - White background, printer-friendly layout

- [ ] **Step 4: Repeat with Letter page size**

Repeat step 3 but select "Letter" to verify both page sizes work.

- [ ] **Step 5: Run all tests one final time**

```bash
cd build/Desktop_Qt_6_10_2-Debug && ctest --output-on-failure
```

Expected: All tests pass.

- [ ] **Step 6: Final commit if any adjustments were needed**

```bash
git add -A
git commit -m "fix(RVC-31): FIXED: adjustments from manual testing"
```
