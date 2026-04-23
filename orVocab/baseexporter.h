#ifndef BASEEXPORTER_H
#define BASEEXPORTER_H

#include <QObject>
#include <QString>
#include <QUrl>
#include <QVector>

class QNetworkAccessManager;
class QThread;

struct WordEntry {
    QString word;
    QString phonetic;
    QString definitionHtml;
    QString translationHtml;
    bool valid = false;
};

class BaseExporter : public QObject
{
    Q_OBJECT

public:
    explicit BaseExporter(const QStringList &words, QObject *parent = nullptr);
    ~BaseExporter() override;

    void exportToFile(const QString &outputPath);

signals:
    void progress(int current, int total);
    void finished(bool success, const QString &filePath);

protected:
    virtual bool renderToFile(const QVector<WordEntry> &entries, const QString &outputPath) = 0;

private:
    void doWork();
    void fetchWord(const QString &word, WordEntry &entry);

    QStringList m_words;
    QString m_outputPath;
    QThread *m_thread = nullptr;
    QNetworkAccessManager *m_nam = nullptr;
};

#endif // BASEEXPORTER_H
