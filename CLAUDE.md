# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

orVocab is a lightweight Qt 6 Quick (QML) desktop vocabulary builder. Users curate a personal word bank with English definitions (via dictionaryapi.dev), Arabic translations (via Google Translate GTX API), and audio pronunciations (QtMultimedia). Data persists to a local `vocab.json` file. Targets Windows 10/11 and Linux.

## Build Commands

```bash
# Configure (from orVocab/ subdirectory)
cmake -B build -DCMAKE_BUILD_TYPE=Debug -G Ninja

# Build
cmake --build build

# Clean
cmake --build build --target clean

# Run
./build/apporVocab

# Run tests (once Catch2 tests exist)
cd build && ctest --output-on-failure
```

**Requirements:** CMake 3.16+, Qt 6.8+ (project uses Qt 6.10.2), Ninja generator.

## Architecture

- **`orVocab/`** — Application source directory containing CMakeLists.txt, C++ sources, and QML files
- **`design/`** — Design artifacts: PRD (`prd.md`), Architecture Decision Records (`ADR.md`), wireframe mockup (`gui.png`), and planned directory structure (`recommended_directory_structure.md`)
- **Entry point:** `main.cpp` creates `QGuiApplication` + `QQmlApplicationEngine`, loads the `orVocab` QML module
- **UI:** `Main.qml` is the root QML component loaded by the engine
- **Build target:** `apporVocab` (executable), defined via `qt_add_executable` + `qt_add_qml_module`
- **QML module URI:** `orVocab`

### Planned Components (per design docs)

- **VocabManager** (C++) — JSON I/O, alphabetical sorting, word bank CRUD. Persists to `vocab.json` via `QStandardPaths::AppDataLocation`.
- **NetworkClient** (C++) — REST calls via `QNetworkAccessManager`. Parses dictionary API responses with `QJsonDocument` (extract `.mp3` from `phonetics` array; extract translation from GTX nested arrays).
- **Sidebar.qml** — Word list with search/add field
- **TranslationView.qml** — RichText display with inline `dir="rtl"` for Arabic

### Key Technical Decisions (from ADR.md)

- Networking: `QNetworkAccessManager` (no Python/external deps)
- Parsing: `QJsonDocument` for nested API responses
- Testing: Catch2 v3 with mocked `QNetworkReply`
- Persistence: `vocab.json` at `QStandardPaths::AppDataLocation`
- Arabic RTL: inline HTML `dir="rtl"` in QML RichText
- Encoding: UTF-8 project-wide

## Conventions

- Qt Creator is the primary IDE (config in `.qtcreator/`)
- 4-space indentation for both C++ and QML
- CMake `qt_add_qml_module()` manages QML file registration — add new QML files to the `QML_FILES` list in CMakeLists.txt
- New C++ classes exposed to QML should be added as sources to the `qt_add_executable` call
