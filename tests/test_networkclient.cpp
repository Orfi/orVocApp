#include <catch2/catch_all.hpp>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include "networkclient.h"

static int argc = 1;
static char appName[] = "test";
static char *argv[] = {appName, nullptr};

TEST_CASE("NetworkClient dictionary parsing", "[networkclient][dictionary]") {
    QCoreApplication app(argc, argv);

    // Sample dictionary API response for "hello"
    QByteArray dictResponse = R"([
        {
            "meta": { "id": "hello" },
            "fl": "interjection",
            "hwi": {
                "hw": "hello",
                "prs": [
                    { "mw": "he-\u2032l\u014D", "ipa": "/h\u0259\u02c8lo\u028a/", "sound": { "audio": "hello0001" } }
                ]
            },
            "shortdef": [
                "Used as a greeting.",
                "An utterance of \u201chello\u201d; a greeting."
            ]
        }
    ])";

    SECTION("parseDictionaryResponse extracts formatted HTML") {
        auto result = NetworkClient::parseDictionaryResponse(dictResponse);
        REQUIRE_FALSE(result.html.isEmpty());
        REQUIRE(result.html.contains("interjection"));
        REQUIRE(result.html.contains("Used as a greeting."));
    }

    SECTION("parseDictionaryResponse extracts phonetic text") {
        auto result = NetworkClient::parseDictionaryResponse(dictResponse);
        REQUIRE(result.phonetic == "/h\u0259\u02c8lo\u028a/");
    }

    SECTION("parseDictionaryResponse extracts first valid mp3 URL") {
        auto result = NetworkClient::parseDictionaryResponse(dictResponse);
        REQUIRE(result.audioUrl == QUrl("https://media.merriam-webster.com/soundc11/h/hello0001.wav"));
    }

    SECTION("parseDictionaryResponse with no audio returns empty URL") {
        QByteArray noAudio = R"([{
            "meta": { "id": "test" },
            "fl": "noun",
            "hwi": { "hw": "test", "prs": [ { "mw": "\u2032test" } ] },
            "shortdef": [ "A trial." ]
        }])";

        auto result = NetworkClient::parseDictionaryResponse(noAudio);
        REQUIRE(result.audioUrl.isEmpty());
    }

    SECTION("parseDictionaryResponse with empty data returns error") {
        auto result = NetworkClient::parseDictionaryResponse("{}");
        REQUIRE(result.html.isEmpty());
        REQUIRE(result.error == true);
    }
}

TEST_CASE("NetworkClient translation parsing", "[networkclient][translation]") {
    QCoreApplication app(argc, argv);

    // Sample GTX translation API response for "hello" → Arabic
    QByteArray translationResponse = R"([[["مرحبا","hello",null,null,10]],null,"en"])";

    SECTION("parseTranslationResponse extracts Arabic text with RTL HTML") {
        auto result = NetworkClient::parseTranslationResponse(translationResponse);
        REQUIRE_FALSE(result.html.isEmpty());
        REQUIRE(result.html.contains("مرحبا"));
        REQUIRE(result.html.contains("dir=\"rtl\""));
    }

    SECTION("parseTranslationResponse with empty data returns error") {
        auto result = NetworkClient::parseTranslationResponse("[]");
        REQUIRE(result.error == true);
    }

    SECTION("parseTranslationResponse with invalid JSON returns error") {
        auto result = NetworkClient::parseTranslationResponse("not json");
        REQUIRE(result.error == true);
    }
}
