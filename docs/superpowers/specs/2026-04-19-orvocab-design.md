# orVocab Design Specification

## Overview

orVocab is a lightweight Qt 6 Quick (QML) desktop vocabulary builder. Users curate a personal word bank with English definitions (via dictionaryapi.dev), Arabic translations (via Google Translate GTX API), and audio pronunciations (QtMultimedia). Data persists to a local `vocab.json` file. Targets Windows 10/11 and Linux.

## Architecture

Two C++ classes exposed as QML singletons, three QML components.

### C++ Layer

**VocabManager** — word list CRUD, JSON I/O, search filtering, import/export.

- `words` : `QStringList` Q_PROPERTY — the full sorted word list, bound to QML
- `filteredWords` : `QStringList` Q_PROPERTY — the currently filtered subset, bound to QML sidebar
- `addWord(const QString &word)` — if word exists, returns its index (no duplicate added); if new, inserts alphabetically, saves to JSON, returns index
- `removeWord(const QString &word)` — removes from list, saves to JSON
- `filterWords(const QString &query)` — updates `filteredWords` with case-insensitive "contains" matching
- `loadFromJson()` — reads `vocab.json` on startup; missing file starts empty; corrupt JSON starts empty with warning logged
- `saveToJson()` — writes full word list to `vocab.json` after every mutation
- `exportJson(const QUrl &path)` — copies vocab.json to user-chosen path
- `exportText(const QUrl &path)` — writes one word per line to user-chosen .txt file
- `importJson(const QUrl &path)` — replaces entire word bank with imported JSON file (caller must confirm first)
- `importText(const QUrl &path)` — reads one-word-per-line text file, merges into existing list, skips duplicates, trims whitespace, skips blank lines, maintains alphabetical sort

**NetworkClient** — REST API calls via `QNetworkAccessManager`.

- `fetchDefinition(const QString &word)` — GET `https://api.dictionaryapi.dev/api/v2/entries/en/{word}`
- `fetchTranslation(const QString &word)` — GET `https://translate.googleapis.com/translate_a/single?client=gtx&sl=en&tl=ar&dt=t&q={word}`
- Signal: `definitionReady(const QString &html)` — formatted HTML with part-of-speech, definitions, examples
- Signal: `translationReady(const QString &html)` — Arabic text wrapped in `dir="rtl"` HTML
- Signal: `audioUrlReady(const QUrl &url)` — first valid `.mp3` URL from phonetics array
- Signal: `requestFailed(const QString &area, const QString &errorString)` — area is "definition" or "translation"

Both classes are registered as QML singletons via `qmlRegisterSingletonInstance` or `QML_SINGLETON`.

### QML Layer

**Main.qml** — root window, two-panel layout, toolbar, coordinates between Sidebar and TranslationView. Follows system dark/light theme.

**Sidebar.qml** — left panel (fixed width ~220px):
- Search/add text field at top with Add button
- Scrollable word list (`ListView`) bound to `VocabManager.filteredWords`
- Selected word highlighted
- Right-click context menu with "Delete" option

**TranslationView.qml** — right content panel:
- Word name label (large) with phonetic text below
- "Play Pronunciation" button (disabled until audio URL available)
- English Definition area (RichText)
- Arabic Translation area (RichText, RTL-aligned)
- Each area shows inline error message on API failure

## Data Model & Persistence

### vocab.json format

```json
{
    "words": ["algorithm", "binary", "cache", "delegate"]
}
```

Sorted string array. No cached definitions or translations — lookups happen live at selection time.

### File location

`QStandardPaths::AppDataLocation/vocab.json`
- Linux: `~/.local/share/orVocab/vocab.json`
- Windows: `C:/Users/<user>/AppData/Local/orVocab/vocab.json`

### Operations

- **Load** — on app startup. Missing file → empty list. Corrupt JSON → empty list, warning logged.
- **Save** — after every add/remove. Writes full list.
- **Export JSON** — copy vocab.json to user-chosen path via QML `FileDialog` (from `QtQuick.Dialogs`).
- **Export Text** — one word per line to user-chosen .txt file via QML `FileDialog`.
- **Import JSON** — user picks .json file via QML `FileDialog` → confirmation dialog ("This will replace your current word list of N words. Continue?") → replaces entire word bank. Corrupt import file → error shown, existing list untouched.
- **Import Text** — user picks .txt file via QML `FileDialog` → merges into existing list, skips duplicates, trims whitespace, skips blank lines, maintains alphabetical sort. Non-destructive, no confirmation needed.

## UI Layout & Interactions

### Layout

```
┌──────────────────────────────────────────────────────────────┐
│ orVocab              [Import JSON][Import Text][Export JSON][Export Text] │
├──────────────┬───────────────────────────────────────────────┤
│ [Search/Add] │  word name                 [Play Pronunciation] │
│  [Add]       │  /phonetic/                                    │
│──────────────│───────────────────────────────────────────────│
│  algorithm ◄ │  ┌─ English Definition ──────────────────────┐ │
│  allocate    │  │ noun                                      │ │
│  binary      │  │ A process or set of rules...              │ │
│  cache       │  │ "a basic algorithm for division"          │ │
│  delegate    │  └───────────────────────────────────────────┘ │
│              │  ┌─ Arabic Translation ──────────────────────┐ │
│              │  │                              خوارزمية     │ │
│              │  └───────────────────────────────────────────┘ │
└──────────────┴───────────────────────────────────────────────┘
```

### Interactions

| Action | Trigger | Behavior |
|--------|---------|----------|
| Search/Filter | Type in search field | Sidebar list filters in real-time, case-insensitive "contains" matching |
| Add Word | Type + click Add | If word exists → selects it in list. If new → adds alphabetically, saves, selects it |
| Select Word | Double-click in sidebar | Fires `fetchDefinition()` + `fetchTranslation()` in parallel |
| Delete Word | Right-click → "Delete" | Removes word, saves immediately |
| Play Audio | Click "Play Pronunciation" | Plays .mp3 URL via `QMediaPlayer`. Button disabled until URL available |
| Import JSON | Toolbar button | File dialog → confirmation → replaces word bank |
| Import Text | Toolbar button | File dialog → merges words, skips duplicates |
| Export JSON | Toolbar button | File dialog → saves vocab.json copy |
| Export Text | Toolbar button | File dialog → saves one-word-per-line .txt |

### Theme

Follows system dark/light mode via Qt's built-in palette detection.

## Networking & API Integration

### Dictionary API

`GET https://api.dictionaryapi.dev/api/v2/entries/en/{word}`

Parsing:
- Extract `meanings` array → format each: bold part-of-speech label, definition text, italic examples
- Extract `phonetic` string from root or `phonetics` array → display under word name
- Extract first valid `.mp3` URL from `phonetics` array → emit via `audioUrlReady`
- Format everything as HTML for QML RichText display

### Translation API

`GET https://translate.googleapis.com/translate_a/single?client=gtx&sl=en&tl=ar&dt=t&q={word}`

Parsing:
- Translation text is at `[0][0][0]` in the nested JSON arrays
- Wrap in HTML with `dir="rtl"` for proper Arabic rendering in QML RichText

### Signal flow on word selection

1. QML calls `NetworkClient.fetchDefinition(word)` and `NetworkClient.fetchTranslation(word)`
2. Both HTTP requests fire in parallel
3. As each completes: emit `definitionReady(html)`, `translationReady(html)`, or `audioUrlReady(url)`
4. On failure: emit `requestFailed(area, errorString)`
5. Each area updates independently — no waiting for both

### Error handling

| Scenario | Error message | Where displayed |
|----------|--------------|-----------------|
| Network timeout / no connection | "Could not fetch definition. Check your connection." | Definition area |
| Word not found (404) | "No definition found for '{word}'." | Definition area |
| Translation API failure | "Could not fetch translation." | Translation area |

Errors display inline in the respective content area. User re-selects the word to retry.

## Testing Strategy

### Process: Strict TDD (Red-Green-Refactor)

Every feature starts with a failing test. Write the minimum implementation to pass. Refactor if needed. No production code without a preceding test. This is non-negotiable.

### Framework

Catch2 v3 (3.4.0), already installed system-wide.

### VocabManager tests

- Add word → appears in list in correct alphabetical position
- Add duplicate word → list unchanged, returns existing index
- Remove word → gone from list
- Filter words with query → correct subset returned (case-insensitive contains)
- Load valid JSON → correct word list
- Load corrupt JSON → empty list, no crash
- Load missing file → empty list
- Save and reload → round-trip integrity
- Import JSON → replaces list entirely
- Import text → merges without duplicates, maintains sort
- Import text with blank lines / whitespace → handled gracefully
- Export JSON → file matches expected format
- Export text → one word per line, sorted

### NetworkClient tests

- Mock `QNetworkReply` for dictionary API → correct HTML output, phonetic extracted, .mp3 URL extracted
- Mock `QNetworkReply` for translation API → correct Arabic text extracted with RTL wrapper
- Mock 404 response → `requestFailed` emitted with "not found" message
- Mock network error → `requestFailed` emitted with connection error message
- Concurrent requests (definition + translation) → both signals emitted independently

### Not tested (manual only)

- QML UI interactions
- Live API calls

## Technical Constraints

- **Qt version:** 6.8+ (project uses 6.10.2)
- **C++ standard:** C++17
- **Build system:** CMake 3.16+ with Ninja generator
- **Encoding:** UTF-8 project-wide for Arabic support
- **Dependencies:** Qt6::Quick, Qt6::Network, Qt6::Multimedia, Qt6::QuickDialogs2 (for FileDialog), Catch2 v3
