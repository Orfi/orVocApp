// orVocApp/vocabmanager.h
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

    Q_INVOKABLE QObject* createPdfExporter(int pageSize);

signals:
    void wordsChanged();

private:
    QStringListModel *m_sourceModel;
    QSortFilterProxyModel *m_proxyModel;

    QString dataFilePath() const;
};

#endif // VOCABMANAGER_H
