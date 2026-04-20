# QSortFilterProxyModel Refactor Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace manual QStringList filtering in VocabManager with QStringListModel + QSortFilterProxyModel for granular row-level updates and smooth scrolling at scale.

**Architecture:** VocabManager owns a QStringListModel (source data) and QSortFilterProxyModel (filtering/sorting). The proxy is exposed to QML as a `wordModel` property. Manual filtering code is removed entirely.

**Tech Stack:** Qt 6.10.2 (QStringListModel, QSortFilterProxyModel), QML, Catch2 v3

---

## File Structure

- **Modify:** `orVocab/vocabmanager.h` — Replace QStringList members with model pointers, new property/method
- **Modify:** `orVocab/vocabmanager.cpp` — Rewrite internals to use model API
- **Modify:** `orVocab/Sidebar.qml` — Bind to new model property, update delegate
- **Modify:** `tests/test_vocabmanager.cpp` — Minor updates for new signal name

---

### Task 1: Update VocabManager Header

**Files:**
- Modify: `orVocab/vocabmanager.h`

- [ ] **Step 1: Update the header file**

Replace the current header with the new interface using model pointers:

```cpp
// orVocab/vocabmanager.h
#ifndef VOCABMANAGER_H
#define VOCABMANAGER_H

#include <QObject>
#include <QSortFilterProxyModel>
#include <QStringList>
#include <QStringListModel>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

class VocabManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QStringList words READ words NOTIFY wordsChanged)
    Q_PROPERTY(QSortFilterProxyModel* wordModel READ wordModel CONSTANT)

public:
    explicit VocabManager(QObject *parent = nullptr);

    QStringList words() const;
    QSortFilterProxyModel* wordModel() const;
    QStringList filteredWords() const;

    Q_INVOKABLE int addWord(const QString &word);
    Q_INVOKABLE void removeWord(const QString &word);
    Q_INVOKABLE void filterWords(const QString &query);
    Q_INVOKABLE int indexOfWord(const QString &word) const;

    Q_INVOKABLE void loadFromJson();
    Q_INVOKABLE void saveToJson();
    Q_INVOKABLE void exportJson(const QUrl &path);
    Q_INVOKABLE void exportText(const QUrl &path);
    Q_INVOKABLE void importJson(const QUrl &path);
    Q_INVOKABLE void importText(const QUrl &path);

signals:
    void wordsChanged();

private:
    QStringListModel *m_sourceModel;
    QSortFilterProxyModel *m_proxyModel;

    QString dataFilePath() const;
};

#endif // VOCABMANAGER_H
```

- [ ] **Step 2: Commit**

```bash
git add orVocab/vocabmanager.h
git commit -m "refactor(RVC-30): CHANGED: VocabManager header to use QSortFilterProxyModel"
```

---

### Task 2: Rewrite VocabManager Implementation

**Files:**
- Modify: `orVocab/vocabmanager.cpp`

- [ ] **Step 1: Rewrite the implementation**

Replace the full implementation with model-based logic:

```cpp
// orVocab/vocabmanager.cpp
#include "vocabmanager.h"
#include <algorithm>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

VocabManager::VocabManager(QObject *parent)
    : QObject(parent)
    , m_sourceModel(new QStringListModel(this))
    , m_proxyModel(new QSortFilterProxyModel(this))
{
    m_proxyModel->setSourceModel(m_sourceModel);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
}

QStringList VocabManager::words() const
{
    return m_sourceModel->stringList();
}

QSortFilterProxyModel* VocabManager::wordModel() const
{
    return m_proxyModel;
}

QStringList VocabManager::filteredWords() const
{
    QStringList result;
    for (int i = 0; i < m_proxyModel->rowCount(); ++i) {
        QModelIndex idx = m_proxyModel->index(i, 0);
        result.append(idx.data(Qt::DisplayRole).toString());
    }
    return result;
}

int VocabManager::addWord(const QString &word)
{
    QString normalized = word.trimmed().toLower();
    if (normalized.isEmpty())
        return -1;

    QStringList current = m_sourceModel->stringList();
    auto it = std::lower_bound(current.begin(), current.end(), normalized);
    int idx = static_cast<int>(it - current.begin());

    if (it != current.end() && *it == normalized)
        return idx;

    m_sourceModel->insertRow(idx);
    m_sourceModel->setData(m_sourceModel->index(idx, 0), normalized);
    saveToJson();
    emit wordsChanged();
    return idx;
}

void VocabManager::removeWord(const QString &word)
{
    QString normalized = word.trimmed().toLower();
    QStringList current = m_sourceModel->stringList();
    int idx = current.indexOf(normalized);
    if (idx >= 0) {
        m_sourceModel->removeRow(idx);
        saveToJson();
        emit wordsChanged();
    }
}

void VocabManager::filterWords(const QString &query)
{
    m_proxyModel->setFilterFixedString(query);
}

int VocabManager::indexOfWord(const QString &word) const
{
    QString normalized = word.trimmed().toLower();
    for (int i = 0; i < m_proxyModel->rowCount(); ++i) {
        QModelIndex idx = m_proxyModel->index(i, 0);
        if (idx.data(Qt::DisplayRole).toString() == normalized)
            return i;
    }
    return -1;
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
    QStringList words;
    for (const QJsonValue &v : arr)
        words.append(v.toString());

    m_sourceModel->setStringList(words);
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
    for (const QString &w : m_sourceModel->stringList())
        arr.append(w);

    QJsonObject obj;
    obj["words"] = arr;

    file.write(QJsonDocument(obj).toJson());
    file.close();
}

void VocabManager::exportJson(const QUrl &path)
{
    QFile file(path.toLocalFile());
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning("VocabManager: cannot write to %s", qPrintable(path.toLocalFile()));
        return;
    }

    QJsonArray arr;
    for (const QString &w : m_sourceModel->stringList())
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

    for (const QString &w : m_sourceModel->stringList()) {
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
    m_sourceModel->setStringList(newWords);
    saveToJson();
    emit wordsChanged();
}

void VocabManager::importText(const QUrl &path)
{
    QFile file(path.toLocalFile());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("VocabManager: cannot read %s", qPrintable(path.toLocalFile()));
        return;
    }

    QStringList current = m_sourceModel->stringList();
    while (!file.atEnd()) {
        QString line = QString::fromUtf8(file.readLine()).trimmed().toLower();
        if (line.isEmpty())
            continue;

        auto it = std::lower_bound(current.begin(), current.end(), line);
        if (it == current.end() || *it != line)
            current.insert(it, line);
    }
    file.close();

    m_sourceModel->setStringList(current);
    saveToJson();
    emit wordsChanged();
}

QString VocabManager::dataFilePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return dir + "/vocab.json";
}
```

- [ ] **Step 2: Commit**

```bash
git add orVocab/vocabmanager.cpp
git commit -m "refactor(RVC-30): CHANGED: VocabManager implementation to use QSortFilterProxyModel"
```

---

### Task 3: Update Sidebar.qml

**Files:**
- Modify: `orVocab/Sidebar.qml`

- [ ] **Step 1: Update model binding and delegate**

Three changes in Sidebar.qml:

1. Line 42 — change `VocabManager.filteredWords.indexOf(word)` to `VocabManager.indexOfWord(word)`
2. Line 61 — change `model: VocabManager.filteredWords` to `model: VocabManager.wordModel`
3. Line 67 — change `text: modelData` to `text: model.display`
4. Lines 71, 79 — change `modelData` references to `model.display`

Full updated Sidebar.qml:

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
    signal wordDeleted(string word)

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
                onAccepted: addButton.clicked()
            }

            Button {
                id: addButton
                text: "Add"
                onClicked: {
                    if (searchField.text.trim() === "")
                        return;
                    var word = searchField.text.trim().toLowerCase();
                    VocabManager.addWord(searchField.text);
                    searchField.text = "";
                    var idx = VocabManager.indexOfWord(word);
                    if (idx >= 0) {
                        wordList.currentIndex = idx;
                        sidebar.wordDoubleClicked(word);
                    }
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
            model: VocabManager.wordModel
            clip: true
            currentIndex: -1

            delegate: ItemDelegate {
                width: wordList.width
                text: model.display
                highlighted: wordList.currentIndex === index

                onDoubleClicked: {
                    wordList.currentIndex = index;
                    sidebar.wordDoubleClicked(model.display);
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.RightButton
                    onClicked: function(mouse) {
                        wordList.currentIndex = index;
                        contextMenu.selectedWord = model.display;
                        contextMenu.popup();
                    }
                }
            }

            Menu {
                id: contextMenu
                property string selectedWord: ""

                MenuItem {
                    text: "Delete"
                    onTriggered: {
                        var word = contextMenu.selectedWord;
                        VocabManager.removeWord(word);
                        sidebar.wordDeleted(word);
                    }
                }
            }
        }
    }
}
```

- [ ] **Step 2: Commit**

```bash
git add orVocab/Sidebar.qml
git commit -m "refactor(RVC-30): CHANGED: Sidebar.qml to use wordModel property"
```

---

### Task 4: Update Tests

**Files:**
- Modify: `tests/test_vocabmanager.cpp`

- [ ] **Step 1: Update test file**

The existing tests call `vm.filteredWords()` which still exists as a convenience method reading from the proxy. The only change needed is removing any reliance on the `filteredWordsChanged` signal (tests don't use it directly, so this is a no-op).

However, one behavioral difference: after `addWord` with a filter active, the old code called `updateFilteredWords()` which re-applied the filter. With the proxy model, filtering is automatic — but `addWord` inserts into the source model, and the proxy auto-filters. The test on line 103-106 expects `"alpha"` to appear in filtered results after adding with filter `"al"` active. This still works because the proxy re-evaluates automatically.

The test at line 106 expects order `{"algorithm", "allocate", "alpha"}`. With the proxy model, since we insert at sorted position in the source model (`alpha` goes between `algorithm` and `allocate`), the proxy will show them in source order: `{"algorithm", "alpha", "allocate"}`. Update this assertion:

```cpp
    SECTION("filteredWords updates when word is added matching current filter") {
        vm.filterWords("al");
        vm.addWord("alpha");
        REQUIRE(vm.filteredWords() == QStringList({"algorithm", "alpha", "allocate"}));
    }
```

That's the only test change needed.

- [ ] **Step 2: Run tests to verify they pass**

Run:
```bash
cd build/Desktop_Qt_6_10_2-Debug && ctest --output-on-failure
```

Expected: All tests PASS

- [ ] **Step 3: Commit**

```bash
git add tests/test_vocabmanager.cpp
git commit -m "test(RVC-30): CHANGED: filter test assertion to match proxy model sort order"
```

---

### Task 5: Build and Verify

- [ ] **Step 1: Full rebuild**

```bash
cmake --build build/Desktop_Qt_6_10_2-Debug --target clean
cmake --build build/Desktop_Qt_6_10_2-Debug
```

Expected: Builds with no errors or warnings

- [ ] **Step 2: Run all tests**

```bash
cd build/Desktop_Qt_6_10_2-Debug && ctest --output-on-failure
```

Expected: All tests PASS

- [ ] **Step 3: Manual smoke test**

Run the application:
```bash
./build/Desktop_Qt_6_10_2-Debug/orVocab/orVocApp
```

Verify:
- Full word list displays on launch
- Typing in search field narrows the list
- Clearing search shows all words again
- Adding a word inserts it and scrolls to it
- Right-click delete removes the word
- Scrolling is smooth

- [ ] **Step 4: Final commit (if any fixups needed)**

Only if smoke testing reveals issues that need fixing.
