// tests/test_vocabmanager.cpp
#include <catch2/catch_all.hpp>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include "vocabmanager.h"

static int argc = 1;
static char appName[] = "test";
static char *argv[] = {appName, nullptr};

TEST_CASE("VocabManager add and remove words", "[vocabmanager]") {
    QCoreApplication app(argc, argv);
    app.setApplicationName("orVocab-test");

    VocabManager vm;

    SECTION("addWord inserts in alphabetical order") {
        vm.addWord("cherry");
        vm.addWord("apple");
        vm.addWord("banana");
        REQUIRE(vm.words() == QStringList({"apple", "banana", "cherry"}));
    }

    SECTION("addWord returns index of inserted word") {
        int idx = vm.addWord("banana");
        REQUIRE(idx == 0);
        idx = vm.addWord("apple");
        REQUIRE(idx == 0);  // apple sorts before banana
    }

    SECTION("addWord with duplicate returns existing index without adding") {
        vm.addWord("apple");
        vm.addWord("banana");
        int idx = vm.addWord("apple");
        REQUIRE(idx == 0);
        REQUIRE(vm.words().size() == 2);
    }

    SECTION("removeWord removes existing word") {
        vm.addWord("apple");
        vm.addWord("banana");
        vm.removeWord("apple");
        REQUIRE(vm.words() == QStringList({"banana"}));
    }

    SECTION("removeWord with nonexistent word does nothing") {
        vm.addWord("apple");
        vm.removeWord("banana");
        REQUIRE(vm.words() == QStringList({"apple"}));
    }

    SECTION("addWord normalizes to lowercase and trims whitespace") {
        vm.addWord("  Apple  ");
        vm.addWord("BANANA");
        REQUIRE(vm.words() == QStringList({"apple", "banana"}));
    }

    SECTION("addWord rejects empty string") {
        int idx = vm.addWord("");
        REQUIRE(idx == -1);
        idx = vm.addWord("   ");
        REQUIRE(idx == -1);
        REQUIRE(vm.words().isEmpty());
    }
}

TEST_CASE("VocabManager filter words", "[vocabmanager]") {
    QCoreApplication app(argc, argv);
    app.setApplicationName("orVocab-test");

    VocabManager vm;
    vm.addWord("algorithm");
    vm.addWord("allocate");
    vm.addWord("binary");
    vm.addWord("cache");

    SECTION("empty filter returns all words") {
        vm.filterWords("");
        REQUIRE(vm.filteredWords() == QStringList({"algorithm", "allocate", "binary", "cache"}));
    }

    SECTION("filter narrows list with case-insensitive contains") {
        vm.filterWords("al");
        REQUIRE(vm.filteredWords() == QStringList({"algorithm", "allocate"}));
    }

    SECTION("filter with no matches returns empty list") {
        vm.filterWords("xyz");
        REQUIRE(vm.filteredWords().isEmpty());
    }

    SECTION("filter is case-insensitive") {
        vm.filterWords("AL");
        REQUIRE(vm.filteredWords() == QStringList({"algorithm", "allocate"}));
    }

    SECTION("filteredWords updates when word is added matching current filter") {
        vm.filterWords("al");
        vm.addWord("alpha");
        REQUIRE(vm.filteredWords() == QStringList({"algorithm", "allocate", "alpha"}));
    }

    SECTION("filteredWords updates when matching word is removed") {
        vm.filterWords("al");
        vm.removeWord("algorithm");
        REQUIRE(vm.filteredWords() == QStringList({"allocate"}));
    }
}

TEST_CASE("VocabManager JSON persistence", "[vocabmanager][json]") {
    QCoreApplication app(argc, argv);
    app.setApplicationName("orVocab-test");

    // Use test-specific data location to avoid polluting real data
    QStandardPaths::setTestModeEnabled(true);

    // Clean up any leftover test data
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir(dataDir).removeRecursively();

    SECTION("loadFromJson with missing file starts empty") {
        VocabManager vm;
        vm.loadFromJson();
        REQUIRE(vm.words().isEmpty());
    }

    SECTION("saveToJson then loadFromJson round-trips correctly") {
        {
            VocabManager vm;
            vm.addWord("cherry");
            vm.addWord("apple");
            vm.addWord("banana");
            vm.saveToJson();
        }
        {
            VocabManager vm;
            vm.loadFromJson();
            REQUIRE(vm.words() == QStringList({"apple", "banana", "cherry"}));
        }
    }

    SECTION("loadFromJson with corrupt JSON starts empty") {
        QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dataDir);
        QFile file(dataDir + "/vocab.json");
        file.open(QIODevice::WriteOnly);
        file.write("not valid json {{{");
        file.close();

        VocabManager vm;
        vm.loadFromJson();
        REQUIRE(vm.words().isEmpty());
    }

    SECTION("addWord auto-saves to JSON") {
        {
            VocabManager vm;
            vm.loadFromJson();
            vm.addWord("apple");
            vm.addWord("banana");
        }
        {
            VocabManager vm;
            vm.loadFromJson();
            REQUIRE(vm.words() == QStringList({"apple", "banana"}));
        }
    }

    SECTION("removeWord auto-saves to JSON") {
        {
            VocabManager vm;
            vm.loadFromJson();
            vm.addWord("apple");
            vm.addWord("banana");
        }
        {
            VocabManager vm;
            vm.loadFromJson();
            vm.removeWord("apple");
        }
        {
            VocabManager vm;
            vm.loadFromJson();
            REQUIRE(vm.words() == QStringList({"banana"}));
        }
    }

    // Clean up
    QDir(dataDir).removeRecursively();
    QStandardPaths::setTestModeEnabled(false);
}
