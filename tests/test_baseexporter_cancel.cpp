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

TEST_CASE("BaseExporter retryBackoffMs produces the documented schedule",
          "[baseexporter][retry]")
{
    REQUIRE(BaseExporter::retryBackoffMs(0) == 500);
    REQUIRE(BaseExporter::retryBackoffMs(1) == 1500);
    REQUIRE(BaseExporter::retryBackoffMs(2) == 4500);
}

TEST_CASE("BaseExporter retry constants are sane",
          "[baseexporter][retry]")
{
    REQUIRE(BaseExporter::kMaxRetries == 3);
    REQUIRE(BaseExporter::kTransferTimeoutMs >= 5000);
}

TEST_CASE("BaseExporter inter-word delay is paced for Cloudflare rate-limits",
          "[baseexporter][retry]")
{
    // Cloudflare 1015 trips on dictionaryapi.dev at burst rates;
    // keep at least 1 req/sec between words as a policy floor.
    REQUIRE(BaseExporter::kInterWordDelayMs >= 1000);
}
