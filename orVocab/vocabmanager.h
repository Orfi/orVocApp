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
