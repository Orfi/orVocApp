#include <catch2/catch_all.hpp>

#include "baseexporter.h"

#include <QCoreApplication>
#include <QSignalSpy>
#include <QTimer>

static int argc = 1;
static char appName[] = "test";
static char *argv[] = {appName, nullptr};

namespace {
class StubExporter : public BaseExporter
{
public:
    explicit StubExporter(const QStringList &words) : BaseExporter(words) {}

protected:
    bool renderToFile(const QVector<WordEntry> &, const QString &) override
    {
        return true;
    }
};
}

TEST_CASE("BaseExporter cancel before first fetch emits cancelled without finished",
          "[baseexporter][cancel]")
{
    QCoreApplication app(argc, argv);

    StubExporter exporter({"apple", "banana"});
    QSignalSpy cancelledSpy(&exporter, &BaseExporter::cancelled);
    QSignalSpy finishedSpy(&exporter, &BaseExporter::finished);

    exporter.exportToFile("/tmp/unused-cancel-test.pdf");
    exporter.requestCancel();

    QTimer::singleShot(50, &app, [&]() { app.quit(); });
    app.exec();

    REQUIRE(cancelledSpy.count() >= 1);
    REQUIRE(finishedSpy.count() == 0);
}
