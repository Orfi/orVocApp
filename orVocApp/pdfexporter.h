#ifndef PDFEXPORTER_H
#define PDFEXPORTER_H

#include "baseexporter.h"

#include <QPageSize>

class QPainter;

class PdfExporter : public BaseExporter
{
    Q_OBJECT

public:
    explicit PdfExporter(const QStringList &words,
                         QPageSize::PageSizeId pageSize,
                         QObject *parent = nullptr);

    bool renderToFile(const QVector<WordEntry> &entries, const QString &outputPath) override;
    static QColor letterColor(int letterIndex);

private:
    void drawCoverPage(QPainter &painter, int wordCount, const QRectF &pageRect);
    void drawPageHeader(QPainter &painter, QChar letter, int letterIndex, const QRectF &pageRect);
    void drawPageFooter(QPainter &painter, int pageNumber, const QRectF &pageRect);
    qreal drawWordEntry(QPainter &painter, const WordEntry &entry, qreal yPos, const QRectF &pageRect);
    qreal measureWordEntry(const WordEntry &entry, const QRectF &pageRect);

    QPageSize::PageSizeId m_pageSize;
};

#endif // PDFEXPORTER_H
