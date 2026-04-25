# RVC-35 — Not Adding Wrong Words

## Ticket
- **Key:** RVC-35
- **Title:** Not adding wrong words
- **Type:** Task
- **Jira:** https://welorfi.atlassian.net/jira/software/projects/RVC/boards/46?selectedIssue=RVC-35
- **Branch:** `feat/RVC-35-validate-word-before-add`

## Problem
Currently, the Sidebar's Add button (and Enter key) insert a word into the bank immediately. Invalid words — typos, non-English strings, gibberish — pollute the list and fail silently when their definition lookup returns no result.

## Goal
Validate that a word has an English dictionary entry **before** it is added to the bank. If validation fails for any reason (not a real word, or network unavailable), the word is not added and the user is notified via a fading overlay that reuses the existing PDF-export notification surface.

## User-Facing Behavior

### Happy path (valid word)
1. User types a word and presses Enter or clicks Add.
2. Search field and Add button disable during the in-flight lookup.
3. Dictionary API responds with a definition.
4. Word is inserted into the bank, selected, and scrolled into view.
5. `TranslationView` fetches and renders the definition (the normal display path).
6. Inputs re-enable.

### Word already in the bank
1. User types a word that already exists.
2. **No network call is made.** The word is selected and scrolled into view.
3. `TranslationView` performs its usual lookup to display the definition.

### Invalid word
1. User types a garbage string and presses Enter or clicks Add.
2. Inputs disable during the in-flight lookup.
3. Dictionary API returns 404 (or a parse error).
4. Word is **not** added. Inputs re-enable.
5. Notification overlay appears with the text:
   > **Could not validate! Please check your input and connection!**
6. Notification fades in, holds for 3 seconds, fades out.

### Network failure / timeout
1. User types a word while offline (or the API hangs).
2. Inputs disable.
3. Either `QNetworkReply` errors, **or** a client-side 5-second timer expires.
4. Word is **not** added. Inputs re-enable.
5. Same notification as the invalid-word case appears (unified message).

## Scope of Change

### Files touched
- **`orVocab/ExportOverlay.qml` → renamed to `orVocab/NotificationOverlay.qml`.** Generalized to support arbitrary notifications (not only PDF-export results). Hold time unified to 3000ms for all notifications.
- **`orVocab/Sidebar.qml`.** Replaces the direct `addWord` call with a validate-then-add flow. Adds `validating` state and `validationFailed` signal. Uses a `Connections` block to listen for `NetworkClient.definitionReady` / `requestFailed` only while a validation is in flight. Adds a 5-second timeout `Timer`.
- **`orVocab/Main.qml`.** Updates the component reference (`ExportOverlay { id: exportOverlay }` → `NotificationOverlay { id: notificationOverlay }`) and all call sites. Forwards `Sidebar.validationFailed` to `notificationOverlay.showNotification(...)`.
- **`orVocab/CMakeLists.txt`.** Updates the QML_FILES entry from `ExportOverlay.qml` to `NotificationOverlay.qml`.

### Files unchanged
- `orVocab/vocabmanager.{h,cpp}` — `addWord` stays synchronous; no API contract change.
- `orVocab/networkclient.{h,cpp}` — existing `fetchDefinition` + `definitionReady` / `requestFailed` signals are sufficient.
- `orVocab/TranslationView.qml` — no change. Its existing `lookupWord` path fires after the word is added; the second dictionary fetch (per decision #4 in the Decisions Log) is acceptable.
- `orVocab/baseexporter.{h,cpp}`, `orVocab/pdfexporter.{h,cpp}` — untouched.
- `tests/*` — no new Catch2 tests; logic change is confined to QML.
- Packaging and build scripts — untouched.

## Component Design

### `NotificationOverlay.qml` (renamed from `ExportOverlay.qml`)

**Kept (existing PDF-export surface):**
- `property bool exporting` — unchanged, drives the progress bar visibility.
- `startExport(wordCount)` / `updateProgress(cur, tot)` — unchanged.
- `showResult(success)` — now a thin wrapper around the generic method.

**New:**
- `showNotification(text, isError)` — sets label text, sets color (green `#27ae60` for success, red `#e74c3c` for error), runs the fade-in → 3s hold → fade-out sequence.

**Behavior change:**
- Hold timer reduced from 4000ms to **3000ms**, applied uniformly to both PDF-export results and validation-failure notifications.

**Refactored `showResult`:**
```qml
function showResult(success) {
    exporting = false;
    showNotification(
        success ? "PDF exported successfully" : "Export failed",
        !success
    );
}
```

### `Sidebar.qml`

**New state:**
```qml
property bool validating: false
property string pendingWord: ""
signal validationFailed()
```

**Input disabling:**
- `TextField` (search) — `enabled: !sidebar.validating`
- `Button` (Add) — `enabled: !sidebar.validating && searchField.text.trim() !== ""`

**Add flow (`attemptAdd()` function):**
```qml
function attemptAdd() {
    var word = searchField.text.trim().toLowerCase();
    if (word === "") return;

    // Skip validation if already in bank
    if (VocabManager.indexOfWord(word) >= 0) {
        selectAndScrollTo(word);
        searchField.text = "";
        return;
    }

    sidebar.pendingWord = word;
    sidebar.validating = true;
    validationTimeout.restart();
    NetworkClient.fetchDefinition(word);
}
```

**Signal handling (`Connections` block, enabled only while validating):**
```qml
Connections {
    target: NetworkClient
    enabled: sidebar.validating

    function onDefinitionReady(html) {
        validationTimeout.stop();
        VocabManager.addWord(sidebar.pendingWord);
        selectAndScrollTo(sidebar.pendingWord);
        searchField.text = "";
        sidebar.validating = false;
        sidebar.pendingWord = "";
    }

    function onRequestFailed(area, errorString) {
        if (area !== "definition") return;
        validationTimeout.stop();
        sidebar.validationFailed();
        sidebar.validating = false;
        sidebar.pendingWord = "";
    }
}
```

**Timeout timer:**
```qml
Timer {
    id: validationTimeout
    interval: 5000
    repeat: false
    onTriggered: {
        if (!sidebar.validating) return;
        sidebar.validationFailed();
        sidebar.validating = false;
        sidebar.pendingWord = "";
    }
}
```

**Trigger wiring:**
- `searchField.onAccepted: sidebar.attemptAdd()`
- `addButton.onClicked: sidebar.attemptAdd()`

**Helper function (`selectAndScrollTo`):** A small refactor of the existing selection logic (currently inline in the Add button's `onClicked`) into a reusable function. It looks up the word's index in the proxy model, sets `wordList.currentIndex`, calls `wordList.positionViewAtIndex`, and emits `sidebar.wordClicked(word)`.

### `Main.qml`

Forward the new signal:
```qml
Sidebar {
    onValidationFailed: notificationOverlay.showNotification(
        "Could not validate! Please check your input and connection!",
        true
    )
    ...
}
```

Update all existing `exportOverlay` identifiers to `notificationOverlay`.

## Decisions Log

| # | Decision | Rationale |
|---|---|---|
| 1 | On network failure, **block** the add and show a **unified notification** message regardless of cause (404 vs offline) | Simpler UX; the user's recovery action is the same: check input, check connection |
| 2 | **Disable search field + Add button** during validation; do not dim the sidebar or Import button | Validation is short (~300ms); full-dimming is proportionate to multi-second operations like PDF export |
| 3 | **Skip validation** when the word is already in the bank | It was validated at add time; saves an API call and latency |
| 4 | On success, fire a **second dictionary fetch** via the existing `TranslationView.lookupWord` path (do not reuse the validation response) | Simpler: no coupling between Sidebar and TranslationView. The extra fetch is negligible compared to the clarity benefit |
| 5 | **Rename** `ExportOverlay.qml` → `NotificationOverlay.qml` now, not later | Scope is small; avoids a follow-up rename commit |
| 6 | Unified **3000ms** hold time for all notifications (was 4000ms for PDF export) | Matches ticket spec; consistent UX across notification types |
| 7 | **5-second client-side timeout** on validation | Prevents the app from appearing frozen on hung network; re-enables inputs with the same error notification |

## Out of Scope
- C++ `VocabManager.addWord` stays synchronous.
- No new Catch2 unit tests (logic change is confined to QML; no public C++ API change).
- `TranslationView`, `ImportPopup`, `ExportPopup`, packaging scripts unchanged.
- No offline mode or local-cache-based validation.

## Testing

### Automated (existing, must still pass)
All 11 existing Catch2 tests continue to pass — no C++ changes, so no regression risk at the unit level.

### Manual test plan
1. Add a valid word (`hello`) via Enter — inputs disable briefly, word added, definition appears.
2. Add a valid word (`hello`) via the Add button — same outcome.
3. Add a garbage word (`asdfqwerty`) — inputs disable, re-enable, red notification shows the unified message, fades after 3s, word is not in the list.
4. Type a word already in the bank and press Enter — no network call (verify via DevTools or logging), word is selected and scrolled into view.
5. Disconnect network, try adding any new word — after ~5s the notification appears and inputs re-enable.
6. Export a PDF after any of the above — notification still reads `PDF exported successfully` and fades at 3s (not 4s).
7. Search/filter still works when not validating.

## Risks
- **Signal coordination bug.** If `Connections.enabled` is toggled at the wrong time, a stale `definitionReady` could fire and mutate state. Mitigated by setting `validating = false` synchronously in both handlers and the timeout.
- **API rate limiting on rapid typing.** Each Add press sends one request, but the disabled inputs prevent repeated pressing. Low risk.
- **Stale reply after timeout.** If the 5s timeout fires, we show the error notification and reset `validating = false`. A late `definitionReady` arriving afterwards will be ignored (because `Connections.enabled` is now false), but the word will not be added. Acceptable — the user got feedback and can retry.
