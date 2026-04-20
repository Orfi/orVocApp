# Design: Replace QStringList with QSortFilterProxyModel (RVC-30)

## Summary

Replace the manual `QStringList filteredWords` property in `VocabManager` with a `QStringListModel` + `QSortFilterProxyModel` pair. This gives the ListView granular row-level change signals instead of full-list rebuilds, enabling smooth performance at scale.

## Architecture

VocabManager owns:
- `QStringListModel *m_sourceModel` — holds the full sorted word list
- `QSortFilterProxyModel *m_proxyModel` — filters/sorts, exposed to QML

## VocabManager Interface Changes

### New members
- `QStringListModel *m_sourceModel`
- `QSortFilterProxyModel *m_proxyModel`

### New property
- `Q_PROPERTY(QSortFilterProxyModel* wordModel READ wordModel CONSTANT)` — bound by QML ListView

### New Q_INVOKABLE
- `int indexOfWord(const QString &word)` — returns index of word in the proxy model (for scrolling after add)

### Modified methods
- `filterWords(query)` → calls `m_proxyModel->setFilterFixedString(query)`
- `addWord()` → binary-search sorted position, `m_sourceModel->insertRow()` + `setData()`
- `removeWord()` → find row in source via `m_sourceModel->stringList()`, call `m_sourceModel->removeRow()`
- `loadFromJson()` / `importJson()` / `importText()` → `m_sourceModel->setStringList(sorted)`
- `words()` → returns `m_sourceModel->stringList()`

### Removed
- `QStringList m_words`
- `QStringList m_filteredWords`
- `QString m_currentFilter`
- `void updateFilteredWords()`
- `QStringList filteredWords` Q_PROPERTY
- `filteredWordsChanged` signal

### Kept for tests
- `QStringList filteredWords()` as a regular method (not a property) — reads visible rows from proxy model for test assertions

## QML Changes (Sidebar.qml)

- `model: VocabManager.wordModel`
- Delegate: `model.display` instead of `modelData`
- After add: `VocabManager.indexOfWord(word)` instead of `VocabManager.filteredWords.indexOf(word)`

## Proxy Configuration

```cpp
m_proxyModel->setSourceModel(m_sourceModel);
m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
m_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
```

## Test Strategy

Existing test cases remain conceptually identical. The `filteredWords()` convenience method returns proxy contents as a QStringList, so assertions like `vm.filteredWords() == QStringList({...})` continue to work with minimal changes.
