#include "pdfexporter.h"

#include <QDate>
#include <QFile>
#include <QFont>
#include <QMargins>
#include <QPainter>
#include <QPrinter>
#include <QTextDocument>

static const qreal kMarginMm = 20.0;
static const qreal kHeaderHeight = 40.0;
static const qreal kFooterHeight = 30.0;
static const qreal kEntrySpacing = 16.0;

PdfExporter::PdfExporter(const QStringList &words,
                         QPageSize::PageSizeId pageSize,
                         QObject *parent)
    : BaseExporter(words, parent)
    , m_pageSize(pageSize)
{
}

QColor PdfExporter::letterColor(int letterIndex)
{
    qreal hue = (letterIndex / 26.0) * 360.0;
    return QColor::fromHsv(static_cast<int>(hue), 200, 200);
}

bool PdfExporter::renderToFile(const QVector<WordEntry> &entries, const QString &outputPath)
{
    QPrinter printer(QPrinter::ScreenResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(outputPath);
    printer.setPageSize(QPageSize(m_pageSize));
    printer.setPageOrientation(QPageLayout::Portrait);
    printer.setPageMargins(QMarginsF(kMarginMm, kMarginMm, kMarginMm, kMarginMm),
                           QPageLayout::Millimeter);

    QPainter painter;
    if (!painter.begin(&printer))
        return false;

    QRectF pageRect = printer.pageRect(QPrinter::DevicePixel);

    // Cover page
    drawCoverPage(painter, entries.size(), pageRect);

    // Group entries by first letter
    QMap<QChar, QVector<const WordEntry *>> letterGroups;
    for (const auto &e : entries) {
        QChar letter = e.word.at(0).toUpper();
        letterGroups[letter].append(&e);
    }

    int pageNumber = 1;
    bool cancelledMidRender = false;

    for (auto it = letterGroups.constBegin(); it != letterGroups.constEnd() && !cancelledMidRender; ++it) {
        QChar letter = it.key();
        int letterIndex = letter.unicode() - QChar('A').unicode();
        const auto &words = it.value();

        printer.newPage();
        pageNumber++;

        if (isCancelled()) {
            cancelledMidRender = true;
            break;
        }

        drawPageHeader(painter, letter, letterIndex, pageRect);
        drawPageFooter(painter, pageNumber, pageRect);

        qreal contentTop = kHeaderHeight + kEntrySpacing;
        qreal contentBottom = pageRect.height() - kFooterHeight;
        qreal yPos = contentTop;

        for (const auto *entry : words) {
            qreal entryHeight = measureWordEntry(*entry, pageRect);

            if (yPos + entryHeight > contentBottom) {
                printer.newPage();
                pageNumber++;

                if (isCancelled()) {
                    cancelledMidRender = true;
                    break;
                }

                drawPageHeader(painter, letter, letterIndex, pageRect);
                drawPageFooter(painter, pageNumber, pageRect);
                yPos = contentTop;
            }

            yPos = drawWordEntry(painter, *entry, yPos, pageRect);
            yPos += kEntrySpacing;
        }
    }

    painter.end();

    if (cancelledMidRender || isCancelled()) {
        QFile::remove(outputPath);
        return false;
    }

    return true;
}

void PdfExporter::drawCoverPage(QPainter &painter, int wordCount, const QRectF &pageRect)
{
    qreal centerX = pageRect.width() / 2.0;
    qreal centerY = pageRect.height() / 2.0;

    // Decorative lines (top and bottom)
    qreal lineY_top = pageRect.height() * 0.15;
    qreal lineY_bottom = pageRect.height() * 0.85;
    qreal lineLeft = pageRect.width() * 0.1;
    qreal lineRight = pageRect.width() * 0.9;

    QColor darkBlue("#1a365d");
    for (qreal pos : {lineY_top, lineY_bottom}) {
        painter.fillRect(QRectF(lineLeft, pos, lineRight - lineLeft, 3), darkBlue);
    }

    // Title
    QFont titleFont("Georgia", 36, QFont::Bold);
    painter.setFont(titleFont);
    painter.setPen(Qt::black);
    QRectF titleRect(0, centerY - 80, pageRect.width(), 50);
    painter.drawText(titleRect, Qt::AlignCenter, "orVocApp");

    // Subtitle
    QFont subtitleFont("Georgia", 20);
    subtitleFont.setItalic(true);
    painter.setFont(subtitleFont);
    painter.setPen(QColor("#444444"));
    QRectF subtitleRect(0, centerY - 25, pageRect.width(), 35);
    painter.drawText(subtitleRect, Qt::AlignCenter, "Personal Dictionary");

    // Divider
    painter.setPen(QPen(QColor("#2d5a9e"), 2));
    painter.drawLine(QPointF(centerX - 40, centerY + 30),
                     QPointF(centerX + 40, centerY + 30));

    // Metadata
    QFont metaFont("Georgia", 12);
    painter.setFont(metaFont);
    painter.setPen(QColor("#666666"));
    QRectF wordsRect(0, centerY + 50, pageRect.width(), 25);
    painter.drawText(wordsRect, Qt::AlignCenter,
                     QString("Words: %1").arg(wordCount));
    QRectF dateRect(0, centerY + 75, pageRect.width(), 25);
    painter.drawText(dateRect, Qt::AlignCenter,
                     QString("Generated: %1").arg(QDate::currentDate().toString("MMMM d, yyyy")));
}

void PdfExporter::drawPageHeader(QPainter &painter, QChar letter, int letterIndex, const QRectF &pageRect)
{
    // "orVocApp Dictionary" left-aligned
    QFont headerFont("Georgia", 9);
    painter.setFont(headerFont);
    painter.setPen(QColor("#555555"));
    painter.drawText(QRectF(0, 0, pageRect.width() / 2, kHeaderHeight),
                     Qt::AlignLeft | Qt::AlignVCenter, "orVocApp Dictionary");

    // Colored letter tab top-right
    QColor tabColor = letterColor(letterIndex);
    qreal tabSize = 32;
    qreal tabX = pageRect.width() - tabSize - 4;
    qreal tabY = 4;
    painter.setBrush(tabColor);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(QRectF(tabX, tabY, tabSize, tabSize), 4, 4);

    QFont letterFont("Georgia", 16, QFont::Bold);
    painter.setFont(letterFont);
    painter.setPen(Qt::white);
    painter.drawText(QRectF(tabX, tabY, tabSize, tabSize),
                     Qt::AlignCenter, letter);

    // Header underline
    painter.setPen(QPen(QColor("#111111"), 2));
    painter.drawLine(QPointF(0, kHeaderHeight - 2),
                     QPointF(pageRect.width(), kHeaderHeight - 2));
}

void PdfExporter::drawPageFooter(QPainter &painter, int pageNumber, const QRectF &pageRect)
{
    qreal footerTop = pageRect.height() - kFooterHeight;

    painter.setPen(QPen(QColor("#cccccc"), 1));
    painter.drawLine(QPointF(0, footerTop + 4),
                     QPointF(pageRect.width(), footerTop + 4));

    QFont footerFont("Georgia", 8);
    painter.setFont(footerFont);
    painter.setPen(QColor("#888888"));
    painter.drawText(QRectF(0, footerTop + 8, pageRect.width() / 2, 20),
                     Qt::AlignLeft | Qt::AlignVCenter, "Generated by orVocApp");
    painter.drawText(QRectF(pageRect.width() / 2, footerTop + 8, pageRect.width() / 2, 20),
                     Qt::AlignRight | Qt::AlignVCenter, QString("Page %1").arg(pageNumber));
}

qreal PdfExporter::measureWordEntry(const WordEntry &entry, const QRectF &pageRect)
{
    qreal contentWidth = pageRect.width();

    QTextDocument doc;
    doc.setTextWidth(contentWidth);

    QString html = QString(
        "<div style='font-family: Georgia, serif; color: #000;'>"
        "<p style='font-size: 18px; font-weight: bold; color: #000; margin: 0;'>%1</p>"
        "<p style='font-size: 11px; color: #666; margin: 2px 0 8px 0;'>%2</p>"
        "<div style='color: #222;'>%3</div>"
        "<div style='margin-top: 8px; padding: 6px 10px; background: #f8f8f8; "
        "border: 1px solid #ddd; border-radius: 4px; text-align: right; "
        "direction: rtl; font-size: 15px; color: #000;'>%4</div>"
        "</div>")
        .arg(entry.word.toHtmlEscaped(),
             entry.phonetic.toHtmlEscaped(),
             entry.definitionHtml,
             entry.translationHtml);

    doc.setHtml(html);
    return doc.size().height() + kEntrySpacing;
}

qreal PdfExporter::drawWordEntry(QPainter &painter, const WordEntry &entry, qreal yPos, const QRectF &pageRect)
{
    qreal contentWidth = pageRect.width();

    QTextDocument doc;
    doc.setTextWidth(contentWidth);

    QString html = QString(
        "<div style='font-family: Georgia, serif; color: #000;'>"
        "<p style='font-size: 18px; font-weight: bold; color: #000; margin: 0;'>%1</p>"
        "<p style='font-size: 11px; color: #666; margin: 2px 0 8px 0;'>%2</p>"
        "<div style='color: #222;'>%3</div>"
        "<div style='margin-top: 8px; padding: 6px 10px; background: #f8f8f8; "
        "border: 1px solid #ddd; border-radius: 4px; text-align: right; "
        "direction: rtl; font-size: 15px; color: #000;'>%4</div>"
        "</div>")
        .arg(entry.word.toHtmlEscaped(),
             entry.phonetic.toHtmlEscaped(),
             entry.definitionHtml,
             entry.translationHtml);

    doc.setHtml(html);

    painter.save();
    painter.translate(0, yPos);
    doc.drawContents(&painter);
    painter.restore();

    return yPos + doc.size().height();
}
