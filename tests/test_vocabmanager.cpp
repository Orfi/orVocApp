// tests/test_vocabmanager.cpp
#include <catch2/catch_all.hpp>
#include <QCoreApplication>
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
