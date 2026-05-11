# RVC-45: PDF Export Drops Words on Transient API Failures — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Eliminate silent word drops from PDF export by making the per-word fetch loop resilient to transient API failures, while preserving the existing PDF semantics (no layout/content changes) and existing cancel behaviour.

**Architecture:** The root cause is in `BaseExporter`: any non-success reply (network error, 404, parse fail, hung connection) immediately advances to the next word and leaves the entry `valid=false`, and the render phase then filters those out. The fix adds (1) a per-request `QNetworkRequest::setTransferTimeout(10000)` so hung connections fail fast, (2) an exponential-backoff retry loop (3 retries, 500/1500/4500 ms) around both the dictionary and translation fetch for each word, and (3) a hard-fail path: if retries for any single word are exhausted, the whole export ends via the existing `finished(false, outputPath)` signal. No PDF rendering, UI, or VocabManager code changes.

**Tech Stack:** C++17, Qt 6.10 (QtNetwork, QtCore), Catch2 v3.

**Branch:** `fix/RVC-45-pdf-export-drops-words` (already created).

**Build directory (Qt Creator shadow build, Debug):** `build/Desktop_Qt_6_10_2-Debug`. All `cmake --build` and `ctest` commands below target this path. Do **not** create a new build tree.

**Ticket:** https://welorfi.atlassian.net/jira/software/projects/RVC/boards/46?selectedIssue=RVC-45

---

## File Structure

**Files modified:**
- `orVocApp/baseexporter.h` — add retry constants, phase enum, retry state members, helper-method declarations, Doxygen on new members.
- `orVocApp/baseexporter.cpp` — rewrite fetch pipeline as `issueCurrentRequest` + `onReplyFinished` with retry/backoff; keep cancel semantics; keep 150 ms inter-word throttle.
- `tests/test_baseexporter_cancel.cpp` — add coverage for `retryBackoffMs` (pure helper — no network required).

**Files unchanged:** `pdfexporter.{h,cpp}`, `networkclient.{h,cpp}`, `vocabmanager.{h,cpp}`, any QML, CMakeLists.txt.

---

## Task 1: Add testable `retryBackoffMs` helper (TDD)

Extract the backoff schedule as a `static` pure function so we can unit-test it deterministically without touching the network. This is the only unit-testable slice of the fix — the retry-around-real-network behaviour is verified manually in Task 3.

**Files:**
- Modify: `orVocApp/baseexporter.h` (add public static declaration + constants)
- Modify: `orVocApp/baseexporter.cpp` (add definition)
- Modify: `tests/test_baseexporter_cancel.cpp` (add new TEST_CASE)

- [ ] **Step 1: Write the failing test**

Append this `TEST_CASE` to the end of `tests/test_baseexporter_cancel.cpp` (after the existing one, before EOF):

```cpp
TEST_CASE("BaseExporter retryBackoffMs produces the documented schedule",
          "[baseexporter][retry]")
{
    REQUIRE(BaseExporter::retryBackoffMs(0) == 500);
    REQUIRE(BaseExporter::retryBackoffMs(1) == 1500);
    REQUIRE(BaseExporter::retryBackoffMs(2) == 4500);
}

TEST_CASE("BaseExporter retry constants are sane",
          "[baseexporter][retry]")
{
    REQUIRE(BaseExporter::kMaxRetries == 3);
    REQUIRE(BaseExporter::kTransferTimeoutMs >= 5000);
}
```

- [ ] **Step 2: Run tests to verify failure**

Run:
```bash
cmake --build /mnt/BA707A64707A2773/code/orVocApp/build/Desktop_Qt_6_10_2-Debug --target orVocAppTests 2>&1 | tail -20
```
Expected: **compile error** — `retryBackoffMs`, `kMaxRetries`, `kTransferTimeoutMs` not declared.

- [ ] **Step 3: Add declarations and constants to `baseexporter.h`**

In `orVocApp/baseexporter.h`, inside `class BaseExporter`, add the following to the `public:` section (immediately after the `isCancelled()` inline definition, before `signals:`):

```cpp
    /// @brief Maximum retry attempts per network request before failing the whole export.
    static constexpr int kMaxRetries = 3;

    /// @brief Per-request transfer timeout in milliseconds (applied to both dict and translation).
    static constexpr int kTransferTimeoutMs = 10000;

    /**
     * @brief Returns the backoff delay in milliseconds for retry attempt @p attempt.
     * @param attempt Zero-based attempt index (0 == first retry, 1 == second, …).
     * @return 500 ms for attempt 0, 1500 ms for attempt 1, 4500 ms for attempt 2, and
     *         more generally @c 500 * 3^attempt. Defined as a pure function so it can
     *         be unit-tested without touching the network.
     */
    static int retryBackoffMs(int attempt);
```

- [ ] **Step 4: Implement in `baseexporter.cpp`**

Near the top of `orVocApp/baseexporter.cpp`, immediately after the existing `BaseExporter::~BaseExporter()` destructor (around line 22), add:

```cpp
int BaseExporter::retryBackoffMs(int attempt)
{
    // 500 * 3^attempt — 500, 1500, 4500 for attempts 0..2.
    int delay = 500;
    for (int i = 0; i < attempt; ++i)
        delay *= 3;
    return delay;
}
```

- [ ] **Step 5: Run tests to verify pass**

Run:
```bash
cmake --build /mnt/BA707A64707A2773/code/orVocApp/build/Desktop_Qt_6_10_2-Debug --target orVocAppTests 2>&1 | tail -10 && \
ctest --test-dir /mnt/BA707A64707A2773/code/orVocApp/build/Desktop_Qt_6_10_2-Debug --output-on-failure -R baseexporter
```
Expected: both new `[baseexporter][retry]` TEST_CASEs pass; existing cancel test still passes. Read the summary line carefully — report the actual pass/fail counts, not just the exit code.

- [ ] **Step 6: Commit**

```bash
cd /mnt/BA707A64707A2773/code/orVocApp && \
git add orVocApp/baseexporter.h orVocApp/baseexporter.cpp tests/test_baseexporter_cancel.cpp && \
git commit -m "test(RVC-45): ADDED: retryBackoffMs helper with schedule covered by Catch2"
```

---

## Task 2: Add retry + transfer-timeout to the fetch pipeline

Rewrite the per-word fetch flow to retry each request up to `kMaxRetries` times with exponential backoff, apply a transfer timeout, and hard-fail the export if retries are exhausted for any word. Preserve: (a) the existing 150 ms inter-word throttle, (b) `requestCancel()` semantics (cancel observed between retries and during the backoff wait), (c) the existing `finished(success, path)` / `cancelled()` contract.

**Files:**
- Modify: `orVocApp/baseexporter.h` (add phase enum, state members, private method decls, Doxygen)
- Modify: `orVocApp/baseexporter.cpp` (replace `fetchNextWord`, `onDefinitionReply`, `onTranslationReply` with new `issueCurrentRequest`, `onReplyFinished`, `scheduleRetryOrFail`, `advanceWord`)

- [ ] **Step 1: Add state and private API to `baseexporter.h`**

In `orVocApp/baseexporter.h`, extend the existing `private:` section at the bottom of the class. Replace the existing block:

```cpp
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

with this:

```cpp
private:
    /// @brief Which leg of the per-word fetch pair is currently in-flight.
    enum class FetchPhase { Definition, Translation };

    void fetchNextWord();         ///< Resets retry state and starts the definition leg for m_currentIndex.
    void issueCurrentRequest();   ///< (Re-)issues the HTTP GET for the current phase+word.
    void onReplyFinished(QNetworkReply *reply); ///< Handles dict or translation reply based on m_currentPhase.
    void scheduleRetryOrFail();   ///< Schedules a backoff retry of issueCurrentRequest, or hard-fails the export.
    void advanceWord();           ///< Emits progress, clears retry state, schedules next word via the 150ms throttle.
    void startRender();

    QStringList m_words;
    QString m_outputPath;
    QNetworkAccessManager *m_nam = nullptr;
    QVector<WordEntry> m_entries;
    int m_currentIndex = 0;
    std::atomic<bool> m_cancelled{false};
    QPointer<QNetworkReply> m_currentReply;

    FetchPhase m_currentPhase = FetchPhase::Definition;
    int m_retryAttempt = 0;       ///< 0 = original attempt; >0 = retry number.
};
```

- [ ] **Step 2: Replace the fetch implementation in `baseexporter.cpp`**

In `orVocApp/baseexporter.cpp`, replace the three existing methods `fetchNextWord()`, `onDefinitionReply()`, `onTranslationReply()` (lines approximately 41–123 in the current file) with the following six methods. Keep `exportToFile`, `requestCancel`, `startRender`, and the new `retryBackoffMs` (from Task 1) unchanged.

Exact `old_string` to replace:

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

Exact `new_string` replacement:

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

    m_currentPhase = FetchPhase::Definition;
    m_retryAttempt = 0;
    issueCurrentRequest();
}

void BaseExporter::issueCurrentRequest()
{
    if (m_cancelled.load(std::memory_order_acquire)) {
        emit cancelled();
        return;
    }

    const QString &word = m_words[m_currentIndex];

    QNetworkRequest request;
    if (m_currentPhase == FetchPhase::Definition) {
        QUrl url("https://api.dictionaryapi.dev/api/v2/entries/en/" + word);
        request.setUrl(url);
    } else {
        QUrl url("https://translate.googleapis.com/translate_a/single");
        QUrlQuery query;
        query.addQueryItem("client", "gtx");
        query.addQueryItem("sl", "en");
        query.addQueryItem("tl", "ar");
        query.addQueryItem("dt", "t");
        query.addQueryItem("q", word);
        url.setQuery(query);
        request.setUrl(url);
    }
    request.setTransferTimeout(kTransferTimeoutMs);

    QNetworkReply *reply = m_nam->get(request);
    m_currentReply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        onReplyFinished(reply);
    });
}

void BaseExporter::onReplyFinished(QNetworkReply *reply)
{
    if (m_cancelled.load(std::memory_order_acquire)) {
        emit cancelled();
        return;
    }

    WordEntry &entry = m_entries[m_currentIndex];

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray body = reply->readAll();

        if (m_currentPhase == FetchPhase::Definition) {
            auto result = NetworkClient::parseDictionaryResponse(body);
            if (!result.error) {
                entry.definitionHtml = result.html;
                entry.phonetic = result.phonetic;
                m_currentPhase = FetchPhase::Translation;
                m_retryAttempt = 0;
                issueCurrentRequest();
                return;
            }
        } else {
            auto result = NetworkClient::parseTranslationResponse(body);
            if (!result.error) {
                entry.translationHtml = result.html;
                entry.valid = true;
                advanceWord();
                return;
            }
        }
    }

    scheduleRetryOrFail();
}

void BaseExporter::scheduleRetryOrFail()
{
    if (m_cancelled.load(std::memory_order_acquire)) {
        emit cancelled();
        return;
    }

    if (m_retryAttempt >= kMaxRetries) {
        // Exhausted retries for this word — hard-fail the whole export.
        // No partial PDF exists yet (render phase hasn't started).
        emit finished(false, m_outputPath);
        return;
    }

    const int delay = retryBackoffMs(m_retryAttempt);
    ++m_retryAttempt;
    QTimer::singleShot(delay, this, &BaseExporter::issueCurrentRequest);
}

void BaseExporter::advanceWord()
{
    emit progress(m_currentIndex + 1, m_words.size());
    m_currentIndex++;
    m_retryAttempt = 0;
    m_currentPhase = FetchPhase::Definition;
    QTimer::singleShot(150, this, &BaseExporter::fetchNextWord);
}
```

- [ ] **Step 3: Build**

Run:
```bash
cmake --build /mnt/BA707A64707A2773/code/orVocApp/build/Desktop_Qt_6_10_2-Debug 2>&1 | tail -20
```
Expected: clean build, no warnings related to the rewrite. If a warning fires on `QUrlQuery` include, the header already has `#include <QUrlQuery>` — confirm it's still there.

- [ ] **Step 4: Run all unit tests to confirm no regression**

Run:
```bash
ctest --test-dir /mnt/BA707A64707A2773/code/orVocApp/build/Desktop_Qt_6_10_2-Debug --output-on-failure
```
Expected: all previously-passing tests still pass (VocabManager, NetworkClient, PdfExporter, BaseExporter cancel). Read the summary line. Report exact pass/fail counts. The cancel test must still succeed — the cancel check is preserved at the same five points (`fetchNextWord`, `issueCurrentRequest`, `onReplyFinished`, `scheduleRetryOrFail`, plus the existing check in `startRender`).

- [ ] **Step 5: Commit**

```bash
cd /mnt/BA707A64707A2773/code/orVocApp && \
git add orVocApp/baseexporter.h orVocApp/baseexporter.cpp && \
git commit -m "fix(RVC-45): FIXED: PDF export drops words on transient API failures"
```

---

## Task 3: Manual verification

This is the real acceptance gate. The unit tests only cover the backoff schedule; the actual "stop dropping words" behaviour has to be observed against the live APIs.

**Files:** none (observation only)

- [ ] **Step 1: Launch the Debug build against the user's real vocab.json**

Run:
```bash
/mnt/BA707A64707A2773/code/orVocApp/build/Desktop_Qt_6_10_2-Debug/orVocApp/apporVocApp
```

- [ ] **Step 2: Export reference text file**

In the app, click **Export → Text**, save as `~/Desktop/output/vocab.txt`. Note the total line count — this is the ground truth (should be ~361 words).

- [ ] **Step 3: Export PDF**

In the app, click **Export → PDF → A4 → Export**, save as `~/Desktop/output/vocab.pdf`. Watch the progress counter — it should reach `361/361`. If it hard-fails with "Export failed", note which index the progress bar stopped at (indicates the API is genuinely down; re-run at a better moment).

- [ ] **Step 4: Compare coverage**

Open the PDF. Spot-check the letter groups the user flagged as truncated — especially **M** — and verify they now contain the full set of words that exist in the text export under that letter. Specifically, check every word starting with `m` from `vocab.txt` appears in the PDF's `M` section. Do the same for `c`, `d`, `s` (the long groups) as a cross-check.

Report the result as:
- `M` section in PDF: N words present / N words expected from txt → PASS/FAIL
- `C` section in PDF: N / N → PASS/FAIL
- `D` section in PDF: N / N → PASS/FAIL
- `S` section in PDF: N / N → PASS/FAIL

- [ ] **Step 5: Regression — cancel still works**

Start a PDF export, click the red Cancel button mid-way (around 50/361). Expected: progress stops, overlay shows "Export cancelled" (transient green notification), no `.pdf` file left on Desktop.

- [ ] **Step 6: Regression — success notification on small list**

(Optional sanity check.) Temporarily reduce to a handful of words by importing a 5-line `.txt`, export PDF, verify "PDF exported successfully" notification still appears.

- [ ] **Step 7: If any of Steps 4–5 fail**

Do **not** commit any ad-hoc fixes yet. Return to the systematic-debugging skill, add `qDebug()` instrumentation around `onReplyFinished`/`scheduleRetryOrFail` on a throwaway commit, capture the log, then decide whether the fix needs: longer timeout / more retries / different backoff curve / narrower retry criterion. Iterate in new commits on this branch.

---

## Task 4: Update Doxygen comments for the changed API

Document the new retry/timeout behaviour so future readers see the contract.

**Files:**
- Modify: `orVocApp/baseexporter.h` (update the class-level comment on `BaseExporter`)

- [ ] **Step 1: Update the class-level Doxygen block**

In `orVocApp/baseexporter.h`, find the class-level comment that begins `@brief Abstract base class for exporters…`. Replace its `@brief` + body (the bullet list about the two-phase pipeline) with:

```cpp
/**
 * @brief Abstract base class for exporters that need dictionary + translation data per word.
 *
 * Runs a two-phase pipeline:
 *  1. Fetch phase — for each input word, sequentially fetches the dictionary entry
 *     and then the Arabic translation via QNetworkAccessManager. Each individual
 *     request carries a @ref kTransferTimeoutMs transfer timeout and is retried up
 *     to @ref kMaxRetries times with exponential backoff (see @ref retryBackoffMs)
 *     on any network/parse failure. If retries are exhausted for a single word, the
 *     whole export hard-fails via @ref finished with @c success=false — the pipeline
 *     never silently drops words from the output. A 150 ms throttle is applied
 *     between consecutive words.
 *  2. Render phase — spawns a QThread that calls the subclass-provided
 *     @ref renderToFile implementation with the successfully-fetched entries.
 *
 * Subclasses implement @ref renderToFile to produce a format-specific output file
 * (e.g. PDF). Progress is reported via @ref progress; completion via @ref finished;
 * user-initiated cancellation via @ref cancelled.
 */
```

- [ ] **Step 2: Build to confirm comment syntax didn't break anything**

Run:
```bash
cmake --build /mnt/BA707A64707A2773/code/orVocApp/build/Desktop_Qt_6_10_2-Debug 2>&1 | tail -5
```
Expected: clean.

- [ ] **Step 3: Commit**

```bash
cd /mnt/BA707A64707A2773/code/orVocApp && \
git add orVocApp/baseexporter.h && \
git commit -m "docs(RVC-45): ADDED: Doxygen on retry and transfer-timeout behaviour"
```

---

## Task 5: Open PR

Only after Task 3 manual verification reports PASS across all spot-checked letter groups.

- [ ] **Step 1: Push the branch**

```bash
cd /mnt/BA707A64707A2773/code/orVocApp && \
git push -u origin fix/RVC-45-pdf-export-drops-words
```

- [ ] **Step 2: Open PR**

```bash
cd /mnt/BA707A64707A2773/code/orVocApp && \
gh pr create --title "fix(RVC-45): PDF export drops words on transient API failures" --body "$(cat <<'EOF'
## Summary
- Adds exponential-backoff retries (3 attempts: 500/1500/4500 ms) and a 10 s transfer timeout to every dictionary and translation request in the PDF export fetch loop.
- On exhausted retries, the whole export hard-fails via the existing `finished(false, path)` signal instead of silently dropping the word from the PDF.
- No PDF layout/content changes. No VocabManager / NetworkClient / UI changes.

## Test plan
- [x] Unit: `retryBackoffMs(0|1|2)` schedule verified by Catch2.
- [x] Unit: all pre-existing tests still pass (VocabManager, NetworkClient, PdfExporter, BaseExporter cancel).
- [ ] Manual: export text + PDF from user's real vocab.json; every word present in the text export also present in the PDF under its letter group.
- [ ] Manual: mid-export cancel still yields "Export cancelled" notification with no stray .pdf file.

Ticket: https://welorfi.atlassian.net/jira/software/projects/RVC/boards/46?selectedIssue=RVC-45
EOF
)"
```

---

## Self-review checklist

- **Spec coverage:**
  - Drop root cause → Task 2 (retry + timeout around both fetch legs).
  - "Only retry; do not change PDF semantics" → Task 2 (PDF render untouched; only fetch loop changed).
  - "No listing failed words for user" → Task 2 (hard-fail via existing `finished(false)` path; the existing overlay already surfaces "Export failed"; no new UI).
  - "Trial and error is fine" → Task 3 Step 7 leaves iteration room in this branch.
  - "Debug build in Qt Creator path" → all build/test commands use `build/Desktop_Qt_6_10_2-Debug`.
  - "orfi-git-conventions" → branch `fix/RVC-45-…`, commit verbs `ADDED`/`FIXED`, PR title `fix(RVC-45): …`.
  - "Tests if feasible" → Task 1 TDDs the only cleanly-unit-testable slice (backoff helper); retry-around-network is manual per Task 3.
- **Placeholder scan:** no TBD / fill-in / "similar to task N" / vague "add error handling" strings.
- **Type consistency:** `FetchPhase`, `m_currentPhase`, `m_retryAttempt`, `kMaxRetries`, `kTransferTimeoutMs`, `retryBackoffMs`, `issueCurrentRequest`, `onReplyFinished`, `scheduleRetryOrFail`, `advanceWord` spelled identically across header, impl, and tests.
