# orVocab Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a Qt 6 QML desktop vocabulary builder with word bank CRUD, REST API lookups (English definitions + Arabic translations), audio pronunciation, and JSON persistence using strict TDD.

**Architecture:** Two C++ QML singletons (VocabManager for data, NetworkClient for APIs) plus three QML components (Main.qml layout, Sidebar.qml word list, TranslationView.qml display). All C++ logic is test-driven with Catch2 v3 before any production code.

**Tech Stack:** C++17, Qt 6.10.2 (Quick, Network, Multimedia, QuickDialogs2), Catch2 v3.4.0, CMake 3.16+, Ninja

---

## File Structure

### C++ Sources (in `orVocab/`)
- `main.cpp` — **Modify**: register singletons, load QML engine
- `vocabmanager.h` — **Create**: VocabManager class declaration
- `vocabmanager.cpp` — **Create**: VocabManager implementation
- `networkclient.h` — **Create**: NetworkClient class declaration
- `networkclient.cpp` — **Create**: NetworkClient implementation

### QML Files (in `orVocab/`)
- `Main.qml` — **Modify**: root layout, toolbar, coordination
- `Sidebar.qml` — **Create**: word list, search/add, context menu
- `TranslationView.qml` — **Create**: definition display, translation, audio

### Tests (in `tests/`)
- `CMakeLists.txt` — **Create**: test executable setup with Catch2
- `test_vocabmanager.cpp` — **Create**: VocabManager unit tests
- `test_networkclient.cpp` — **Create**: NetworkClient unit tests

### Build
- `orVocab/CMakeLists.txt` — **Modify**: add new sources, Qt modules, QML files
- `CMakeLists.txt` (root) — **Create**: top-level CMake to tie app + tests together

---

## Task 1: Project Build Infrastructure

Set up the CMake build so the app compiles with all required Qt modules and Catch2 tests can run.

**Files:**
- Create: `CMakeLists.txt` (root, top-level)
- Modify: `orVocab/CMakeLists.txt`
- Create: `tests/CMakeLists.txt`
- Create: `tests/test_placeholder.cpp`

- [ ] **Step 1: Create root CMakeLists.txt**

This top-level file delegates to the app and test subdirectories.

```cmake
# CMakeLists.txt (root)
cmake_minimum_required(VERSION 3.16)

project(orVocab VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(orVocab)
add_subdirectory(tests)
```

- [ ] **Step 2: Update orVocab/CMakeLists.txt**

Remove the duplicated project-level settings (now in root) and add the new Qt modules.

Replace the entire contents of `orVocab/CMakeLists.txt` with:

```cmake
find_package(Qt6 REQUIRED COMPONENTS Quick Network Multimedia QuickDialogs2)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(apporVocab
    main.cpp
)

qt_add_qml_module(apporVocab
    URI orVocab
    QML_FILES
        Main.qml
)

set_target_properties(apporVocab PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(apporVocab
    PRIVATE
        Qt6::Quick
        Qt6::Network
        Qt6::Multimedia
        Qt6::QuickDialogs2
)

include(GNUInstallDirs)
install(TARGETS apporVocab
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
```

- [ ] **Step 3: Create tests/CMakeLists.txt with Catch2**

Since Catch2 v3 is installed system-wide, use `find_package`.

```cmake
# tests/CMakeLists.txt
find_package(Catch2 3 REQUIRED)

add_executable(orVocabTests
    test_placeholder.cpp
)

target_link_libraries(orVocabTests
    PRIVATE
        Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(orVocabTests)
```

- [ ] **Step 4: Create a placeholder test to verify the pipeline**

```cpp
// tests/test_placeholder.cpp
#include <catch2/catch_all.hpp>

TEST_CASE("Build pipeline works", "[setup]") {
    REQUIRE(1 + 1 == 2);
}
```

- [ ] **Step 5: Build and run tests**

Run:
```bash
cd /mnt/BA707A64707A2773/code/orVocab
cmake -B build -DCMAKE_BUILD_TYPE=Debug -G Ninja
cmake --build build
cd build && ctest --output-on-failure
```

Expected: Build succeeds. 1 test passes.

- [ ] **Step 6: Commit**

```bash
git add CMakeLists.txt orVocab/CMakeLists.txt tests/CMakeLists.txt tests/test_placeholder.cpp
git commit -m "feat(RVC-1): ADDED: build infrastructure with Catch2 test pipeline"
```

---

## Task 2: VocabManager — Add and Remove Words (TDD)

Build the core word list operations: add (alphabetical insert, duplicate detection), remove, and the `words` property.

**Files:**
- Create: `orVocab/vocabmanager.h`
- Create: `orVocab/vocabmanager.cpp`
- Modify: `tests/CMakeLists.txt`
- Create: `tests/test_vocabmanager.cpp`
- Modify: `orVocab/CMakeLists.txt`

- [ ] **Step 1: Write failing tests for addWord and removeWord**

```cpp
// tests/test_vocabmanager.cpp
#include <catch2/catch_all.hpp>
#include <QCoreApplication>
#include "vocabmanager.h"

static int argc = 1;
static char appName[] = "test";
static char *argv[] = {appName, nullptr};

TEST_CASE("VocabManager add and remove words", "[vocabmanager]") {
    QCoreApplication app(argc, argv);
    app.setApplicationName("orVocab-test");

    VocabManager vm;

    SECTION("addWord inserts in alphabetical order") {
        vm.addWord("cherry");
        vm.addWord("apple");
        vm.addWord("banana");
        REQUIRE(vm.words() == QStringList({"apple", "banana", "cherry"}));
    }

    SECTION("addWord returns index of inserted word") {
        int idx = vm.addWord("banana");
        REQUIRE(idx == 0);
        idx = vm.addWord("apple");
        REQUIRE(idx == 0);  // apple sorts before banana
    }

    SECTION("addWord with duplicate returns existing index without adding") {
        vm.addWord("apple");
        vm.addWord("banana");
        int idx = vm.addWord("apple");
        REQUIRE(idx == 0);
        REQUIRE(vm.words().size() == 2);
    }

    SECTION("removeWord removes existing word") {
        vm.addWord("apple");
        vm.addWord("banana");
        vm.removeWord("apple");
        REQUIRE(vm.words() == QStringList({"banana"}));
    }

    SECTION("removeWord with nonexistent word does nothing") {
        vm.addWord("apple");
        vm.removeWord("banana");
        REQUIRE(vm.words() == QStringList({"apple"}));
    }

    SECTION("addWord normalizes to lowercase and trims whitespace") {
        vm.addWord("  Apple  ");
        vm.addWord("BANANA");
        REQUIRE(vm.words() == QStringList({"apple", "banana"}));
    }

    SECTION("addWord rejects empty string") {
        int idx = vm.addWord("");
        REQUIRE(idx == -1);
        idx = vm.addWord("   ");
        REQUIRE(idx == -1);
        REQUIRE(vm.words().isEmpty());
    }
}
```

- [ ] **Step 2: Update tests/CMakeLists.txt to compile VocabManager**

Replace the contents of `tests/CMakeLists.txt`:

```cmake
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Catch2 3 REQUIRED)

add_executable(orVocabTests
    test_placeholder.cpp
    test_vocabmanager.cpp
    ${CMAKE_SOURCE_DIR}/orVocab/vocabmanager.cpp
)

target_include_directories(orVocabTests PRIVATE ${CMAKE_SOURCE_DIR}/orVocab)

target_link_libraries(orVocabTests
    PRIVATE
        Catch2::Catch2WithMain
        Qt6::Core
)

include(CTest)
include(Catch)
catch_discover_tests(orVocabTests)
```

- [ ] **Step 3: Run tests to verify they fail**

Run:
```bash
cd /mnt/BA707A64707A2773/code/orVocab
cmake --build build
```

Expected: FAIL — `vocabmanager.h` not found, compilation error.

- [ ] **Step 4: Create vocabmanager.h with minimal declaration**

```cpp
// orVocab/vocabmanager.h
#ifndef VOCABMANAGER_H
#define VOCABMANAGER_H

#include <QObject>
#include <QStringList>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

class VocabManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QStringList words READ words NOTIFY wordsChanged)
    Q_PROPERTY(QStringList filteredWords READ filteredWords NOTIFY filteredWordsChanged)

public:
    explicit VocabManager(QObject *parent = nullptr);

    QStringList words() const;
    QStringList filteredWords() const;

    Q_INVOKABLE int addWord(const QString &word);
    Q_INVOKABLE void removeWord(const QString &word);
    Q_INVOKABLE void filterWords(const QString &query);

    Q_INVOKABLE void loadFromJson();
    Q_INVOKABLE void saveToJson();

    Q_INVOKABLE void exportJson(const QUrl &path);
    Q_INVOKABLE void exportText(const QUrl &path);
    Q_INVOKABLE void importJson(const QUrl &path);
    Q_INVOKABLE void importText(const QUrl &path);

signals:
    void wordsChanged();
    void filteredWordsChanged();

private:
    QStringList m_words;
    QStringList m_filteredWords;
    QString m_currentFilter;

    QString dataFilePath() const;
    void updateFilteredWords();
};

#endif // VOCABMANAGER_H
```

- [ ] **Step 5: Create vocabmanager.cpp with add/remove implementation**

```cpp
// orVocab/vocabmanager.cpp
#include "vocabmanager.h"
#include <algorithm>

VocabManager::VocabManager(QObject *parent)
    : QObject(parent)
{
}

QStringList VocabManager::words() const
{
    return m_words;
}

QStringList VocabManager::filteredWords() const
{
    return m_filteredWords;
}

int VocabManager::addWord(const QString &word)
{
    QString normalized = word.trimmed().toLower();
    if (normalized.isEmpty())
        return -1;

    auto it = std::lower_bound(m_words.begin(), m_words.end(), normalized);
    int idx = static_cast<int>(it - m_words.begin());

    if (it != m_words.end() && *it == normalized)
        return idx;

    m_words.insert(idx, normalized);
    updateFilteredWords();
    emit wordsChanged();
    return idx;
}

void VocabManager::removeWord(const QString &word)
{
    QString normalized = word.trimmed().toLower();
    if (m_words.removeOne(normalized)) {
        updateFilteredWords();
        emit wordsChanged();
    }
}

void VocabManager::filterWords(const QString &query)
{
    m_currentFilter = query;
    updateFilteredWords();
}

void VocabManager::updateFilteredWords()
{
    QStringList filtered;
    if (m_currentFilter.isEmpty()) {
        filtered = m_words;
    } else {
        for (const QString &w : m_words) {
            if (w.contains(m_currentFilter, Qt::CaseInsensitive))
                filtered.append(w);
        }
    }
    if (filtered != m_filteredWords) {
        m_filteredWords = filtered;
        emit filteredWordsChanged();
    }
}

void VocabManager::loadFromJson() {}
void VocabManager::saveToJson() {}
void VocabManager::exportJson(const QUrl &) {}
void VocabManager::exportText(const QUrl &) {}
void VocabManager::importJson(const QUrl &) {}
void VocabManager::importText(const QUrl &) {}
QString VocabManager::dataFilePath() const { return QString(); }
```

- [ ] **Step 6: Add vocabmanager to orVocab/CMakeLists.txt**

In `orVocab/CMakeLists.txt`, update the `qt_add_executable` call:

```cmake
qt_add_executable(apporVocab
    main.cpp
    vocabmanager.h
    vocabmanager.cpp
)
```

- [ ] **Step 7: Build and run tests**

Run:
```bash
cd /mnt/BA707A64707A2773/code/orVocab
cmake -B build -DCMAKE_BUILD_TYPE=Debug -G Ninja
cmake --build build
cd build && ctest --output-on-failure
```

Expected: All VocabManager add/remove tests PASS.

- [ ] **Step 8: Commit**

```bash
git add orVocab/vocabmanager.h orVocab/vocabmanager.cpp orVocab/CMakeLists.txt tests/CMakeLists.txt tests/test_vocabmanager.cpp
git commit -m "feat(RVC-1): ADDED: VocabManager add/remove with TDD"
```

---

## Task 3: VocabManager — Filter Words (TDD)

**Files:**
- Modify: `tests/test_vocabmanager.cpp`

- [ ] **Step 1: Write failing tests for filterWords**

Append to `tests/test_vocabmanager.cpp`:

```cpp
TEST_CASE("VocabManager filter words", "[vocabmanager]") {
    QCoreApplication app(argc, argv);
    app.setApplicationName("orVocab-test");

    VocabManager vm;
    vm.addWord("algorithm");
    vm.addWord("allocate");
    vm.addWord("binary");
    vm.addWord("cache");

    SECTION("empty filter returns all words") {
        vm.filterWords("");
        REQUIRE(vm.filteredWords() == QStringList({"algorithm", "allocate", "binary", "cache"}));
    }

    SECTION("filter narrows list with case-insensitive contains") {
        vm.filterWords("al");
        REQUIRE(vm.filteredWords() == QStringList({"algorithm", "allocate"}));
    }

    SECTION("filter with no matches returns empty list") {
        vm.filterWords("xyz");
        REQUIRE(vm.filteredWords().isEmpty());
    }

    SECTION("filter is case-insensitive") {
        vm.filterWords("AL");
        REQUIRE(vm.filteredWords() == QStringList({"algorithm", "allocate"}));
    }

    SECTION("filteredWords updates when word is added matching current filter") {
        vm.filterWords("al");
        vm.addWord("alpha");
        REQUIRE(vm.filteredWords() == QStringList({"algorithm", "allocate", "alpha"}));
    }

    SECTION("filteredWords updates when matching word is removed") {
        vm.filterWords("al");
        vm.removeWord("algorithm");
        REQUIRE(vm.filteredWords() == QStringList({"allocate"}));
    }
}
```

- [ ] **Step 2: Build and run tests**

Run:
```bash
cd /mnt/BA707A64707A2773/code/orVocab
cmake --build build
cd build && ctest --output-on-failure
```

Expected: All filter tests PASS (implementation already exists from Task 2 — this validates it).

- [ ] **Step 3: Commit**

```bash
git add tests/test_vocabmanager.cpp
git commit -m "test(RVC-1): ADDED: VocabManager filter tests"
```

---

## Task 4: VocabManager — JSON Persistence (TDD)

Implement load/save with `QStandardPaths`, handling missing files and corrupt JSON.

**Files:**
- Modify: `tests/test_vocabmanager.cpp`
- Modify: `orVocab/vocabmanager.cpp`

- [ ] **Step 1: Write failing tests for JSON persistence**

Append to `tests/test_vocabmanager.cpp`:

```cpp
#include <QDir>
#include <QFile>
#include <QStandardPaths>

TEST_CASE("VocabManager JSON persistence", "[vocabmanager][json]") {
    QCoreApplication app(argc, argv);
    app.setApplicationName("orVocab-test");

    // Use test-specific data location to avoid polluting real data
    QStandardPaths::setTestModeEnabled(true);

    // Clean up any leftover test data
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir(dataDir).removeRecursively();

    SECTION("loadFromJson with missing file starts empty") {
        VocabManager vm;
        vm.loadFromJson();
        REQUIRE(vm.words().isEmpty());
    }

    SECTION("saveToJson then loadFromJson round-trips correctly") {
        {
            VocabManager vm;
            vm.addWord("cherry");
            vm.addWord("apple");
            vm.addWord("banana");
            vm.saveToJson();
        }
        {
            VocabManager vm;
            vm.loadFromJson();
            REQUIRE(vm.words() == QStringList({"apple", "banana", "cherry"}));
        }
    }

    SECTION("loadFromJson with corrupt JSON starts empty") {
        QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dataDir);
        QFile file(dataDir + "/vocab.json");
        file.open(QIODevice::WriteOnly);
        file.write("not valid json {{{");
        file.close();

        VocabManager vm;
        vm.loadFromJson();
        REQUIRE(vm.words().isEmpty());
    }

    SECTION("addWord auto-saves to JSON") {
        {
            VocabManager vm;
            vm.loadFromJson();
            vm.addWord("apple");
            vm.addWord("banana");
        }
        {
            VocabManager vm;
            vm.loadFromJson();
            REQUIRE(vm.words() == QStringList({"apple", "banana"}));
        }
    }

    SECTION("removeWord auto-saves to JSON") {
        {
            VocabManager vm;
            vm.loadFromJson();
            vm.addWord("apple");
            vm.addWord("banana");
        }
        {
            VocabManager vm;
            vm.loadFromJson();
            vm.removeWord("apple");
        }
        {
            VocabManager vm;
            vm.loadFromJson();
            REQUIRE(vm.words() == QStringList({"banana"}));
        }
    }

    // Clean up
    QDir(dataDir).removeRecursively();
    QStandardPaths::setTestModeEnabled(false);
}
```

- [ ] **Step 2: Run tests to verify they fail**

Run:
```bash
cd /mnt/BA707A64707A2773/code/orVocab
cmake --build build
cd build && ctest --output-on-failure
```

Expected: FAIL — `saveToJson` and `loadFromJson` are stubs.

- [ ] **Step 3: Implement loadFromJson and saveToJson**

Replace the stub implementations in `orVocab/vocabmanager.cpp`:

```cpp
// Add these includes at the top
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

QString VocabManager::dataFilePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return dir + "/vocab.json";
}

void VocabManager::loadFromJson()
{
    QFile file(dataFilePath());
    if (!file.exists())
        return;

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("VocabManager: cannot open %s for reading", qPrintable(dataFilePath()));
        return;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError) {
        qWarning("VocabManager: corrupt JSON in %s: %s",
                 qPrintable(dataFilePath()),
                 qPrintable(parseError.errorString()));
        return;
    }

    QJsonArray arr = doc.object().value("words").toArray();
    m_words.clear();
    for (const QJsonValue &v : arr)
        m_words.append(v.toString());

    updateFilteredWords();
    emit wordsChanged();
}

void VocabManager::saveToJson()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);

    QFile file(dataFilePath());
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning("VocabManager: cannot open %s for writing", qPrintable(dataFilePath()));
        return;
    }

    QJsonArray arr;
    for (const QString &w : m_words)
        arr.append(w);

    QJsonObject obj;
    obj["words"] = arr;

    file.write(QJsonDocument(obj).toJson());
    file.close();
}
```

- [ ] **Step 4: Add auto-save calls to addWord and removeWord**

In `orVocab/vocabmanager.cpp`, update `addWord` — add `saveToJson()` after inserting:

```cpp
int VocabManager::addWord(const QString &word)
{
    QString normalized = word.trimmed().toLower();
    if (normalized.isEmpty())
        return -1;

    auto it = std::lower_bound(m_words.begin(), m_words.end(), normalized);
    int idx = static_cast<int>(it - m_words.begin());

    if (it != m_words.end() && *it == normalized)
        return idx;

    m_words.insert(idx, normalized);
    saveToJson();
    updateFilteredWords();
    emit wordsChanged();
    return idx;
}
```

Update `removeWord` — add `saveToJson()` after removing:

```cpp
void VocabManager::removeWord(const QString &word)
{
    QString normalized = word.trimmed().toLower();
    if (m_words.removeOne(normalized)) {
        saveToJson();
        updateFilteredWords();
        emit wordsChanged();
    }
}
```

- [ ] **Step 5: Build and run tests**

Run:
```bash
cd /mnt/BA707A64707A2773/code/orVocab
cmake --build build
cd build && ctest --output-on-failure
```

Expected: All JSON persistence tests PASS.

- [ ] **Step 6: Commit**

```bash
git add orVocab/vocabmanager.cpp tests/test_vocabmanager.cpp
git commit -m "feat(RVC-1): ADDED: VocabManager JSON persistence with auto-save"
```

---

## Task 5: VocabManager — Import and Export (TDD)

**Files:**
- Modify: `tests/test_vocabmanager.cpp`
- Modify: `orVocab/vocabmanager.cpp`

- [ ] **Step 1: Write failing tests for export/import**

Append to `tests/test_vocabmanager.cpp`:

```cpp
#include <QTemporaryDir>

TEST_CASE("VocabManager export and import", "[vocabmanager][io]") {
    QCoreApplication app(argc, argv);
    app.setApplicationName("orVocab-test");

    QStandardPaths::setTestModeEnabled(true);
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir(dataDir).removeRecursively();

    QTemporaryDir tmpDir;
    REQUIRE(tmpDir.isValid());

    SECTION("exportJson writes correct JSON to target path") {
        VocabManager vm;
        vm.addWord("banana");
        vm.addWord("apple");

        QString exportPath = tmpDir.path() + "/exported.json";
        vm.exportJson(QUrl::fromLocalFile(exportPath));

        QFile file(exportPath);
        REQUIRE(file.open(QIODevice::ReadOnly));
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonArray arr = doc.object().value("words").toArray();
        REQUIRE(arr.size() == 2);
        REQUIRE(arr[0].toString() == "apple");
        REQUIRE(arr[1].toString() == "banana");
    }

    SECTION("exportText writes one word per line") {
        VocabManager vm;
        vm.addWord("cherry");
        vm.addWord("apple");

        QString exportPath = tmpDir.path() + "/exported.txt";
        vm.exportText(QUrl::fromLocalFile(exportPath));

        QFile file(exportPath);
        REQUIRE(file.open(QIODevice::ReadOnly));
        QString content = QString::fromUtf8(file.readAll());
        REQUIRE(content == "apple\ncherry\n");
    }

    SECTION("importJson replaces entire word bank") {
        VocabManager vm;
        vm.addWord("old_word");

        // Write a JSON file to import
        QString importPath = tmpDir.path() + "/import.json";
        QFile file(importPath);
        file.open(QIODevice::WriteOnly);
        file.write(R"({"words": ["delta", "alpha", "gamma"]})");
        file.close();

        vm.importJson(QUrl::fromLocalFile(importPath));
        REQUIRE(vm.words() == QStringList({"alpha", "delta", "gamma"}));
    }

    SECTION("importJson with corrupt file leaves existing list untouched") {
        VocabManager vm;
        vm.addWord("apple");

        QString importPath = tmpDir.path() + "/bad.json";
        QFile file(importPath);
        file.open(QIODevice::WriteOnly);
        file.write("not json {{{");
        file.close();

        vm.importJson(QUrl::fromLocalFile(importPath));
        REQUIRE(vm.words() == QStringList({"apple"}));
    }

    SECTION("importText merges without duplicates") {
        VocabManager vm;
        vm.addWord("apple");
        vm.addWord("cherry");

        QString importPath = tmpDir.path() + "/import.txt";
        QFile file(importPath);
        file.open(QIODevice::WriteOnly);
        file.write("banana\napple\ndate\n");
        file.close();

        vm.importText(QUrl::fromLocalFile(importPath));
        REQUIRE(vm.words() == QStringList({"apple", "banana", "cherry", "date"}));
    }

    SECTION("importText handles blank lines and whitespace") {
        VocabManager vm;

        QString importPath = tmpDir.path() + "/messy.txt";
        QFile file(importPath);
        file.open(QIODevice::WriteOnly);
        file.write("  apple  \n\n  banana \n   \ncherry\n");
        file.close();

        vm.importText(QUrl::fromLocalFile(importPath));
        REQUIRE(vm.words() == QStringList({"apple", "banana", "cherry"}));
    }

    QDir(dataDir).removeRecursively();
    QStandardPaths::setTestModeEnabled(false);
}
```

- [ ] **Step 2: Run tests to verify they fail**

Run:
```bash
cd /mnt/BA707A64707A2773/code/orVocab
cmake --build build
cd build && ctest --output-on-failure
```

Expected: FAIL — export/import methods are stubs.

- [ ] **Step 3: Implement export and import methods**

Replace the stubs in `orVocab/vocabmanager.cpp`:

```cpp
void VocabManager::exportJson(const QUrl &path)
{
    QFile file(path.toLocalFile());
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning("VocabManager: cannot write to %s", qPrintable(path.toLocalFile()));
        return;
    }

    QJsonArray arr;
    for (const QString &w : m_words)
        arr.append(w);

    QJsonObject obj;
    obj["words"] = arr;
    file.write(QJsonDocument(obj).toJson());
    file.close();
}

void VocabManager::exportText(const QUrl &path)
{
    QFile file(path.toLocalFile());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning("VocabManager: cannot write to %s", qPrintable(path.toLocalFile()));
        return;
    }

    for (const QString &w : m_words) {
        file.write(w.toUtf8());
        file.write("\n");
    }
    file.close();
}

void VocabManager::importJson(const QUrl &path)
{
    QFile file(path.toLocalFile());
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("VocabManager: cannot read %s", qPrintable(path.toLocalFile()));
        return;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError) {
        qWarning("VocabManager: corrupt JSON in import file: %s",
                 qPrintable(parseError.errorString()));
        return;
    }

    QJsonArray arr = doc.object().value("words").toArray();
    QStringList newWords;
    for (const QJsonValue &v : arr)
        newWords.append(v.toString().trimmed().toLower());

    newWords.sort();
    m_words = newWords;
    saveToJson();
    updateFilteredWords();
    emit wordsChanged();
}

void VocabManager::importText(const QUrl &path)
{
    QFile file(path.toLocalFile());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("VocabManager: cannot read %s", qPrintable(path.toLocalFile()));
        return;
    }

    while (!file.atEnd()) {
        QString line = QString::fromUtf8(file.readLine()).trimmed().toLower();
        if (line.isEmpty())
            continue;

        auto it = std::lower_bound(m_words.begin(), m_words.end(), line);
        if (it == m_words.end() || *it != line)
            m_words.insert(it, line);
    }
    file.close();

    saveToJson();
    updateFilteredWords();
    emit wordsChanged();
}
```

- [ ] **Step 4: Build and run tests**

Run:
```bash
cd /mnt/BA707A64707A2773/code/orVocab
cmake --build build
cd build && ctest --output-on-failure
```

Expected: All export/import tests PASS.

- [ ] **Step 5: Commit**

```bash
git add orVocab/vocabmanager.cpp tests/test_vocabmanager.cpp
git commit -m "feat(RVC-1): ADDED: VocabManager import/export (JSON + text)"
```

---

## Task 6: NetworkClient — Dictionary API Parsing (TDD)

Build the NetworkClient with mocked responses. Test dictionary API JSON parsing, HTML formatting, phonetic extraction, and audio URL extraction.

**Files:**
- Create: `orVocab/networkclient.h`
- Create: `orVocab/networkclient.cpp`
- Create: `tests/test_networkclient.cpp`
- Modify: `tests/CMakeLists.txt`
- Modify: `orVocab/CMakeLists.txt`

- [ ] **Step 1: Write failing tests for dictionary response parsing**

The NetworkClient uses QNetworkAccessManager internally, which is hard to mock at the network layer. Instead, we extract the parsing logic into testable static/private methods and test those directly.

```cpp
// tests/test_networkclient.cpp
#include <catch2/catch_all.hpp>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include "networkclient.h"

static int argc = 1;
static char appName[] = "test";
static char *argv[] = {appName, nullptr};

TEST_CASE("NetworkClient dictionary parsing", "[networkclient][dictionary]") {
    QCoreApplication app(argc, argv);

    // Sample dictionary API response for "hello"
    QByteArray dictResponse = R"([
        {
            "word": "hello",
            "phonetic": "/həˈloʊ/",
            "phonetics": [
                { "text": "/həˈloʊ/" },
                { "text": "/həˈloʊ/", "audio": "https://api.dictionaryapi.dev/media/pronunciations/en/hello-us.mp3" }
            ],
            "meanings": [
                {
                    "partOfSpeech": "noun",
                    "definitions": [
                        {
                            "definition": "An utterance of \"hello\"; a greeting.",
                            "example": "she was getting hellos from everyone"
                        }
                    ]
                },
                {
                    "partOfSpeech": "interjection",
                    "definitions": [
                        {
                            "definition": "Used as a greeting."
                        }
                    ]
                }
            ]
        }
    ])";

    SECTION("parseDictionaryResponse extracts formatted HTML") {
        auto result = NetworkClient::parseDictionaryResponse(dictResponse);
        REQUIRE_FALSE(result.html.isEmpty());
        REQUIRE(result.html.contains("noun"));
        REQUIRE(result.html.contains("interjection"));
        REQUIRE(result.html.contains("An utterance of"));
        REQUIRE(result.html.contains("she was getting hellos"));
    }

    SECTION("parseDictionaryResponse extracts phonetic text") {
        auto result = NetworkClient::parseDictionaryResponse(dictResponse);
        REQUIRE(result.phonetic == "/həˈloʊ/");
    }

    SECTION("parseDictionaryResponse extracts first valid mp3 URL") {
        auto result = NetworkClient::parseDictionaryResponse(dictResponse);
        REQUIRE(result.audioUrl == QUrl("https://api.dictionaryapi.dev/media/pronunciations/en/hello-us.mp3"));
    }

    SECTION("parseDictionaryResponse with no audio returns empty URL") {
        QByteArray noAudio = R"([{
            "word": "test",
            "phonetics": [{ "text": "/tɛst/" }],
            "meanings": [{ "partOfSpeech": "noun", "definitions": [{ "definition": "A trial." }] }]
        }])";

        auto result = NetworkClient::parseDictionaryResponse(noAudio);
        REQUIRE(result.audioUrl.isEmpty());
    }

    SECTION("parseDictionaryResponse with empty data returns error") {
        auto result = NetworkClient::parseDictionaryResponse("{}");
        REQUIRE(result.html.isEmpty());
        REQUIRE(result.error == true);
    }
}
```

- [ ] **Step 2: Update tests/CMakeLists.txt**

Add NetworkClient source and Qt6::Network:

```cmake
find_package(Qt6 REQUIRED COMPONENTS Core Network)
find_package(Catch2 3 REQUIRED)

add_executable(orVocabTests
    test_placeholder.cpp
    test_vocabmanager.cpp
    test_networkclient.cpp
    ${CMAKE_SOURCE_DIR}/orVocab/vocabmanager.cpp
    ${CMAKE_SOURCE_DIR}/orVocab/networkclient.cpp
)

target_include_directories(orVocabTests PRIVATE ${CMAKE_SOURCE_DIR}/orVocab)

target_link_libraries(orVocabTests
    PRIVATE
        Catch2::Catch2WithMain
        Qt6::Core
        Qt6::Network
)

include(CTest)
include(Catch)
catch_discover_tests(orVocabTests)
```

- [ ] **Step 3: Run tests to verify they fail**

Run:
```bash
cd /mnt/BA707A64707A2773/code/orVocab
cmake --build build
```

Expected: FAIL — `networkclient.h` not found.

- [ ] **Step 4: Create networkclient.h**

```cpp
// orVocab/networkclient.h
#ifndef NETWORKCLIENT_H
#define NETWORKCLIENT_H

#include <QObject>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

class QNetworkAccessManager;

struct DictionaryResult {
    QString html;
    QString phonetic;
    QUrl audioUrl;
    bool error = false;
};

struct TranslationResult {
    QString html;
    bool error = false;
};

class NetworkClient : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit NetworkClient(QObject *parent = nullptr);

    Q_INVOKABLE void fetchDefinition(const QString &word);
    Q_INVOKABLE void fetchTranslation(const QString &word);

    static DictionaryResult parseDictionaryResponse(const QByteArray &data);
    static TranslationResult parseTranslationResponse(const QByteArray &data);

signals:
    void definitionReady(const QString &html);
    void phoneticReady(const QString &phonetic);
    void audioUrlReady(const QUrl &url);
    void translationReady(const QString &html);
    void requestFailed(const QString &area, const QString &errorString);

private:
    QNetworkAccessManager *m_nam;
};

#endif // NETWORKCLIENT_H
```

- [ ] **Step 5: Create networkclient.cpp with parsing implementation**

```cpp
// orVocab/networkclient.cpp
#include "networkclient.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

NetworkClient::NetworkClient(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

DictionaryResult NetworkClient::parseDictionaryResponse(const QByteArray &data)
{
    DictionaryResult result;

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray() || doc.array().isEmpty()) {
        result.error = true;
        return result;
    }

    QJsonObject entry = doc.array().first().toObject();

    // Extract phonetic
    result.phonetic = entry.value("phonetic").toString();

    // Extract audio URL — find first phonetics entry with a non-empty .mp3 audio
    QJsonArray phonetics = entry.value("phonetics").toArray();
    for (const QJsonValue &p : phonetics) {
        QString audio = p.toObject().value("audio").toString();
        if (!audio.isEmpty() && audio.endsWith(".mp3")) {
            result.audioUrl = QUrl(audio);
            break;
        }
    }

    // If phonetic was empty, try from phonetics array
    if (result.phonetic.isEmpty()) {
        for (const QJsonValue &p : phonetics) {
            QString text = p.toObject().value("text").toString();
            if (!text.isEmpty()) {
                result.phonetic = text;
                break;
            }
        }
    }

    // Build HTML from meanings
    QString html;
    QJsonArray meanings = entry.value("meanings").toArray();
    for (const QJsonValue &m : meanings) {
        QJsonObject meaning = m.toObject();
        QString pos = meaning.value("partOfSpeech").toString();
        html += "<p><b>" + pos + "</b></p>";

        QJsonArray definitions = meaning.value("definitions").toArray();
        for (const QJsonValue &d : definitions) {
            QJsonObject def = d.toObject();
            html += "<p>" + def.value("definition").toString() + "</p>";
            QString example = def.value("example").toString();
            if (!example.isEmpty())
                html += "<p><i>\"" + example + "\"</i></p>";
        }
    }

    if (html.isEmpty())
        result.error = true;

    result.html = html;
    return result;
}

void NetworkClient::fetchDefinition(const QString &word)
{
    QUrl url("https://api.dictionaryapi.dev/api/v2/entries/en/" + word);
    QNetworkReply *reply = m_nam->get(QNetworkRequest(url));

    connect(reply, &QNetworkReply::finished, this, [this, reply, word]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::ContentNotFoundError)
                emit requestFailed("definition", QString("No definition found for '%1'.").arg(word));
            else
                emit requestFailed("definition", "Could not fetch definition. Check your connection.");
            return;
        }

        auto result = parseDictionaryResponse(reply->readAll());
        if (result.error) {
            emit requestFailed("definition", QString("No definition found for '%1'.").arg(word));
            return;
        }

        emit definitionReady(result.html);
        if (!result.phonetic.isEmpty())
            emit phoneticReady(result.phonetic);
        if (!result.audioUrl.isEmpty())
            emit audioUrlReady(result.audioUrl);
    });
}

void NetworkClient::fetchTranslation(const QString &) {}

TranslationResult NetworkClient::parseTranslationResponse(const QByteArray &)
{
    return TranslationResult{};
}
```

- [ ] **Step 6: Add networkclient to orVocab/CMakeLists.txt**

Update `qt_add_executable`:

```cmake
qt_add_executable(apporVocab
    main.cpp
    vocabmanager.h
    vocabmanager.cpp
    networkclient.h
    networkclient.cpp
)
```

- [ ] **Step 7: Build and run tests**

Run:
```bash
cd /mnt/BA707A64707A2773/code/orVocab
cmake -B build -DCMAKE_BUILD_TYPE=Debug -G Ninja
cmake --build build
cd build && ctest --output-on-failure
```

Expected: All dictionary parsing tests PASS.

- [ ] **Step 8: Commit**

```bash
git add orVocab/networkclient.h orVocab/networkclient.cpp orVocab/CMakeLists.txt tests/test_networkclient.cpp tests/CMakeLists.txt
git commit -m "feat(RVC-1): ADDED: NetworkClient with dictionary API parsing (TDD)"
```

---

## Task 7: NetworkClient — Translation API Parsing (TDD)

**Files:**
- Modify: `tests/test_networkclient.cpp`
- Modify: `orVocab/networkclient.cpp`

- [ ] **Step 1: Write failing tests for translation parsing**

Append to `tests/test_networkclient.cpp`:

```cpp
TEST_CASE("NetworkClient translation parsing", "[networkclient][translation]") {
    QCoreApplication app(argc, argv);

    // Sample GTX translation API response for "hello" → Arabic
    QByteArray translationResponse = R"([[["مرحبا","hello",null,null,10]],null,"en"])";

    SECTION("parseTranslationResponse extracts Arabic text with RTL HTML") {
        auto result = NetworkClient::parseTranslationResponse(translationResponse);
        REQUIRE_FALSE(result.html.isEmpty());
        REQUIRE(result.html.contains("مرحبا"));
        REQUIRE(result.html.contains("dir=\"rtl\""));
    }

    SECTION("parseTranslationResponse with empty data returns error") {
        auto result = NetworkClient::parseTranslationResponse("[]");
        REQUIRE(result.error == true);
    }

    SECTION("parseTranslationResponse with invalid JSON returns error") {
        auto result = NetworkClient::parseTranslationResponse("not json");
        REQUIRE(result.error == true);
    }
}
```

- [ ] **Step 2: Run tests to verify they fail**

Run:
```bash
cd /mnt/BA707A64707A2773/code/orVocab
cmake --build build
cd build && ctest --output-on-failure
```

Expected: FAIL — `parseTranslationResponse` is a stub.

- [ ] **Step 3: Implement parseTranslationResponse and fetchTranslation**

Replace the stubs in `orVocab/networkclient.cpp`:

```cpp
TranslationResult NetworkClient::parseTranslationResponse(const QByteArray &data)
{
    TranslationResult result;

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        result.error = true;
        return result;
    }

    // GTX response structure: [[["translated","original",...],...],...,"source_lang"]
    QJsonArray root = doc.array();
    if (root.isEmpty() || !root[0].isArray()) {
        result.error = true;
        return result;
    }

    QJsonArray translations = root[0].toArray();
    if (translations.isEmpty() || !translations[0].isArray()) {
        result.error = true;
        return result;
    }

    QString translated = translations[0].toArray()[0].toString();
    if (translated.isEmpty()) {
        result.error = true;
        return result;
    }

    result.html = "<p dir=\"rtl\" style=\"font-size: 20px;\">" + translated + "</p>";
    return result;
}

void NetworkClient::fetchTranslation(const QString &word)
{
    QUrl url("https://translate.googleapis.com/translate_a/single");
    QUrlQuery query;
    query.addQueryItem("client", "gtx");
    query.addQueryItem("sl", "en");
    query.addQueryItem("tl", "ar");
    query.addQueryItem("dt", "t");
    query.addQueryItem("q", word);
    url.setQuery(query);

    QNetworkReply *reply = m_nam->get(QNetworkRequest(url));

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit requestFailed("translation", "Could not fetch translation.");
            return;
        }

        auto result = parseTranslationResponse(reply->readAll());
        if (result.error) {
            emit requestFailed("translation", "Could not fetch translation.");
            return;
        }

        emit translationReady(result.html);
    });
}
```

Add this include at the top of `networkclient.cpp`:

```cpp
#include <QUrlQuery>
```

- [ ] **Step 4: Build and run tests**

Run:
```bash
cd /mnt/BA707A64707A2773/code/orVocab
cmake --build build
cd build && ctest --output-on-failure
```

Expected: All translation parsing tests PASS.

- [ ] **Step 5: Commit**

```bash
git add orVocab/networkclient.cpp tests/test_networkclient.cpp
git commit -m "feat(RVC-1): ADDED: NetworkClient translation API parsing (TDD)"
```

---

## Task 8: QML — Main Layout and Sidebar

Build the QML UI: Main.qml with toolbar and two-panel layout, Sidebar.qml with word list, search/add, and context menu.

**Files:**
- Modify: `orVocab/Main.qml`
- Create: `orVocab/Sidebar.qml`
- Modify: `orVocab/CMakeLists.txt`
- Modify: `orVocab/main.cpp`

- [ ] **Step 1: Update main.cpp to register singletons**

```cpp
// orVocab/main.cpp
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "vocabmanager.h"
#include "networkclient.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName("orVocab");
    app.setApplicationName("orVocab");

    VocabManager vocabManager;
    vocabManager.loadFromJson();

    NetworkClient networkClient;

    QQmlApplicationEngine engine;

    qmlRegisterSingletonInstance("orVocab", 1, 0, "VocabManager", &vocabManager);
    qmlRegisterSingletonInstance("orVocab", 1, 0, "NetworkClient", &networkClient);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("orVocab", "Main");

    return app.exec();
}
```

- [ ] **Step 2: Create Sidebar.qml**

```qml
// orVocab/Sidebar.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: sidebar
    color: palette.base
    border.color: palette.mid
    border.width: 0

    signal wordSelected(string word)
    signal wordDoubleClicked(string word)

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 8
            spacing: 4

            TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: "Search or add word..."
                onTextChanged: VocabManager.filterWords(text)
            }

            Button {
                text: "Add"
                onClicked: {
                    if (searchField.text.trim() === "")
                        return;
                    var idx = VocabManager.addWord(searchField.text);
                    if (idx >= 0) {
                        wordList.currentIndex = idx;
                        wordSelected(VocabManager.filteredWords[idx]);
                    }
                    searchField.text = "";
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: palette.mid
        }

        ListView {
            id: wordList
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: VocabManager.filteredWords
            clip: true
            currentIndex: -1

            delegate: ItemDelegate {
                width: wordList.width
                text: modelData
                highlighted: wordList.currentIndex === index

                onDoubleClicked: {
                    wordList.currentIndex = index;
                    sidebar.wordDoubleClicked(modelData);
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.RightButton
                    onClicked: function(mouse) {
                        wordList.currentIndex = index;
                        contextMenu.selectedWord = modelData;
                        contextMenu.popup();
                    }
                }
            }

            Menu {
                id: contextMenu
                property string selectedWord: ""

                MenuItem {
                    text: "Delete"
                    onTriggered: VocabManager.removeWord(contextMenu.selectedWord)
                }
            }
        }
    }
}
```

- [ ] **Step 3: Update Main.qml with toolbar and two-panel layout**

```qml
// orVocab/Main.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    id: root
    width: 900
    height: 600
    visible: true
    title: "orVocab"

    property string currentWord: ""

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8

            Label {
                text: "orVocab"
                font.bold: true
                Layout.fillWidth: true
            }

            Button {
                text: "Import JSON"
                onClicked: importJsonDialog.open()
            }
            Button {
                text: "Import Text"
                onClicked: importTextDialog.open()
            }
            Button {
                text: "Export JSON"
                onClicked: exportJsonDialog.open()
            }
            Button {
                text: "Export Text"
                onClicked: exportTextDialog.open()
            }
        }
    }

    SplitView {
        anchors.fill: parent

        Sidebar {
            SplitView.preferredWidth: 220
            SplitView.minimumWidth: 150

            onWordDoubleClicked: function(word) {
                root.currentWord = word;
                translationView.lookupWord(word);
            }
        }

        TranslationView {
            id: translationView
            SplitView.fillWidth: true
            currentWord: root.currentWord
        }
    }

    // File dialogs
    FileDialog {
        id: importJsonDialog
        title: "Import JSON Word List"
        nameFilters: ["JSON files (*.json)"]
        onAccepted: confirmImportDialog.open()
    }

    FileDialog {
        id: importTextDialog
        title: "Import Text Word List"
        nameFilters: ["Text files (*.txt)"]
        onAccepted: VocabManager.importText(importTextDialog.selectedFile)
    }

    FileDialog {
        id: exportJsonDialog
        title: "Export JSON Word List"
        fileMode: FileDialog.SaveFile
        nameFilters: ["JSON files (*.json)"]
        onAccepted: VocabManager.exportJson(exportJsonDialog.selectedFile)
    }

    FileDialog {
        id: exportTextDialog
        title: "Export Text Word List"
        fileMode: FileDialog.SaveFile
        nameFilters: ["Text files (*.txt)"]
        onAccepted: VocabManager.exportText(exportTextDialog.selectedFile)
    }

    Dialog {
        id: confirmImportDialog
        title: "Confirm Import"
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Ok | Dialog.Cancel

        Label {
            text: "This will replace your current word list of "
                  + VocabManager.words.length + " words. Continue?"
        }

        onAccepted: VocabManager.importJson(importJsonDialog.selectedFile)
    }
}
```

- [ ] **Step 4: Update orVocab/CMakeLists.txt with new QML files**

```cmake
qt_add_qml_module(apporVocab
    URI orVocab
    QML_FILES
        Main.qml
        Sidebar.qml
)
```

- [ ] **Step 5: Build and verify app launches**

Run:
```bash
cd /mnt/BA707A64707A2773/code/orVocab
cmake -B build -DCMAKE_BUILD_TYPE=Debug -G Ninja
cmake --build build
./build/apporVocab
```

Expected: App launches with toolbar, sidebar with search/add field, and empty right panel. You can type a word in the search field and click Add. The word appears in the sidebar list. Right-clicking shows a Delete option.

- [ ] **Step 6: Commit**

```bash
git add orVocab/main.cpp orVocab/Main.qml orVocab/Sidebar.qml orVocab/CMakeLists.txt
git commit -m "feat(RVC-1): ADDED: Main layout with toolbar and Sidebar component"
```

---

## Task 9: QML — TranslationView with Audio

Build the right panel: word display, definition area, Arabic translation area, and audio playback.

**Files:**
- Create: `orVocab/TranslationView.qml`
- Modify: `orVocab/CMakeLists.txt`

- [ ] **Step 1: Create TranslationView.qml**

```qml
// orVocab/TranslationView.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

Rectangle {
    id: translationView
    color: "transparent"

    property string currentWord: ""
    property string phonetic: ""
    property string definitionHtml: ""
    property string translationHtml: ""
    property url audioSource: ""
    property string definitionError: ""
    property string translationError: ""

    function lookupWord(word) {
        currentWord = word;
        phonetic = "";
        definitionHtml = "";
        translationHtml = "";
        audioSource = "";
        definitionError = "";
        translationError = "";
        NetworkClient.fetchDefinition(word);
        NetworkClient.fetchTranslation(word);
    }

    Connections {
        target: NetworkClient

        function onDefinitionReady(html) {
            translationView.definitionHtml = html;
            translationView.definitionError = "";
        }

        function onPhoneticReady(phon) {
            translationView.phonetic = phon;
        }

        function onAudioUrlReady(url) {
            translationView.audioSource = url;
        }

        function onTranslationReady(html) {
            translationView.translationHtml = html;
            translationView.translationError = "";
        }

        function onRequestFailed(area, errorString) {
            if (area === "definition") {
                translationView.definitionError = errorString;
                translationView.definitionHtml = "";
            } else if (area === "translation") {
                translationView.translationError = errorString;
                translationView.translationHtml = "";
            }
        }
    }

    MediaPlayer {
        id: mediaPlayer
        audioOutput: AudioOutput {}
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        // Word header
        RowLayout {
            Layout.fillWidth: true

            ColumnLayout {
                spacing: 2
                Label {
                    text: currentWord
                    font.pixelSize: 28
                    font.bold: true
                    visible: currentWord !== ""
                }
                Label {
                    text: phonetic
                    font.pixelSize: 14
                    opacity: 0.6
                    visible: phonetic !== ""
                }
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "Play Pronunciation"
                enabled: audioSource.toString() !== ""
                onClicked: {
                    mediaPlayer.source = audioSource;
                    mediaPlayer.play();
                }
            }
        }

        // English Definition
        GroupBox {
            Layout.fillWidth: true
            Layout.fillHeight: true
            title: "English Definition"

            ScrollView {
                anchors.fill: parent
                clip: true

                Text {
                    width: parent.width
                    textFormat: Text.RichText
                    wrapMode: Text.Wrap
                    text: definitionError !== "" ? "<p style='color: #e74c3c;'>" + definitionError + "</p>" : definitionHtml
                    color: palette.text
                    visible: definitionHtml !== "" || definitionError !== ""
                }

                Label {
                    anchors.centerIn: parent
                    text: "Select a word to see its definition"
                    opacity: 0.5
                    visible: definitionHtml === "" && definitionError === "" && currentWord === ""
                }

                BusyIndicator {
                    anchors.centerIn: parent
                    running: currentWord !== "" && definitionHtml === "" && definitionError === ""
                }
            }
        }

        // Arabic Translation
        GroupBox {
            Layout.fillWidth: true
            Layout.preferredHeight: 120
            title: "Arabic Translation"

            ScrollView {
                anchors.fill: parent
                clip: true

                Text {
                    width: parent.width
                    textFormat: Text.RichText
                    wrapMode: Text.Wrap
                    horizontalAlignment: Text.AlignRight
                    text: translationError !== "" ? "<p style='color: #e74c3c;'>" + translationError + "</p>" : translationHtml
                    color: palette.text
                    visible: translationHtml !== "" || translationError !== ""
                }

                Label {
                    anchors.centerIn: parent
                    text: "Translation will appear here"
                    opacity: 0.5
                    visible: translationHtml === "" && translationError === "" && currentWord === ""
                }

                BusyIndicator {
                    anchors.centerIn: parent
                    running: currentWord !== "" && translationHtml === "" && translationError === ""
                }
            }
        }
    }
}
```

- [ ] **Step 2: Add TranslationView.qml to CMakeLists.txt**

```cmake
qt_add_qml_module(apporVocab
    URI orVocab
    QML_FILES
        Main.qml
        Sidebar.qml
        TranslationView.qml
)
```

- [ ] **Step 3: Build and test the full app**

Run:
```bash
cd /mnt/BA707A64707A2773/code/orVocab
cmake --build build
./build/apporVocab
```

Expected: Full app launches. Test the following:
1. Type "hello" and click Add → appears in sidebar
2. Double-click "hello" → definition and Arabic translation load, phonetic appears, Play Pronunciation button becomes enabled
3. Click Play Pronunciation → audio plays
4. Right-click "hello" → Delete from context menu
5. Toolbar buttons open file dialogs

- [ ] **Step 4: Run all unit tests to confirm nothing broke**

Run:
```bash
cd /mnt/BA707A64707A2773/code/orVocab/build
ctest --output-on-failure
```

Expected: All tests PASS.

- [ ] **Step 5: Commit**

```bash
git add orVocab/TranslationView.qml orVocab/CMakeLists.txt
git commit -m "feat(RVC-1): ADDED: TranslationView with definition, translation, and audio playback"
```

---

## Task 10: Remove Placeholder Test and Final Cleanup

**Files:**
- Delete: `tests/test_placeholder.cpp`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Remove placeholder test**

Delete `tests/test_placeholder.cpp` and remove it from `tests/CMakeLists.txt`:

```cmake
add_executable(orVocabTests
    test_vocabmanager.cpp
    test_networkclient.cpp
    ${CMAKE_SOURCE_DIR}/orVocab/vocabmanager.cpp
    ${CMAKE_SOURCE_DIR}/orVocab/networkclient.cpp
)
```

- [ ] **Step 2: Build and run all tests**

Run:
```bash
cd /mnt/BA707A64707A2773/code/orVocab
cmake --build build
cd build && ctest --output-on-failure
```

Expected: All tests PASS. No placeholder test in output.

- [ ] **Step 3: Run the app and verify all features**

Run:
```bash
cd /mnt/BA707A64707A2773/code/orVocab
./build/apporVocab
```

Verify the complete user journey:
1. App starts with empty word list
2. Type "algorithm" → click Add → appears in sidebar sorted
3. Type "binary" → click Add → appears below "algorithm"
4. Type "al" in search → list filters to show "algorithm" only
5. Clear search → full list shown
6. Double-click "algorithm" → definition + Arabic translation + phonetic load
7. Click Play Pronunciation → audio plays
8. Right-click "binary" → Delete → removed from list
9. Export JSON → file dialog → saves correctly
10. Export Text → file dialog → saves correctly
11. Import Text → file dialog → merges words
12. Import JSON → file dialog → confirmation → replaces word list
13. Close and reopen app → word list persists

- [ ] **Step 4: Commit**

```bash
git add tests/CMakeLists.txt
git rm tests/test_placeholder.cpp
git commit -m "chore(RVC-1): REMOVED: placeholder test, final cleanup"
```
