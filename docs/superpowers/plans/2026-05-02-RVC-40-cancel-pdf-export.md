# RVC-40 — Cancellable PDF Export Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let the user cancel a PDF export mid-run from a round red Cancel button in the toolbar, with no partial PDF left on disk.

**Architecture:** Add a `std::atomic<bool>` cancel flag and a `QPointer<QNetworkReply>` to `BaseExporter`, plus `requestCancel()` slot and `cancelled()` signal. Fetch loop and render loop both poll the flag; fetch aborts the in-flight reply, render deletes the partial file. QML toolbar overlay gains a Cancel button wired to the active exporter.

**Tech Stack:** Qt 6.10 C++17 (QObject, QNetworkAccessManager, QPointer, QThread, QPrinter, QPainter, std::atomic), QML (Qt Quick Controls), Catch2 v3.

**Reference spec:** `docs/superpowers/specs/2026-05-02-RVC-40-cancel-pdf-export-design.md`

**Build paths:**
- Debug build dir: `/mnt/BA707A64707A2773/code/orVocApp/build/Desktop_Qt_6_10_2-Debug` (Qt Creator shadow build — per project rule).
- Build command: `cmake --build /mnt/BA707A64707A2773/code/orVocApp/build/Desktop_Qt_6_10_2-Debug`
- Test command: `ctest --output-on-failure --test-dir /mnt/BA707A64707A2773/code/orVocApp/build/Desktop_Qt_6_10_2-Debug`
- Run app: `/mnt/BA707A64707A2773/code/orVocApp/build/Desktop_Qt_6_10_2-Debug/orVocApp/orVocApp`

**Commit convention:** `type(RVC-40): VERB: description` per `orfi-git-conventions`.

---

## File Structure

Files created:
- `tests/test_baseexporter_cancel.cpp` — new Catch2 test for BaseExporter cancel-before-fetch behaviour (uses a stub subclass to avoid hitting the network).

Files modified:
- `orVocApp/baseexporter.h` — add `m_cancelled`, `m_currentReply`, `requestCancel()`, `cancelled()`, reply-held member.
- `orVocApp/baseexporter.cpp` — implement `requestCancel`, gate fetch loop and reply callbacks, branch between `cancelled()` / `finished()` after render.
- `orVocApp/pdfexporter.h` — widen `renderToFile` to read the cancel flag (already has access via base class — no signature change).
- `orVocApp/pdfexporter.cpp` — add page-boundary cancel checks, delete partial file on cancel.
- `orVocApp/NotificationOverlay.qml` — add `activeExporter`, `setActiveExporter`, `showCancelled`, Cancel button, reset-enabled-on-exporting-change.
- `orVocApp/Main.qml` — wire `exporter.cancelled` → `notificationOverlay.showCancelled`, call `setActiveExporter`.
- `tests/test_pdfexporter.cpp` — add cancel-during-render test case.
- `tests/CMakeLists.txt` — add `test_baseexporter_cancel.cpp` to the test target sources.

---

## Task 1: Extend BaseExporter header with cancel API

**Files:**
- Modify: `orVocApp/baseexporter.h`

- [ ] **Step 1: Add atomic, QPointer, and new slot+signal to `BaseExporter`**

Replace the class body of `BaseExporter` in `orVocApp/baseexporter.h` with:

```cpp
class BaseExporter : public QObject
{
    Q_OBJECT

public:
    explicit BaseExporter(const QStringList &words, QObject *parent = nullptr);
    ~BaseExporter() override;

    Q_INVOKABLE void exportToFile(const QString &outputPath);
    Q_INVOKABLE void requestCancel();

    bool isCancelled() const { return m_cancelled.load(std::memory_order_acquire); }

signals:
    void progress(int current, int total);
    void finished(bool success, const QString &filePath);
    void cancelled();

protected:
    virtual bool renderToFile(const QVector<WordEntry> &entries, const QString &outputPath) = 0;

private:
    void fetchNextWord();
    void onDefinitionReply(int index, QNetworkReply *reply);
    void onTranslationReply(int index, QNetworkReply *reply);
    void startRender();

    QStringList m_words;
    QString m_outputPath;
    QNetworkAccessManager *m_nam = nullptr;
    QVector<WordEntry> m_entries;
    int m_currentIndex = 0;
    std::atomic<bool> m_cancelled{false};
    QPointer<QNetworkReply> m_currentReply;
};
```

Also add these includes at the top of `baseexporter.h` (below `#include <QVector>`):

```cpp
#include <QPointer>
#include <atomic>
```

And forward-declare `QNetworkReply` near the other forward declarations (it already exists — confirm it's there; if not, add `class QNetworkReply;`).

- [ ] **Step 2: Build and verify existing behaviour is unaffected**

Run:
```bash
cmake --build /mnt/BA707A64707A2773/code/orVocApp/build/Desktop_Qt_6_10_2-Debug 2>&1 | tail -10
```
Expected: clean build, no new warnings on the modified header. The unused `m_cancelled` / `m_currentReply` / `requestCancel` are fine — fleshed out in Task 2.

- [ ] **Step 3: Run existing tests to confirm no regression**

Run:
```bash
ctest --output-on-failure --test-dir /mnt/BA707A64707A2773/code/orVocApp/build/Desktop_Qt_6_10_2-Debug
```
Expected: all 11 tests pass (VocabManager, NetworkClient, WordEntry, PdfExporter).

- [ ] **Step 4: Commit**

```bash
git add orVocApp/baseexporter.h
git commit -m "feat(RVC-40): ADDED: cancel API scaffolding on BaseExporter header"
```

---

## Task 2: Implement requestCancel and track in-flight reply

**Files:**
- Modify: `orVocApp/baseexporter.cpp`

- [ ] **Step 1: Store `m_currentReply` when issuing the definition fetch**

In `orVocApp/baseexporter.cpp`, replace `BaseExporter::fetchNextWord` with:

```cpp
void BaseExporter::fetchNextWord()
{
    if (m_cancelled.load(std::memory_order_acquire)) {
        emit cancelled();
        return;
    }

    if (m_currentIndex >= m_words.size()) {
        startRender();
        return;
    }

    const QString &word = m_words[m_currentIndex];

    QUrl defUrl("https://api.dictionaryapi.dev/api/v2/entries/en/" + word);
    QNetworkReply *reply = m_nam->get(QNetworkRequest(defUrl));
    m_currentReply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        onDefinitionReply(m_currentIndex, reply);
    });
}
```

- [ ] **Step 2: Gate `onDefinitionReply` on the cancel flag and track the translation reply**

Replace `BaseExporter::onDefinitionReply` with:

```cpp
void BaseExporter::onDefinitionReply(int index, QNetworkReply *defReply)
{
    if (m_cancelled.load(std::memory_order_acquire)) {
        emit cancelled();
        return;
    }

    WordEntry &entry = m_entries[index];

    if (defReply->error() == QNetworkReply::NoError) {
        auto result = NetworkClient::parseDictionaryResponse(defReply->readAll());
        if (!result.error) {
            entry.definitionHtml = result.html;
            entry.phonetic = result.phonetic;

            QUrl transUrl("https://translate.googleapis.com/translate_a/single");
            QUrlQuery query;
            query.addQueryItem("client", "gtx");
            query.addQueryItem("sl", "en");
            query.addQueryItem("tl", "ar");
            query.addQueryItem("dt", "t");
            query.addQueryItem("q", entry.word);
            transUrl.setQuery(query);

            QNetworkReply *transReply = m_nam->get(QNetworkRequest(transUrl));
            m_currentReply = transReply;
            connect(transReply, &QNetworkReply::finished, this, [this, transReply]() {
                transReply->deleteLater();
                onTranslationReply(m_currentIndex, transReply);
            });
            return;
        }
    }

    emit progress(m_currentIndex + 1, m_words.size());
    m_currentIndex++;
    QTimer::singleShot(150, this, &BaseExporter::fetchNextWord);
}
```

- [ ] **Step 3: Gate `onTranslationReply` on the cancel flag**

Replace `BaseExporter::onTranslationReply` with:

```cpp
void BaseExporter::onTranslationReply(int index, QNetworkReply *transReply)
{
    if (m_cancelled.load(std::memory_order_acquire)) {
        emit cancelled();
        return;
    }

    WordEntry &entry = m_entries[index];

    if (transReply->error() == QNetworkReply::NoError) {
        auto result = NetworkClient::parseTranslationResponse(transReply->readAll());
        if (!result.error) {
            entry.translationHtml = result.html;
            entry.valid = true;
        }
    }

    emit progress(m_currentIndex + 1, m_words.size());
    m_currentIndex++;
    QTimer::singleShot(150, this, &BaseExporter::fetchNextWord);
}
```

- [ ] **Step 4: Implement `requestCancel`**

Append at the bottom of `orVocApp/baseexporter.cpp`:

```cpp
void BaseExporter::requestCancel()
{
    m_cancelled.store(true, std::memory_order_release);
    if (m_currentReply)
        m_currentReply->abort();
}
```

- [ ] **Step 5: Build**

Run:
```bash
cmake --build /mnt/BA707A64707A2773/code/orVocApp/build/Desktop_Qt_6_10_2-Debug 2>&1 | tail -10
```
Expected: clean build.

- [ ] **Step 6: Run existing tests**

Run:
```bash
ctest --output-on-failure --test-dir /mnt/BA707A64707A2773/code/orVocApp/build/Desktop_Qt_6_10_2-Debug
```
Expected: 11/11 pass.

- [ ] **Step 7: Commit**

```bash
git add orVocApp/baseexporter.cpp
git commit -m "feat(RVC-40): ADDED: cancel checks in fetch phase and requestCancel implementation"
```

---

## Task 3: Render-phase cancel — PdfExporter page-boundary checks and partial-file cleanup

**Files:**
- Modify: `orVocApp/pdfexporter.cpp`

- [ ] **Step 1: Add cancel checks at page boundaries and clean up partial PDF on cancel**

Replace `PdfExporter::renderToFile` in `orVocApp/pdfexporter.cpp` with:

```cpp
bool PdfExporter::renderToFile(const QVector<WordEntry> &entries, const QString &outputPath)
{
    QPrinter printer(QPrinter::ScreenResolution);
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
    bool cancelledMidRender = false;

    for (auto it = letterGroups.constBegin(); it != letterGroups.constEnd() && !cancelledMidRender; ++it) {
        QChar letter = it.key();
        int letterIndex = letter.unicode() - QChar('A').unicode();
        const auto &words = it.value();

        printer.newPage();
        pageNumber++;

        if (isCancelled()) {
            cancelledMidRender = true;
            break;
        }

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

                if (isCancelled()) {
                    cancelledMidRender = true;
                    break;
                }

                drawPageHeader(painter, letter, letterIndex, pageRect);
                drawPageFooter(painter, pageNumber, pageRect);
                yPos = contentTop;
            }

            yPos = drawWordEntry(painter, *entry, yPos, pageRect);
            yPos += kEntrySpacing;
        }
    }

    painter.end();

    if (cancelledMidRender || isCancelled()) {
        QFile::remove(outputPath);
        return false;
    }

    return true;
}
```

Also add `#include <QFile>` to the top of `orVocApp/pdfexporter.cpp` if not already present.

- [ ] **Step 2: Build**

Run:
```bash
cmake --build /mnt/BA707A64707A2773/code/orVocApp/build/Desktop_Qt_6_10_2-Debug 2>&1 | tail -10
```
Expected: clean build.

- [ ] **Step 3: Run existing tests (success paths must still pass)**

Run:
```bash
ctest --output-on-failure --test-dir /mnt/BA707A64707A2773/code/orVocApp/build/Desktop_Qt_6_10_2-Debug
```
Expected: 11/11 pass — the three existing PdfExporter tests still render successfully (flag defaults to false).

- [ ] **Step 4: Commit**

```bash
git add orVocApp/pdfexporter.cpp
git commit -m "feat(RVC-40): ADDED: cancel checks in PdfExporter render loop with partial-file cleanup"
```

---

## Task 4: Branch between cancelled() and finished() in the render worker

**Files:**
- Modify: `orVocApp/baseexporter.cpp`

- [ ] **Step 1: Inspect the cancel flag after `renderToFile` returns; also bail early if fetch produced no entries because user cancelled**

Replace `BaseExporter::startRender` with:

```cpp
void BaseExporter::startRender()
{
    if (m_cancelled.load(std::memory_order_acquire)) {
        emit cancelled();
        return;
    }

    QVector<WordEntry> validEntries;
    for (const auto &e : m_entries) {
        if (e.valid)
            validEntries.append(e);
    }

    if (validEntries.isEmpty()) {
        emit finished(false, m_outputPath);
        return;
    }

    QThread *thread = QThread::create([this, validEntries]() {
        bool success = renderToFile(validEntries, m_outputPath);
        if (m_cancelled.load(std::memory_order_acquire)) {
            emit cancelled();
        } else {
            emit finished(success, m_outputPath);
        }
    });
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}
```

- [ ] **Step 2: Build**

Run:
```bash
cmake --build /mnt/BA707A64707A2773/code/orVocApp/build/Desktop_Qt_6_10_2-Debug 2>&1 | tail -10
```
Expected: clean build.

- [ ] **Step 3: Run existing tests**

Run:
```bash
ctest --output-on-failure --test-dir /mnt/BA707A64707A2773/code/orVocApp/build/Desktop_Qt_6_10_2-Debug
```
Expected: 11/11 pass.

- [ ] **Step 4: Commit**

```bash
git add orVocApp/baseexporter.cpp
git commit -m "feat(RVC-40): ADDED: cancelled signal emission path in render worker"
```

---

## Task 5: NotificationOverlay — Cancel button, active exporter handle, showCancelled

**Files:**
- Modify: `orVocApp/NotificationOverlay.qml`

- [ ] **Step 1: Replace `NotificationOverlay.qml` with the Cancel-aware version**

Replace the entire contents of `orVocApp/NotificationOverlay.qml` with:

```qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: overlay
    Layout.preferredWidth: visible ? 220 : 0
    Layout.fillHeight: true
    visible: exporting || notificationLabel.opacity > 0

    property int current: 0
    property int total: 0
    property bool exporting: false
    property var activeExporter: null

    function startExport(wordCount) {
        current = 0;
        total = wordCount;
        exporting = true;
        notificationLabel.opacity = 0;
        notificationLabel.text = "";
        cancelButton.enabled = true;
    }

    function setActiveExporter(e) {
        activeExporter = e;
    }

    function updateProgress(cur, tot) {
        current = cur;
        total = tot;
    }

    function showNotification(text, isError) {
        fadeOutNotification.stop();
        holdTimer.stop();
        notificationLabel.text = text;
        notificationLabel.color = isError ? "#e74c3c" : "#27ae60";
        fadeInNotification.start();
    }

    function showResult(success) {
        exporting = false;
        activeExporter = null;
        showNotification(
            success ? "PDF exported successfully" : "Export failed",
            !success
        );
    }

    function showCancelled() {
        exporting = false;
        activeExporter = null;
        showNotification("Export cancelled", false);
    }

    RowLayout {
        anchors.fill: parent
        spacing: 6
        visible: overlay.exporting

        Label {
            text: overlay.current + "/" + overlay.total
            font.pixelSize: 11
        }

        ProgressBar {
            Layout.fillWidth: true
            Layout.preferredHeight: 14
            from: 0
            to: overlay.total
            value: overlay.current
        }

        Button {
            id: cancelButton
            implicitWidth: height
            implicitHeight: 24
            padding: 0
            hoverEnabled: true
            ToolTip.visible: hovered
            ToolTip.delay: 500
            ToolTip.text: "Cancel export"

            background: Rectangle {
                radius: width / 2
                color: cancelButton.enabled
                       ? (cancelButton.down
                          ? Qt.darker("#e74c3c", 1.2)
                          : (cancelButton.hovered
                             ? Qt.lighter("#e74c3c", 1.15)
                             : "#e74c3c"))
                       : Qt.darker("#e74c3c", 1.6)
                border.color: Qt.darker("#e74c3c", 1.4)
                border.width: 1
            }

            contentItem: Item {}

            onClicked: {
                cancelButton.enabled = false;
                if (overlay.activeExporter)
                    overlay.activeExporter.requestCancel();
            }
        }
    }

    Label {
        id: notificationLabel
        anchors.centerIn: parent
        font.pixelSize: 12
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
        interval: 3000
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

- [ ] **Step 2: Build**

Run:
```bash
cmake --build /mnt/BA707A64707A2773/code/orVocApp/build/Desktop_Qt_6_10_2-Debug 2>&1 | tail -10
```
Expected: clean build (includes QML cache regeneration for NotificationOverlay).

- [ ] **Step 3: Commit**

```bash
git add orVocApp/NotificationOverlay.qml
git commit -m "feat(RVC-40): ADDED: Cancel button and showCancelled helper in NotificationOverlay"
```

---

## Task 6: Main.qml — wire exporter.cancelled and setActiveExporter

**Files:**
- Modify: `orVocApp/Main.qml`

- [ ] **Step 1: Extend the PDF export dialog handler**

In `orVocApp/Main.qml`, find `Platform.FileDialog { id: exportPdfDialog ... onAccepted: { ... } }` (around line 228). Replace just the `onAccepted` block with:

```qml
        onAccepted: {
            var exporter = VocabManager.createPdfExporter(root.selectedPageSize);
            notificationOverlay.startExport(VocabManager.words.length);
            notificationOverlay.setActiveExporter(exporter);
            exporter.progress.connect(notificationOverlay.updateProgress);
            exporter.finished.connect(function(success, path) {
                notificationOverlay.showResult(success);
            });
            exporter.cancelled.connect(function() {
                notificationOverlay.showCancelled();
            });
            exporter.exportToFile(exportPdfDialog.file.toString().replace("file://", ""));
        }
```

- [ ] **Step 2: Build**

Run:
```bash
cmake --build /mnt/BA707A64707A2773/code/orVocApp/build/Desktop_Qt_6_10_2-Debug 2>&1 | tail -10
```
Expected: clean build.

- [ ] **Step 3: Commit**

```bash
git add orVocApp/Main.qml
git commit -m "feat(RVC-40): ADDED: cancelled signal wiring and active-exporter handoff in Main"
```

---

## Task 7: Catch2 test — BaseExporter cancel before first fetch

**Files:**
- Create: `tests/test_baseexporter_cancel.cpp`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write the new test file**

Create `tests/test_baseexporter_cancel.cpp` with:

```cpp
#include <catch2/catch_all.hpp>

#include "baseexporter.h"

#include <QCoreApplication>
#include <QSignalSpy>
#include <QTimer>

static int argc = 1;
static char appName[] = "test";
static char *argv[] = {appName, nullptr};

namespace {
class StubExporter : public BaseExporter
{
public:
    explicit StubExporter(const QStringList &words) : BaseExporter(words) {}

protected:
    bool renderToFile(const QVector<WordEntry> &, const QString &) override
    {
        return true;
    }
};
}

TEST_CASE("BaseExporter cancel before first fetch emits cancelled without finished",
          "[baseexporter][cancel]")
{
    QCoreApplication app(argc, argv);

    StubExporter exporter({"apple", "banana"});
    QSignalSpy cancelledSpy(&exporter, &BaseExporter::cancelled);
    QSignalSpy finishedSpy(&exporter, &BaseExporter::finished);

    exporter.exportToFile("/tmp/unused-cancel-test.pdf");
    exporter.requestCancel();

    // Let the pending fetchNextWord continuation run and observe the cancel flag.
    QTimer::singleShot(0, &app, [&]() { app.quit(); });
    app.exec();

    REQUIRE(cancelledSpy.count() == 1);
    REQUIRE(finishedSpy.count() == 0);
}
```

- [ ] **Step 2: Register the new source in the tests CMake**

In `tests/CMakeLists.txt`, find the `add_executable(orVocAppTests ...)` call and add `test_baseexporter_cancel.cpp` to its source list. The full call should become:

```cmake
add_executable(orVocAppTests
    test_vocabmanager.cpp
    test_networkclient.cpp
    test_pdfexporter.cpp
    test_baseexporter_cancel.cpp
    ${CMAKE_SOURCE_DIR}/orVocApp/vocabmanager.cpp
    ${CMAKE_SOURCE_DIR}/orVocApp/networkclient.cpp
    ${CMAKE_SOURCE_DIR}/orVocApp/baseexporter.cpp
    ${CMAKE_SOURCE_DIR}/orVocApp/pdfexporter.cpp
)
```

- [ ] **Step 3: Reconfigure CMake so the new file is picked up**

Run:
```bash
cmake -B /mnt/BA707A64707A2773/code/orVocApp/build/Desktop_Qt_6_10_2-Debug -S /mnt/BA707A64707A2773/code/orVocApp 2>&1 | tail -5
```
Expected: `-- Generating done` / `-- Build files have been written`.

- [ ] **Step 4: Build and run the new test**

Run:
```bash
cmake --build /mnt/BA707A64707A2773/code/orVocApp/build/Desktop_Qt_6_10_2-Debug 2>&1 | tail -10
ctest --output-on-failure --test-dir /mnt/BA707A64707A2773/code/orVocApp/build/Desktop_Qt_6_10_2-Debug -R "cancel before first fetch"
```
Expected: 1/1 pass. `cancelled` count = 1, `finished` count = 0.

- [ ] **Step 5: Full test suite**

Run:
```bash
ctest --output-on-failure --test-dir /mnt/BA707A64707A2773/code/orVocApp/build/Desktop_Qt_6_10_2-Debug
```
Expected: 12/12 pass (was 11, now +1).

- [ ] **Step 6: Commit**

```bash
git add tests/test_baseexporter_cancel.cpp tests/CMakeLists.txt
git commit -m "test(RVC-40): ADDED: cancel-before-fetch test for BaseExporter"
```

---

## Task 8: Catch2 test — PdfExporter cancel during render removes partial file

**Files:**
- Modify: `tests/test_pdfexporter.cpp`

- [ ] **Step 1: Append the cancel-during-render test**

At the bottom of `tests/test_pdfexporter.cpp`, add:

```cpp
TEST_CASE("PdfExporter cancel before render returns false and leaves no file", "[pdfexporter][cancel]") {
    QGuiApplication app(argc, argv);

    QTemporaryDir tmpDir;
    REQUIRE(tmpDir.isValid());
    QString pdfPath = tmpDir.path() + "/cancelled.pdf";

    QVector<WordEntry> entries;
    for (char c = 'a'; c <= 'z'; ++c) {
        WordEntry e;
        e.word = QString(QChar::fromLatin1(c)) + "word";
        e.phonetic = "/x/";
        e.definitionHtml = "<p>def</p>";
        e.translationHtml = "<p dir=\"rtl\">ترجمة</p>";
        e.valid = true;
        entries.append(e);
    }

    PdfExporter exporter(QStringList(), QPageSize::A4);
    exporter.requestCancel();

    bool result = exporter.renderToFile(entries, pdfPath);

    REQUIRE(result == false);
    REQUIRE_FALSE(QFile::exists(pdfPath));
}
```

Ensure `#include <QFile>` is present at the top of the file (Qt usually pulls it transitively, but add it explicitly if the build fails).

- [ ] **Step 2: Build**

Run:
```bash
cmake --build /mnt/BA707A64707A2773/code/orVocApp/build/Desktop_Qt_6_10_2-Debug 2>&1 | tail -10
```
Expected: clean build.

- [ ] **Step 3: Run the new test only**

Run:
```bash
ctest --output-on-failure --test-dir /mnt/BA707A64707A2773/code/orVocApp/build/Desktop_Qt_6_10_2-Debug -R "cancel before render"
```
Expected: 1/1 pass.

- [ ] **Step 4: Full test suite**

Run:
```bash
ctest --output-on-failure --test-dir /mnt/BA707A64707A2773/code/orVocApp/build/Desktop_Qt_6_10_2-Debug
```
Expected: 13/13 pass.

- [ ] **Step 5: Commit**

```bash
git add tests/test_pdfexporter.cpp
git commit -m "test(RVC-40): ADDED: cancel-before-render test for PdfExporter"
```

---

## Task 9: Manual smoke test and final push

- [ ] **Step 1: Launch Debug build**

Run:
```bash
/mnt/BA707A64707A2773/code/orVocApp/build/Desktop_Qt_6_10_2-Debug/orVocApp/orVocApp
```

- [ ] **Step 2: Verify five manual checks**

1. Start PDF export with at least 20 words; click Cancel mid-fetch → green "Export cancelled" notification, sidebar re-enabled, no PDF left on disk.
2. Start PDF export; let fetch complete, click Cancel mid-render → green notification, no PDF.
3. Let one export complete normally → green "PDF exported successfully", PDF exists.
4. Cancel twice in quick succession → no crash, single notification.
5. After each scenario, confirm the Cancel button disappears once `exporting` flips false.

- [ ] **Step 3: Close the app and push branch**

Run:
```bash
git push -u origin feat/RVC-40-cancel-pdf-export
```

- [ ] **Step 4: Open PR**

Run:
```bash
gh pr create --title "feat(RVC-40): Make PDF export worker thread cancellable mid-run" --body "$(cat <<'EOF'
## Summary
- Added `requestCancel()` + `cancelled()` on `BaseExporter`, observed by both the fetch loop and the `PdfExporter` render loop.
- Fetch phase aborts the in-flight `QNetworkReply`; render phase deletes the partial PDF on cancel.
- Added round red Cancel button in the toolbar `NotificationOverlay`, matching the pronunciation button's sizing.
- Green "Export cancelled" notification on cancel; red "Export failed" still reserved for system failures.

Ticket: [RVC-40](https://welorfi.atlassian.net/browse/RVC-40)
Spec: `docs/superpowers/specs/2026-05-02-RVC-40-cancel-pdf-export-design.md`

## Test plan
- [x] New Catch2: `BaseExporter cancel before first fetch emits cancelled without finished`
- [x] New Catch2: `PdfExporter cancel before render returns false and leaves no file`
- [x] Full suite: 13/13 pass
- [x] Manual: cancel mid-fetch, cancel mid-render, happy path, double-click cancel, button hides after export ends
EOF
)"
```

---

## Self-Review notes (for author)

- **Spec coverage:** fetch-phase cancel (Task 2), render-phase cancel + partial-file cleanup (Task 3), `cancelled` vs `finished` branching (Task 4), Cancel button UX + `showCancelled` (Task 5), Main.qml wiring (Task 6), both specified Catch2 tests (Tasks 7, 8), manual test plan (Task 9). All goals accounted for.
- **Placeholder scan:** no TBD/TODO, no vague "add validation" — all steps carry code.
- **Type/name consistency:** `requestCancel`, `cancelled`, `isCancelled()`, `showCancelled`, `setActiveExporter` are spelled identically across tasks.
- **Known caveat:** `QSignalSpy` on signals emitted from the worker thread (during render) needs the main-thread event loop to process queued emissions. Task 7 uses a stub that returns from the fetch path (no worker thread), so spying is safe there. Task 8 calls `renderToFile` synchronously on the main thread, bypassing the worker — also safe. No cross-thread spy test is in scope; manual test #2 validates the threaded path.
