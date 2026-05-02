#ifndef PDFEXPORTER_H
#define PDFEXPORTER_H

#include "baseexporter.h"

#include <QPageSize>

class QPainter;

/**
 * @brief Exports the word bank as a formatted PDF dictionary.
 *
 * Produces a multi-page PDF with a cover page, letter-grouped entries (one letter
 * group per page with per-letter color tabs), and per-page headers and footers.
 * Entries are laid out via QTextDocument using an inline HTML template.
 *
 * Rendering runs on a worker thread spawned by BaseExporter; PdfExporter itself
 * contributes only the page-level layout and drawing logic.
 */
class PdfExporter : public BaseExporter
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a PdfExporter for the given words and page size.
     * @param words The source words to fetch and render.
     * @param pageSize The QPageSize::PageSizeId to use (typically A4 or Letter).
     * @param parent Optional QObject parent for lifetime management.
     */
    explicit PdfExporter(const QStringList &words,
                         QPageSize::PageSizeId pageSize,
                         QObject *parent = nullptr);

    /**
     * @brief Renders the given entries to a PDF file at @p outputPath.
     * @param entries Successfully-fetched entries, already filtered by the base class.
     * @param outputPath Absolute path to the PDF file to write.
     * @return True on successful render, false if the QPainter could not be opened
     *         on the QPrinter.
     *
     * Called on a worker thread. Lays out a cover page followed by one or more
     * pages per leading letter, flowing entries using QTextDocument measurement.
     */
    bool renderToFile(const QVector<WordEntry> &entries, const QString &outputPath) override;

    /**
     * @brief Produces a distinct HSV-derived color for each alphabet letter.
     * @param letterIndex Zero-based letter index (0 = 'A', 25 = 'Z').
     * @return A valid QColor unique within the 26-letter range.
     */
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
