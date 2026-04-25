# RVC-35 — Not Adding Wrong Words — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Validate that a word has an English dictionary entry before adding it to the vocabulary bank; on failure, show a 3-second fading notification using a generalized overlay renamed from `ExportOverlay` to `NotificationOverlay`.

**Architecture:** All changes are confined to the QML layer. Validation is orchestrated in `Sidebar.qml`: an `attemptAdd()` function gates insertion on a dictionary-API round trip by subscribing to `NetworkClient.definitionReady` / `requestFailed` while a `validating` flag is active. A 5-second `Timer` bounds hung network calls. The existing `ExportOverlay` is renamed to `NotificationOverlay` and gains a generic `showNotification(text, isError)` method; `showResult(success)` becomes a thin wrapper. Hold time is unified to 3000ms. No C++ is changed.

**Tech Stack:** Qt 6.10 / QML (QtQuick.Controls, QtQuick.Layouts), `QNetworkAccessManager` (unchanged), Catch2 v3 for existing tests (unchanged), CMake + Ninja.

**Spec:** `design/RVC-35-not-adding-wrong-words.md`

---

## File Structure

Files modified:
- `orVocab/ExportOverlay.qml` → **renamed** to `orVocab/NotificationOverlay.qml`, adds `showNotification(text, isError)`; hold time 4000ms → 3000ms.
- `orVocab/Sidebar.qml` → adds `validating` state, `pendingWord`, `validationFailed` signal, `attemptAdd()` function, `selectAndScrollTo()` helper, a `Connections` block gated by `validating`, and a `validationTimeout` Timer. Disables search field + Add button while validating.
- `orVocab/Main.qml` → renames `ExportOverlay` references to `NotificationOverlay`, renames the `exportOverlay` id to `notificationOverlay`, wires `Sidebar.onValidationFailed` → `notificationOverlay.showNotification(...)`.
- `orVocab/CMakeLists.txt` → updates `QML_FILES` entry `ExportOverlay.qml` → `NotificationOverlay.qml`.

Files unchanged:
- `orVocab/vocabmanager.{h,cpp}`, `orVocab/networkclient.{h,cpp}`, `orVocab/TranslationView.qml`, `orVocab/ImportPopup.qml`, `orVocab/ExportPopup.qml`, `orVocab/baseexporter.{h,cpp}`, `orVocab/pdfexporter.{h,cpp}`, all `tests/*`, packaging scripts.

---

## Task 1: Rename `ExportOverlay.qml` to `NotificationOverlay.qml` and update all references

**Files:**
- Rename: `orVocab/ExportOverlay.qml` → `orVocab/NotificationOverlay.qml`
- Modify: `orVocab/CMakeLists.txt` line 23
- Modify: `orVocab/Main.qml` — change component type `ExportOverlay` → `NotificationOverlay`, rename id `exportOverlay` → `notificationOverlay`, update all references.

This task performs the rename as a single atomic commit so the tree is never left in a broken state. No behavior change.

- [ ] **Step 1: Rename the file with git mv**

```bash
git mv orVocab/ExportOverlay.qml orVocab/NotificationOverlay.qml
```

- [ ] **Step 2: Update CMakeLists.txt**

Modify `orVocab/CMakeLists.txt` line 23. Change:

```cmake
        ExportOverlay.qml
```

to:

```cmake
        NotificationOverlay.qml
```

- [ ] **Step 3: Replace the component reference in `Main.qml`**

At `orVocab/Main.qml:49-51`, change:

```qml
            ExportOverlay {
                id: exportOverlay
            }
```

to:

```qml
            NotificationOverlay {
                id: notificationOverlay
            }
```

- [ ] **Step 4: Update all `exportOverlay` identifiers to `notificationOverlay` in `Main.qml`**

Occurrences to replace (from the current file):
- Line 62: `enabled: !exportOverlay.exporting`
- Line 75: `enabled: !exportOverlay.exporting`
- Line 76: `opacity: exportOverlay.exporting ? 0.4 : 1.0`
- Line 153: `exporting: exportOverlay.exporting`
- Line 190: `exportOverlay.startExport(VocabManager.words.length);`
- Line 191: `exporter.progress.connect(exportOverlay.updateProgress);`
- Line 193: `exportOverlay.showResult(success);`

Use a single `sed` to rename all of them in one pass:

```bash
sed -i 's/exportOverlay/notificationOverlay/g' orVocab/Main.qml
```

- [ ] **Step 5: Verify no `ExportOverlay` or `exportOverlay` identifiers remain in QML or CMake**

```bash
grep -n 'ExportOverlay\|exportOverlay' orVocab/*.qml orVocab/CMakeLists.txt
```

Expected: no output. If `grep` returns any lines, fix them before continuing.

- [ ] **Step 6: Configure and build**

```bash
cd orVocab
cmake -B build -DCMAKE_BUILD_TYPE=Debug -G Ninja
cmake --build build 2>&1 | tail -10
```

Expected: build succeeds. Look for `Linking CXX executable apporVocApp` in the output.

- [ ] **Step 7: Smoke-run the app**

```bash
./orVocab/build/apporVocApp &
sleep 3
pkill -f apporVocApp
```

Expected: app starts without QML errors printed to stdout. (Watch for `QQmlComponent: Component is not ready` or `Type not registered`.)

- [ ] **Step 8: Run existing tests to confirm no C++ regression**

```bash
cd orVocab/build && ctest --output-on-failure
```

Expected: read the summary line — must show `11/11 tests passed`.

- [ ] **Step 9: Commit**

```bash
git add orVocab/NotificationOverlay.qml orVocab/Main.qml orVocab/CMakeLists.txt
git commit -m "refactor(RVC-35): MOVED: ExportOverlay.qml to NotificationOverlay.qml"
```

---

## Task 2: Generalize `NotificationOverlay` with `showNotification()` and unify hold time

**Files:**
- Modify: `orVocab/NotificationOverlay.qml`

Add a generic `showNotification(text, isError)` method, refactor `showResult(success)` to delegate to it, and change the hold timer from 4000ms to 3000ms.

- [ ] **Step 1: Add `showNotification(text, isError)` and refactor `showResult()`**

In `orVocab/NotificationOverlay.qml`, replace the existing `showResult` function (currently lines 28–38) with:

```qml
    function showNotification(text, isError) {
        notificationLabel.text = text;
        notificationLabel.color = isError ? "#e74c3c" : "#27ae60";
        fadeInNotification.start();
    }

    function showResult(success) {
        exporting = false;
        showNotification(
            success ? "PDF exported successfully" : "Export failed",
            !success
        );
    }
```

- [ ] **Step 2: Change the hold timer from 4000ms to 3000ms**

In `orVocab/NotificationOverlay.qml`, the `Timer` with `id: holdTimer` currently uses `interval: 4000`. Change to:

```qml
    Timer {
        id: holdTimer
        interval: 3000
        onTriggered: fadeOutNotification.start()
    }
```

- [ ] **Step 3: Rebuild**

```bash
cd orVocab
cmake --build build 2>&1 | tail -5
```

Expected: build succeeds.

- [ ] **Step 4: Manual smoke test — trigger a PDF export, verify 3-second hold**

```bash
./orVocab/build/apporVocApp &
```

In the running app:
1. Click Export → PDF → choose A4 → save to `/tmp/test.pdf`.
2. Wait for the export to finish.
3. Verify the green "PDF exported successfully" text fades in, holds for ~3 seconds (not 4), then fades out.
4. Close the app.

If the timing is obviously wrong (e.g., still ~4s), re-check the `Timer.interval` value.

- [ ] **Step 5: Commit**

```bash
git add orVocab/NotificationOverlay.qml
git commit -m "refactor(RVC-35): ADDED: generic showNotification method and unified 3s hold time"
```

---

## Task 3: Add `selectAndScrollTo` helper + `validating` state to `Sidebar.qml`

**Files:**
- Modify: `orVocab/Sidebar.qml`

This refactors the existing selection logic into a reusable helper and introduces the `validating` flag. No network coordination yet — that comes in Task 4.

- [ ] **Step 1: Add state properties, signal, and helper at the top of `Sidebar`**

In `orVocab/Sidebar.qml`, insert the following immediately after the existing signal declarations (currently lines 12-14: `signal wordSelected`, `signal wordClicked`, `signal wordDeleted`):

```qml
    signal validationFailed()

    property bool validating: false
    property string pendingWord: ""

    function selectAndScrollTo(word) {
        var idx = VocabManager.indexOfWord(word);
        if (idx >= 0) {
            wordList.currentIndex = idx;
            wordList.positionViewAtIndex(idx, ListView.Contain);
            sidebar.wordClicked(word);
        }
    }
```

- [ ] **Step 2: Disable search field while validating**

In `orVocab/Sidebar.qml`, change the `TextField` block (currently at lines 25-31):

```qml
            TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: "Search or add word..."
                onTextChanged: VocabManager.filterWords(text)
                onAccepted: addButton.clicked()
            }
```

to:

```qml
            TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: "Search or add word..."
                enabled: !sidebar.validating
                onTextChanged: VocabManager.filterWords(text)
                onAccepted: addButton.clicked()
            }
```

- [ ] **Step 3: Disable Add button while validating (and when empty)**

In `orVocab/Sidebar.qml`, change the `Button` block (currently at lines 33-49). Replace its body while keeping the button's existing click logic intact for now:

```qml
            Button {
                id: addButton
                text: "Add"
                enabled: !sidebar.validating && searchField.text.trim() !== ""
                onClicked: {
                    if (searchField.text.trim() === "")
                        return;
                    var word = searchField.text.trim().toLowerCase();
                    VocabManager.addWord(searchField.text);
                    searchField.text = "";
                    sidebar.selectAndScrollTo(word);
                }
            }
```

Note: the `onClicked` body was refactored to call `selectAndScrollTo(word)` instead of the inlined index-lookup / currentIndex / positionViewAtIndex / wordClicked block. Behavior is identical (no validation yet).

- [ ] **Step 4: Build and smoke-test (behavior should be unchanged from main)**

```bash
cd orVocab
cmake --build build 2>&1 | tail -5
./build/apporVocApp &
```

In the app:
1. Add a new word → word appears + definition loads (same as before).
2. With the search field empty, observe the Add button is disabled (previously it allowed the click but returned silently; now it's disabled — consistent with the upcoming `validating` gate).
3. Close the app.

- [ ] **Step 5: Run existing tests**

```bash
cd orVocab/build && ctest --output-on-failure
```

Expected: read the summary line — must show `11/11 tests passed`.

- [ ] **Step 6: Commit**

```bash
git add orVocab/Sidebar.qml
git commit -m "refactor(RVC-35): ADDED: selectAndScrollTo helper and validating state to Sidebar"
```

---

## Task 4: Wire validation network flow in `Sidebar.qml`

**Files:**
- Modify: `orVocab/Sidebar.qml`

Introduce `attemptAdd()` which branches on whether the word already exists, otherwise fires the network validation. Add a `Connections` block to `NetworkClient` gated on `validating`, and a 5-second `validationTimeout` Timer.

- [ ] **Step 1: Add `attemptAdd()` function under `selectAndScrollTo()` in `Sidebar.qml`**

Insert immediately after the `selectAndScrollTo` function:

```qml
    function attemptAdd() {
        var word = searchField.text.trim().toLowerCase();
        if (word === "")
            return;

        if (VocabManager.indexOfWord(word) >= 0) {
            sidebar.selectAndScrollTo(word);
            searchField.text = "";
            return;
        }

        sidebar.pendingWord = word;
        sidebar.validating = true;
        validationTimeout.restart();
        NetworkClient.fetchDefinition(word);
    }
```

- [ ] **Step 2: Re-point the Add button to call `attemptAdd()`**

Change the Add button's `onClicked` handler (currently the Task-3 body) to:

```qml
            Button {
                id: addButton
                text: "Add"
                enabled: !sidebar.validating && searchField.text.trim() !== ""
                onClicked: sidebar.attemptAdd()
            }
```

- [ ] **Step 3: Re-point the search field's Enter handler to call `attemptAdd()`**

Change `searchField.onAccepted` from `addButton.clicked()` to `sidebar.attemptAdd()`:

```qml
            TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: "Search or add word..."
                enabled: !sidebar.validating
                onTextChanged: VocabManager.filterWords(text)
                onAccepted: sidebar.attemptAdd()
            }
```

- [ ] **Step 4: Add the `Connections` block**

Insert the following inside the root `Rectangle { id: sidebar }` — placing it near the top of the component, after the properties and helper functions but before `ColumnLayout`:

```qml
    Connections {
        target: NetworkClient
        enabled: sidebar.validating

        function onDefinitionReady(html) {
            validationTimeout.stop();
            VocabManager.addWord(sidebar.pendingWord);
            sidebar.selectAndScrollTo(sidebar.pendingWord);
            searchField.text = "";
            sidebar.validating = false;
            sidebar.pendingWord = "";
        }

        function onRequestFailed(area, errorString) {
            if (area !== "definition")
                return;
            validationTimeout.stop();
            sidebar.validationFailed();
            sidebar.validating = false;
            sidebar.pendingWord = "";
        }
    }
```

- [ ] **Step 5: Add the `validationTimeout` Timer**

Insert after the `Connections` block:

```qml
    Timer {
        id: validationTimeout
        interval: 5000
        repeat: false
        onTriggered: {
            if (!sidebar.validating)
                return;
            sidebar.validationFailed();
            sidebar.validating = false;
            sidebar.pendingWord = "";
        }
    }
```

- [ ] **Step 6: Build**

```bash
cd orVocab
cmake --build build 2>&1 | tail -5
```

Expected: build succeeds.

- [ ] **Step 7: Commit**

```bash
git add orVocab/Sidebar.qml
git commit -m "feat(RVC-35): ADDED: validate word via dictionary API before adding to bank"
```

---

## Task 5: Wire `Sidebar.validationFailed` to `NotificationOverlay` in `Main.qml`

**Files:**
- Modify: `orVocab/Main.qml`

When the Sidebar emits `validationFailed`, show the unified error notification.

- [ ] **Step 1: Add the `onValidationFailed` handler to the `Sidebar` block in `Main.qml`**

The existing `Sidebar { ... }` block (currently at `Main.qml:72-89`) has `onWordClicked` and `onWordDeleted` handlers. Add a third handler:

```qml
        Sidebar {
            SplitView.preferredWidth: 220
            SplitView.minimumWidth: 150
            enabled: !notificationOverlay.exporting
            opacity: notificationOverlay.exporting ? 0.4 : 1.0

            onValidationFailed: notificationOverlay.showNotification(
                "Could not validate! Please check your input and connection!",
                true
            )

            onWordClicked: function(word) {
                root.currentWord = word;
                translationView.lookupWord(word);
            }

            onWordDeleted: function(word) {
                if (root.currentWord === word) {
                    root.currentWord = "";
                    translationView.clearView();
                }
            }
        }
```

- [ ] **Step 2: Build**

```bash
cd orVocab
cmake --build build 2>&1 | tail -5
```

Expected: build succeeds.

- [ ] **Step 3: Manual test — happy path (valid word)**

```bash
./orVocab/build/apporVocApp &
```

1. Type `hello` in the search field → press Enter.
2. Observe: search field + Add button disable briefly.
3. Observe: `hello` appears in the list and is selected.
4. Observe: definition loads in `TranslationView`.
5. Observe: search field + Add button re-enable.
6. Close the app.

- [ ] **Step 4: Manual test — invalid word (404)**

```bash
./orVocab/build/apporVocApp &
```

1. Type `asdfqwerty` in the search field → press Enter.
2. Observe: search field + Add button disable briefly.
3. Observe: red notification `Could not validate! Please check your input and connection!` fades in.
4. Observe: `asdfqwerty` is NOT in the word list.
5. Observe: notification fades out after ~3 seconds.
6. Observe: search field + Add button re-enable (before the notification fades, they re-enable immediately when the `requestFailed` signal arrives).
7. Close the app.

- [ ] **Step 5: Manual test — word already in bank**

```bash
./orVocab/build/apporVocApp &
```

1. Ensure `hello` is already in your word list (add it first if not).
2. Type `hello` again → press Enter.
3. Observe: no network call (inputs do NOT disable), the existing `hello` is selected and scrolled into view.
4. Observe: `TranslationView` loads the definition via its own lookup (this is the "second fetch" per decision #4 — expected).
5. Close the app.

- [ ] **Step 6: Manual test — offline / hung network (5s timeout)**

Disable the network interface (or pull Ethernet / disable Wi-Fi) before the next test:

```bash
# Example; adjust for your setup. Requires sudo.
# sudo ip link set wlan0 down
```

```bash
./orVocab/build/apporVocApp &
```

1. Type any new word → press Enter.
2. Observe: search field + Add button disable.
3. Observe: after ~5 seconds (could be earlier if `QNetworkReply` errors out first), red notification appears and inputs re-enable.
4. Observe: the word is NOT in the list.
5. Close the app, re-enable network.

- [ ] **Step 7: Manual test — PDF export unaffected**

```bash
./orVocab/build/apporVocApp &
```

1. Add a couple of valid words.
2. Click Export → PDF → A4 → save to `/tmp/test.pdf`.
3. Observe: green `PDF exported successfully` appears and fades after 3 seconds (not 4).
4. Close the app.

- [ ] **Step 8: Run existing tests one more time**

```bash
cd orVocab/build && ctest --output-on-failure
```

Expected: `11 tests passed`. Read the summary line.

- [ ] **Step 9: Commit**

```bash
git add orVocab/Main.qml
git commit -m "feat(RVC-35): ADDED: validation failure notification in Main window"
```

---

## Task 6: Final verification

**Files:** none (verification-only)

- [ ] **Step 1: Grep for leftover `ExportOverlay` / `exportOverlay` references**

```bash
grep -rn 'ExportOverlay\|exportOverlay' orVocab/ docs/ design/ README.md 2>/dev/null
```

Expected: only references in the spec `design/RVC-35-not-adding-wrong-words.md` and this plan file (both refer to the old name in context). If `orVocab/*.qml` or `CMakeLists.txt` shows a hit, fix it.

- [ ] **Step 2: Re-run the full manual checklist from the spec**

From `design/RVC-35-not-adding-wrong-words.md` → "Manual test plan" section (items 1–7). Confirm each passes.

- [ ] **Step 3: Run the test suite end-to-end**

```bash
cd orVocab/build && ctest --output-on-failure
```

Expected: read the summary line — must show `11/11 tests passed`.

- [ ] **Step 4: Review the commit log**

```bash
git log --oneline main..HEAD
```

Expected commits (in order):
1. `docs(RVC-35): ADDED: design spec for word validation before add` (already committed in brainstorming phase)
2. `refactor(RVC-35): MOVED: ExportOverlay.qml to NotificationOverlay.qml`
3. `refactor(RVC-35): ADDED: generic showNotification method and unified 3s hold time`
4. `refactor(RVC-35): ADDED: selectAndScrollTo helper and validating state to Sidebar`
5. `feat(RVC-35): ADDED: validate word via dictionary API before adding to bank`
6. `feat(RVC-35): ADDED: validation failure notification in Main window`

- [ ] **Step 5: Done — branch is ready for PR**

Do NOT open the PR from this plan. The user will decide when to ship. Report completion and stand by.

---

## Completion Criteria

All of the following must be true:
1. All 6 tasks above have every checkbox checked.
2. All 11 existing Catch2 tests pass (read the summary line, not just the exit code).
3. The manual test plan items from the spec all pass.
4. No `ExportOverlay` / `exportOverlay` identifier remains in `orVocab/` or `CMakeLists.txt`.
5. The branch `feat/RVC-35-validate-word-before-add` contains the expected commits in order.
